//22��
// line_follow_pd.c
// ��ݮ��С����������Ѳ�� + ����PD + ��̬���� + ���߻ָ�
// ����: wiringPi, softPwm
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <math.h>
#include <wiringPi.h>
#include <softPwm.h>

// ==================== ����ӳ�䣨wiringPi ��ţ� ====================
#define PWNA   1
#define AIN1   3
#define AIN2   2
#define PWNB   4
#define BIN1   6
#define BIN2   5

#define IR_LEFT   25
#define IR_CENTER 24
#define IR_RIGHT  23

// ==================== ��Ϊ��������������� ====================
// ����ĺ���ģ���� "��=0, ��=1"��������ĺ���һ���߼���ת
// #define SENSOR_BLACK_IS_ZERO 1

// PD ���棨�� PWM �ٷֱ�Ϊ�߶ȣ�
static float Kp = 45.0f;
static float Kd = 12.0f;

// �ٶ������ޣ�0~100�����Լ�Ŀ���ٶ�����
static int   BASE_MIN = 50;
static int   BASE_MAX = 70;
static float DV_MAX   = 4.0f;    // ÿ��������ٶȱ仯��PWM�㣩

// ���һ�׵�ͨ��0~1��Խ��Խƽ����x
static float LP_A = 0.10f; // 0.1 : 20.5s

// ���߻ָ�����
static int   RECOVER_TIMEOUT_MS = 300; // ���������ʱ��
static int   RECOVER_PWM        = 80;  // ����ʱ�� PWM 70 : 20-21, 

// �������ڣ����룩
static int   LOOP_MS = 10;

// ==================== ���ߺ��� ====================
static inline int clampi(int x, int lo, int hi){ return x<lo?lo:(x>hi?hi:x); }
static inline float clampf(float x, float lo, float hi){ return x<lo?lo:(x>hi?hi:x); }
static inline float lerp(float a, float b, float t){ return a + (b-a)*t; }

// ͬ����٣�ǰ����
static inline void set_diff_pwm(int l_pwm, int r_pwm) {
    l_pwm = clampi(l_pwm, 0, 100);
    r_pwm = clampi(r_pwm, 0, 100);
    // ͬ��ǰ��
    digitalWrite(AIN1, 1); digitalWrite(AIN2, 0);
    digitalWrite(BIN1, 1); digitalWrite(BIN2, 0);
    softPwmWrite(PWNA, l_pwm);
    softPwmWrite(PWNB, r_pwm);
}

static inline void motors_stop(void){
    softPwmWrite(PWNA, 0);
    softPwmWrite(PWNB, 0);
    digitalWrite(AIN1, 0); digitalWrite(AIN2, 0);
    digitalWrite(BIN1, 0); digitalWrite(BIN2, 0);
}

// ��ȡ��·������������������ b2b1b0 = L C R
static inline int read_line_bits(void) {
    int L = digitalRead(IR_LEFT);
    int C = digitalRead(IR_CENTER);
    int R = digitalRead(IR_RIGHT);
#ifdef SENSOR_BLACK_IS_ZERO
    L ^= 1; C ^= 1; R ^= 1; // ͳһ�� 1=����
#endif
    return (L<<2) | (C<<1) | (R<<0);
}

// ����/����ӳ�䵽����ɢ��� e��
// Լ����e>0 => ��������Ҫ�����ҿ죨����������
static inline float bits_to_error(int b) {
    switch(b) {
        case 0b010: return  0.0f;  // ���м�
        case 0b110: return +0.5f;  // ��+�У���ƫ��
        case 0b100: return +1.0f;  // ����
        case 0b011: return -0.5f;  // ��+�ң���ƫ�ң�
        case 0b001: return -1.0f;  // ����
        case 0b111: return  0.0f;  // ȫ��/���ߣ�������
        case 0b000: return  9.9f;  // ���߱��
        default:     return  0.0f; // ������ϣ����ؾ���
    }
}

// ==================== �˳����� ====================
static void on_exit_int(int sig){
    (void)sig;
    motors_stop();
    printf("\n[EXIT] stop motors.\n");
    exit(0);
}

