//22秒
// line_follow_pd.c
// 树莓派小车：非阻塞巡迹 + 差速PD + 动态调速 + 丢线恢复
// 依赖: wiringPi, softPwm
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <math.h>
#include <wiringPi.h>
#include <softPwm.h>

// ==================== 引脚映射（wiringPi 编号） ====================
#define PWNA   1
#define AIN1   3
#define AIN2   2
#define PWNB   4
#define BIN1   6
#define BIN2   5

#define IR_LEFT   25
#define IR_CENTER 24
#define IR_RIGHT  23

// ==================== 行为参数（按需调整） ====================
// 若你的红外模块是 "黑=0, 白=1"，打开下面的宏做一次逻辑翻转
// #define SENSOR_BLACK_IS_ZERO 1

// PD 增益（以 PWM 百分比为尺度）
static float Kp = 45.0f;
static float Kd = 12.0f;

// 速度上下限（0~100），以及目标速度限坡
static int   BASE_MIN = 50;
static int   BASE_MAX = 70;
static float DV_MAX   = 4.0f;    // 每周期最大速度变化（PWM点）

// 误差一阶低通（0~1，越大越平滑）x
static float LP_A = 0.10f; // 0.1 : 20.5s

// 丢线恢复限制
static int   RECOVER_TIMEOUT_MS = 300; // 单次找线最长时长
static int   RECOVER_PWM        = 80;  // 找线时的 PWM 70 : 20-21, 

// 控制周期（毫秒）
static int   LOOP_MS = 10;

// ==================== 工具函数 ====================
static inline int clampi(int x, int lo, int hi){ return x<lo?lo:(x>hi?hi:x); }
static inline float clampf(float x, float lo, float hi){ return x<lo?lo:(x>hi?hi:x); }
static inline float lerp(float a, float b, float t){ return a + (b-a)*t; }

