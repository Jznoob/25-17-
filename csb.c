// 单文件实现：PCA9685 + 舵机 + 超声波 + 电机 + 简单避障
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/time.h>
#include <math.h>

#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <softPwm.h>

// -------------------- 超声波脚位（wiringPi 编号） --------------------
#define Trig 28     // 依据教材：Trig/Echo 接 O21 → wiringPi 28/29
#define Echo 29

// -------------------- 电机脚位（示例，wiringPi 1..6） -----------------
int PWMA = 1,  AIN2 = 2,  AIN1 = 3;
int PWMB = 4,  BIN2 = 5,  BIN1 = 6;

// -------------------- PCA9685 寄存器/常量 ----------------------------
#define PCA9685_ADDR      0x40
#define PCA9685_MODE1     0x00
#define PCA9685_MODE2     0x01
#define PCA9685_SUBADR1   0x02
#define PCA9685_SUBADR2   0x03
#define PCA9685_SUBADR3   0x04
#define PCA9685_PRESCALE  0xFE
#define PCA9685_LED0_ON_L 0x06
#define PCA9685_ALL_ON_L  0xFA

// MODE1 bits
#define MODE1_RESTART 0x80
#define MODE1_AI      0x20
#define MODE1_SLEEP   0x10

// MODE2 bits
#define MODE2_OUTDRV  0x04  // totem-pole

// PCA9685 固定时钟
#define PCA9685_OSC_HZ 25000000.0
#define PCA9685_MAX_TICK 4096

// 舵机参数（50Hz，0.5~2.5ms）
#define SERVO_FREQ_HZ  50.0
#define SERVO_MIN_MS   0.5f
#define SERVO_MAX_MS   2.5f

// 舵机通道
#define SERVO_CH 0

// -------------------- PCA9685 低级驱动（无外部 .h） -------------------
typedef struct {
    int fd;
    float freq_hz;
} PCA9685;

static int pca9685_write8(int fd, int reg, int val) {
    return wiringPiI2CWriteReg8(fd, reg, val & 0xFF);
}
static int pca9685_read8(int fd, int reg) {
    return wiringPiI2CReadReg8(fd, reg) & 0xFF;
}
static int pca9685_set_pwm_raw(int fd, int ch, int on_tick, int off_tick) {
    int base = PCA9685_LED0_ON_L + 4*ch;
    pca9685_write8(fd, base + 0, on_tick & 0xFF);
    pca9685_write8(fd, base + 1, (on_tick >> 8) & 0x0F);
    pca9685_write8(fd, base + 2, off_tick & 0xFF);
    pca9685_write8(fd, base + 3, (off_tick >> 8) & 0x0F);
    return 0;
}

// 频率设置：prescale = round(25MHz/(4096*freq)) - 1
static void pca9685_set_freq(int fd, float freq_hz) {
    if (freq_hz < 40)  freq_hz = 40;
    if (freq_hz > 1000) freq_hz = 1000;
    float prescale_f = (PCA9685_OSC_HZ / (PCA9685_MAX_TICK * freq_hz)) - 1.0f;
    int prescale = (int)floor(prescale_f + 0.5f);

    int oldmode = pca9685_read8(fd, PCA9685_MODE1);
    int sleepmode = (oldmode & ~MODE1_RESTART) | MODE1_SLEEP;   // 进 SLEEP
    pca9685_write8(fd, PCA9685_MODE1, sleepmode);
    pca9685_write8(fd, PCA9685_PRESCALE, prescale);
    // 退出 SLEEP，开启自动递增
    pca9685_write8(fd, PCA9685_MODE1, MODE1_AI);
    usleep(5000);
    // 输出设置：推挽
    pca9685_write8(fd, PCA9685_MODE2, MODE2_OUTDRV);
    usleep(5000);
}

// 初始化并清零
static PCA9685 pca9685_begin(int i2c_addr, float freq_hz) {
    PCA9685 dev = { .fd = -1, .freq_hz = freq_hz };
    int fd = wiringPiI2CSetup(i2c_addr);
    if (fd < 0) return dev;
    // 自动递增 + 频率
    pca9685_write8(fd, PCA9685_MODE1, MODE1_AI);
    pca9685_set_freq(fd, freq_hz);
    // 关闭全部输出
    pca9685_write8(fd, PCA9685_ALL_ON_L+0, 0);
    pca9685_write8(fd, PCA9685_ALL_ON_L+1, 0);
    pca9685_write8(fd, PCA9685_ALL_ON_L+2, 0);
    pca9685_write8(fd, PCA9685_ALL_ON_L+3, 0);
    dev.fd = fd;
    return dev;
}

// 将毫秒脉宽转换为 tick
static inline int ms_to_tick(float ms, float freq_hz) {
    float period_ms = 1000.0f / freq_hz;
    int tick = (int)floor((ms / period_ms) * PCA9685_MAX_TICK + 0.5f);
    if (tick < 0) tick = 0;
    if (tick > 4095) tick = 4095;
    return tick;
}

// 角度(0~180) → 0.5~2.5ms → tick
static void servo_write_angle(PCA9685 *dev, int ch, float angle_deg) {
    if (angle_deg < 0) angle_deg = 0;
    if (angle_deg > 180) angle_deg = 180;
    float ms = SERVO_MIN_MS + (angle_deg/180.0f)*(SERVO_MAX_MS - SERVO_MIN_MS);
    int off = ms_to_tick(ms, dev->freq_hz);
    pca9685_set_pwm_raw(dev->fd, ch, 0, off);
}