// ==================== ������ ====================
int main(void){
    signal(SIGINT, on_exit_int);

    // ��ʼ��
    wiringPiSetup();
    pinMode(PWNA, OUTPUT);
    pinMode(AIN1, OUTPUT);
    pinMode(AIN2, OUTPUT);
    pinMode(PWNB, OUTPUT);
    pinMode(BIN1, OUTPUT);
    pinMode(BIN2, OUTPUT);

    pinMode(IR_LEFT,   INPUT);
    pinMode(IR_CENTER, INPUT);
    pinMode(IR_RIGHT,  INPUT);

    softPwmCreate(PWNA, 0, 100);
    softPwmCreate(PWNB, 0, 100);

    // ״̬����
    float e = 0.0f, e_lp = 0.0f, e_prev = 0.0f, de = 0.0f;
    float v_smooth = (float)BASE_MIN;
    int   last_dir = +1; // +1=��-1=�ң����ڶ���ʱ����������һأ�

    // ���ȶ��ȹ��ƣ����� RMS��
    float rms_acc = 0.0f;
    int   rms_cnt = 0;
    const int RMS_WIN = 20; // Լ 200ms

    // ���߻ָ�
    int    in_recover = 0;
    unsigned int recover_start = 0;

    printf("[INFO] Line follower started.\n");
#ifdef SENSOR_BLACK_IS_ZERO
    printf("[INFO] Sensor mode: 0=BLACK -> flipped to 1=BLACK.\n");
#else
    printf("[INFO] Sensor mode: 1=BLACK.\n");
#endif

    for(;;){
        // 1) ����������תΪ���
        int bits = read_line_bits();
        float e_raw = bits_to_error(bits);

        // 2) ���ߴ���������ԭ�����ߣ��޶���ʱ��
        if (e_raw > 9.0f) {
            if (!in_recover) {
                in_recover = 1;
                recover_start = millis();
            }
            int pwm = RECOVER_PWM;

            // ���������������
            if (last_dir > 0) {
                // �����ǰ�����ַ�ת��������ת��
                digitalWrite(AIN1,0); digitalWrite(AIN2,1);
                digitalWrite(BIN1,1); digitalWrite(BIN2,0);
            } else {
                digitalWrite(AIN1,1); digitalWrite(AIN2,0);
                digitalWrite(BIN1,0); digitalWrite(BIN2,1);
            }
            softPwmWrite(PWNA, pwm);
            softPwmWrite(PWNB, pwm);

            // ��ʱ -> ��ֱ�����˳�����״̬
            if ((int)(millis() - recover_start) > RECOVER_TIMEOUT_MS) {
                digitalWrite(AIN1,1); digitalWrite(AIN2,0);
                digitalWrite(BIN1,1); digitalWrite(BIN2,0);
                softPwmWrite(PWNA, 35);
                softPwmWrite(PWNB, 35);
                delay(120);
                in_recover = 0;
            }
            delay(LOOP_MS);
            continue; // ��һ����
        }

        // �ص�Ѳ��̬
        in_recover = 0;

        // 3) ����ͨ + ����
        e_lp = LP_A*e_lp + (1.0f-LP_A)*e_raw;
        de   = e_lp - e;   // dt�ֳ��� -> �ϲ��� Kd
        e    = e_lp;

        if (e > 0.05f)      last_dir = +1;
        else if (e < -0.05f) last_dir = -1;

        // 4) �ȶ���ָ�꣨�� RMS��
        rms_acc += e*e; rms_cnt++;
        float rms = 0.0f;
        if (rms_cnt >= RMS_WIN) {
            rms = sqrtf(rms_acc / (float)rms_cnt);
            rms_acc = 0.0f; rms_cnt = 0;
        }

        // 5) ��̬���٣�Խ��Խ�죨���� rms + |de|��
        float gain = 1.0f / (1.0f + 1.2f*rms + 0.8f*fabsf(de)); // 0~1
        float base = (float)BASE_MIN + ((float)BASE_MAX - (float)BASE_MIN) * clampf(gain, 0.0f, 1.0f);

        // �ٶ�����
        float dv = base - v_smooth;
        if (dv >  DV_MAX) dv =  DV_MAX;
        if (dv < -DV_MAX) dv = -DV_MAX;
        v_smooth += dv;

        // 6) PD ת�򣨲��٣�
        float turn = Kp*e + Kd*de;
        int l_pwm = (int)lroundf(v_smooth - turn);
        int r_pwm = (int)lroundf(v_smooth + turn);

        set_diff_pwm(l_pwm, r_pwm);

        // ����ѡ�����������ÿ�������ڴ�ӡһ��
        // static int dbg=0; if((++dbg % 20)==0){
        //     printf("bits=%03b e=%+.2f de=%+.2f base=%.1f v=%.1f L=%d R=%d\n",
        //            bits, e, de, base, v_smooth, l_pwm, r_pwm);
        // }

        delay(LOOP_MS);
    }

    return 0;
}