// 同向差速（前进）
static inline void set_diff_pwm(int l_pwm, int r_pwm) {
    l_pwm = clampi(l_pwm, 0, 100);
    r_pwm = clampi(r_pwm, 0, 100);
    // 同向前进
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

// 读取三路传感器，返回三比特 b2b1b0 = L C R
static inline int read_line_bits(void) {
    int L = digitalRead(IR_LEFT);
    int C = digitalRead(IR_CENTER);
    int R = digitalRead(IR_RIGHT);
#ifdef SENSOR_BLACK_IS_ZERO
    L ^= 1; C ^= 1; R ^= 1; // 统一成 1=黑线
#endif
    return (L<<2) | (C<<1) | (R<<0);
}

// 概率/情形映射到“离散误差 e”
// 约定：e>0 => 线在左，需要左慢右快（向左修正）
static inline float bits_to_error(int b) {
    switch(b) {
        case 0b010: return  0.0f;  // 仅中间
        case 0b110: return +0.5f;  // 左+中（稍偏左）
        case 0b100: return +1.0f;  // 仅左
        case 0b011: return -0.5f;  // 中+右（稍偏右）
        case 0b001: return -1.0f;  // 仅右
        case 0b111: return  0.0f;  // 全黑/粗线：按居中
        case 0b000: return  9.9f;  // 丢线标记
        default:     return  0.0f; // 其他组合：保守居中
    }
}

// ==================== 退出处理 ====================
static void on_exit_int(int sig){
    (void)sig;
    motors_stop();
    printf("\n[EXIT] stop motors.\n");
    exit(0);
}

// ==================== 主程序 ====================
int main(void){
    signal(SIGINT, on_exit_int);

    // 初始化
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

    // 状态变量
    float e = 0.0f, e_lp = 0.0f, e_prev = 0.0f, de = 0.0f;
    float v_smooth = (float)BASE_MIN;
    int   last_dir = +1; // +1=左，-1=右（用于丢线时朝最近方向找回）

    // 简化稳定度估计（滑窗 RMS）
    float rms_acc = 0.0f;
    int   rms_cnt = 0;
    const int RMS_WIN = 20; // 约 200ms

    // 丢线恢复
    int    in_recover = 0;
    unsigned int recover_start = 0;

    printf("[INFO] Line follower started.\n");
#ifdef SENSOR_BLACK_IS_ZERO
    printf("[INFO] Sensor mode: 0=BLACK -> flipped to 1=BLACK.\n");
#else
    printf("[INFO] Sensor mode: 1=BLACK.\n");
#endif

    for(;;){
        // 1) 读传感器并转为误差
        int bits = read_line_bits();
        float e_raw = bits_to_error(bits);

        // 2) 丢线处理（非阻塞原地找线，限定超时）
        if (e_raw > 9.0f) {
            if (!in_recover) {
                in_recover = 1;
                recover_start = millis();
            }
            int pwm = RECOVER_PWM;

            // 依据最近误差方向旋回
            if (last_dir > 0) {
                // 左后右前（左轮反转、右轮正转）
                digitalWrite(AIN1,0); digitalWrite(AIN2,1);
                digitalWrite(BIN1,1); digitalWrite(BIN2,0);
            } else {
                digitalWrite(AIN1,1); digitalWrite(AIN2,0);
                digitalWrite(BIN1,0); digitalWrite(BIN2,1);
            }
            softPwmWrite(PWNA, pwm);
            softPwmWrite(PWNB, pwm);

            // 超时 -> 短直行再退出找线状态
            if ((int)(millis() - recover_start) > RECOVER_TIMEOUT_MS) {
                digitalWrite(AIN1,1); digitalWrite(AIN2,0);
                digitalWrite(BIN1,1); digitalWrite(BIN2,0);
                softPwmWrite(PWNA, 35);
                softPwmWrite(PWNB, 35);
                delay(120);
                in_recover = 0;
            }
            delay(LOOP_MS);
            continue; // 下一周期
        }

        // 回到巡迹态
        in_recover = 0;

        // 3) 误差低通 + 导数
        e_lp = LP_A*e_lp + (1.0f-LP_A)*e_raw;
        de   = e_lp - e;   // dt≈常数 -> 合并到 Kd
        e    = e_lp;

        if (e > 0.05f)      last_dir = +1;
        else if (e < -0.05f) last_dir = -1;

        // 4) 稳定度指标（简化 RMS）
        rms_acc += e*e; rms_cnt++;
        float rms = 0.0f;
        if (rms_cnt >= RMS_WIN) {
            rms = sqrtf(rms_acc / (float)rms_cnt);
            rms_acc = 0.0f; rms_cnt = 0;
        }

        // 5) 动态调速：越稳越快（基于 rms + |de|）
        float gain = 1.0f / (1.0f + 1.2f*rms + 0.8f*fabsf(de)); // 0~1
        float base = (float)BASE_MIN + ((float)BASE_MAX - (float)BASE_MIN) * clampf(gain, 0.0f, 1.0f);

        // 速度限坡
        float dv = base - v_smooth;
        if (dv >  DV_MAX) dv =  DV_MAX;
        if (dv < -DV_MAX) dv = -DV_MAX;
        v_smooth += dv;

        // 6) PD 转向（差速）
        float turn = Kp*e + Kd*de;
        int l_pwm = (int)lroundf(v_smooth - turn);
        int r_pwm = (int)lroundf(v_smooth + turn);

        set_diff_pwm(l_pwm, r_pwm);

        // （可选）调试输出：每若干周期打印一次
        // static int dbg=0; if((++dbg % 20)==0){
        //     printf("bits=%03b e=%+.2f de=%+.2f base=%.1f v=%.1f L=%d R=%d\n",
        //            bits, e, de, base, v_smooth, l_pwm, r_pwm);
        // }

        delay(LOOP_MS);
    }

    return 0;
}