// -------------------- 电机控制（TB6612 逻辑） -------------------------
static inline void t_stop(unsigned t_ms){
    digitalWrite(AIN2,0); digitalWrite(AIN1,0); softPwmWrite(PWMA,0);
    digitalWrite(BIN2,0); digitalWrite(BIN1,0); softPwmWrite(PWMB,0);
    delay(t_ms);
}
static inline void t_up(unsigned speed, unsigned t_ms){
    digitalWrite(AIN2,0); digitalWrite(AIN1,1); softPwmWrite(PWMA,speed);
    digitalWrite(BIN2,0); digitalWrite(BIN1,1); softPwmWrite(PWMB,speed);
    delay(t_ms);
}
static inline void t_down(unsigned speed, unsigned t_ms){
    digitalWrite(AIN2,1); digitalWrite(AIN1,0); softPwmWrite(PWMA,speed);
    digitalWrite(BIN2,1); digitalWrite(BIN1,0); softPwmWrite(PWMB,speed);
    delay(t_ms);
}
static inline void t_left(unsigned speed, unsigned t_ms){
    digitalWrite(AIN2,1); digitalWrite(AIN1,0); softPwmWrite(PWMA,speed);
    digitalWrite(BIN2,0); digitalWrite(BIN1,1); softPwmWrite(PWMB,speed);
    delay(t_ms);
}
static inline void t_right(unsigned speed, unsigned t_ms){
    digitalWrite(AIN2,0); digitalWrite(AIN1,1); softPwmWrite(PWMA,speed);
    digitalWrite(BIN2,1); digitalWrite(BIN1,0); softPwmWrite(PWMB,speed);
    delay(t_ms);
}

// -------------------- 超声波：初始化 + 测距 ---------------------------
static inline void ultraInit(void){
    pinMode(Echo, INPUT);
    pinMode(Trig, OUTPUT);
    digitalWrite(Trig, LOW);
}

// 单次测距（返回微秒；含超时）
static float disOnce_us(void){
    struct timeval t1, t2;
    long start, stop;
    // 触发 ≥10us
    digitalWrite(Trig, LOW);  delayMicroseconds(2);
    digitalWrite(Trig, HIGH); delayMicroseconds(12);
    digitalWrite(Trig, LOW);

    long guard = 30000; // 30ms 等待 Echo=1
    while(digitalRead(Echo)==0 && guard-->0) delayMicroseconds(1);
    if (guard<=0) return -1;

    gettimeofday(&t1, NULL);
    guard = 30000; // 最远测距~4m，给 30ms 余量
    while(digitalRead(Echo)==1 && guard-->0) delayMicroseconds(1);
    gettimeofday(&t2, NULL);
    if (guard<=0) return -1;

    start = (long)t1.tv_sec*1000000 + t1.tv_usec;
    stop  = (long)t2.tv_sec*1000000 + t2.tv_usec;
    return (float)(stop - start);
}

static float disMeasure_cm(void){
    // 三次中值滤波
    float v[3] = { -1,-1,-1 };
    for(int i=0;i<3;i++){
        float us = disOnce_us();
        v[i] = (us < 0) ? 1e9 : us;
        delay(10);
    }
    // 排序
    for(int i=0;i<3;i++) for(int j=i+1;j<3;j++) if(v[j]<v[i]){ float t=v[i]; v[i]=v[j]; v[j]=t; }
    float us = (v[1] >= 1e8) ? -1 : v[1];
    if (us < 0) return -1;
    // d = 340 * t / 2（t: 秒；340m/s → 34000 cm/s）
    return (us/1e6f)*34000.0f/2.0f;
}

// 在目标角度上测距（舵机就位延时）
static float measure_at(PCA9685 *dev, float angle){
    servo_write_angle(dev, SERVO_CH, angle);
    delay(400);
    return disMeasure_cm();
}

// -------------------- 主程序：扫描-避障 -------------------------------
int main(void){
    if (wiringPiSetup() < 0) { fprintf(stderr,"wiringPi init failed\n"); return 1; }

    // 电机 GPIO & PWM
    pinMode (PWMA, OUTPUT);  pinMode (AIN2, OUTPUT);  pinMode (AIN1, OUTPUT);
    pinMode (PWMB, OUTPUT);  pinMode (BIN2, OUTPUT);  pinMode (BIN1, OUTPUT);
    softPwmCreate(PWMA, 0, 100);
    softPwmCreate(PWMB, 0, 100);

    // 超声波
    ultraInit();

    // PCA9685 舵机
    PCA9685 p = pca9685_begin(PCA9685_ADDR, SERVO_FREQ_HZ);
    if (p.fd < 0) { fprintf(stderr,"PCA9685 not found @0x40\n"); return 2; }

    // 舵机回中
    servo_write_angle(&p, SERVO_CH, 90);
    delay(300);

    const float TH = 30.0f; // 阈值（cm）
    while(1){
        float df = measure_at(&p, 90);
        printf("[Front] %.1f cm\n", df);

        if (df > 0 && df < TH){
            float dl = measure_at(&p, 175);
            float dr = measure_at(&p, 5);
            printf("  [Left] %.1f  [Right] %.1f\n", dl, dr);

            t_down(50, 400);                 // 退避
            if (dl >= dr) t_left(50, 350);   // 择优转向
            else           t_right(50, 350);
            t_stop(100);
        }else{
            t_up(50, 0);                      // 前进
            servo_write_angle(&p, SERVO_CH, 90);
            delay(30);
        }
    }
    return 0;
}

