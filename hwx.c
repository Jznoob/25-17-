#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wiringPi.h>
#include <lirc/lirc_client.h>

// 电机引脚配置（根据你实际的接线来修改）
#define motor1_pin1 0   // Motor1 forward
#define motor1_pin2 1   // Motor1 backward
#define motor2_pin1 2   // Motor2 forward
#define motor2_pin2 3   // Motor2 backward
#define speed_pin1 4    // Motor1 speed (PWM)
#define speed_pin2 5    // Motor2 speed (PWM)

// 定义遥控器按键对应的键名
char *keymap[21] = {
    "KEY_CHANNELDOWN", 
    "KEY_CHANNEL", 
    "KEY_CHANNELUP", 
    "KEY_PREVIOUS", 
    "KEY_NEXT", 
    "KEY_PLAYPAUSE", 
    "KEY_VOLUMEDOWN", 
    "KEY_VOLUMEUP", 
    "KEY_EQUAL", 
    "KEY_NUMERIC_0", 
    "BTN_0", 
    "BTN_1", 
    "KEY_NUMERIC_1", 
    "KEY_NUMERIC_2", 
    "KEY_NUMERIC_3", 
    "KEY_NUMERIC_4", 
    "KEY_NUMERIC_5", 
    "KEY_NUMERIC_6", 
    "KEY_NUMERIC_7", 
    "KEY_NUMERIC_8", 
    "KEY_NUMERIC_9"
};

// 控制小车前进
void t_up(int speed, int time) {
    printf("Moving forward with speed %d for %d ms\n", speed, time);
    // 设置电机方向，前进
    digitalWrite(motor1_pin1, HIGH);   // 电机1前进
    digitalWrite(motor1_pin2, LOW);    // 电机1前进
    digitalWrite(motor2_pin1, HIGH);   // 电机2前进
    digitalWrite(motor2_pin2, LOW);    // 电机2前进

    // 使用 PWM 控制电机速度
    pwmWrite(speed_pin1, speed); // 设置电机1的速度
    pwmWrite(speed_pin2, speed); // 设置电机2的速度

    // 停止时间
    delay(time);

    // 停止电机
    digitalWrite(motor1_pin1, LOW);  // 停止电机1
    digitalWrite(motor1_pin2, LOW);  // 停止电机1
    digitalWrite(motor2_pin1, LOW);  // 停止电机2
    digitalWrite(motor2_pin2, LOW);  // 停止电机2
}

// 控制小车后退
void t_down(int speed, int time) {
    // 设置电机方向，后退
    digitalWrite(motor1_pin1, LOW);    // 电机1后退
    digitalWrite(motor1_pin2, HIGH);   // 电机1后退
    digitalWrite(motor2_pin1, LOW);    // 电机2后退
    digitalWrite(motor2_pin2, HIGH);   // 电机2后退

    // 使用 PWM 控制电机速度
    pwmWrite(speed_pin1, speed);  // 设置电机1的速度
    pwmWrite(speed_pin2, speed);  // 设置电机2的速度

    // 停止时间
    delay(time);

    // 停止电机
    digitalWrite(motor1_pin1, LOW);  // 停止电机1
    digitalWrite(motor1_pin2, LOW);  // 停止电机1
    digitalWrite(motor2_pin1, LOW);  // 停止电机2
    digitalWrite(motor2_pin2, LOW);  // 停止电机2
}

// 控制小车左转
void t_left(int speed, int time) {
    // 设置电机方向，左转
    digitalWrite(motor1_pin1, LOW);   // 电机1后退
    digitalWrite(motor1_pin2, HIGH);  // 电机1后退
    digitalWrite(motor2_pin1, HIGH);  // 电机2前进
    digitalWrite(motor2_pin2, LOW);   // 电机2前进

    // 使用 PWM 控制电机速度
    pwmWrite(speed_pin1, speed);  // 设置电机1的速度
    pwmWrite(speed_pin2, speed);  // 设置电机2的速度

    // 停止时间
    delay(time);

    // 停止电机
    digitalWrite(motor1_pin1, LOW);  // 停止电机1
    digitalWrite(motor1_pin2, LOW);  // 停止电机1
    digitalWrite(motor2_pin1, LOW);  // 停止电机2
    digitalWrite(motor2_pin2, LOW);  // 停止电机2
}

// 控制小车右转
void t_right(int speed, int time) {
    // 设置电机方向，右转
    digitalWrite(motor1_pin1, HIGH);  // 电机1前进
    digitalWrite(motor1_pin2, LOW);   // 电机1前进
    digitalWrite(motor2_pin1, LOW);   // 电机2后退
    digitalWrite(motor2_pin2, HIGH);  // 电机2后退

    // 使用 PWM 控制电机速度
    pwmWrite(speed_pin1, speed);  // 设置电机1的速度
    pwmWrite(speed_pin2, speed);  // 设置电机2的速度

    // 停止时间
    delay(time);

    // 停止电机
    digitalWrite(motor1_pin1, LOW);  // 停止电机1
    digitalWrite(motor1_pin2, LOW);  // 停止电机1
    digitalWrite(motor2_pin1, LOW);  // 停止电机2
    digitalWrite(motor2_pin2, LOW);  // 停止电机2
}

// 停止小车
void t_stop(int time) {
    // 停止电机
    digitalWrite(motor1_pin1, LOW);  // 停止电机1
    digitalWrite(motor1_pin2, LOW);  // 停止电机1
    digitalWrite(motor2_pin1, LOW);  // 停止电机2
    digitalWrite(motor2_pin2, LOW);  // 停止电机2

    // 打印停止信息
    printf("Stopping for %d ms\n", time);

    // 停止时间
    delay(time);
}

// 根据按键执行相应的动作
void ircontrol(char *code) {
    printf("执行按键%s\n", code);
    if (strstr(code, keymap[1])) {   // 前进
        t_up(50, 1000);
    } else if (strstr(code, keymap[7])) {   // 后退
        t_down(50, 1000);
    } else if (strstr(code, keymap[3])) {   // 左转
        t_left(50, 1000);
    } else if (strstr(code, keymap[5])) {   // 右转
        t_right(50, 1000);
    } else if (strstr(code, keymap[4])) {   // 停止
        t_stop(10);
    }
}

int main(void) {
    printf("开始执行hwx\n");
    struct lirc_config *config;
    int buttonTimer = millis();  // 用于记录按键时间
    char *code;
    char *c;

    // 初始化wiringPi
    if (wiringPiSetup() == -1) {
        printf("setup wiringPi failed !\n");
        return 1;
    }

    // 设置电机控制引脚
    pinMode(motor1_pin1, OUTPUT);
    pinMode(motor1_pin2, OUTPUT);
    pinMode(motor2_pin1, OUTPUT);
    pinMode(motor2_pin2, OUTPUT);
    pinMode(speed_pin1, PWM_OUTPUT);
    pinMode(speed_pin2, PWM_OUTPUT);


    // 初始化LIRC
    if (lirc_init("lirc", 1) == -1) {
        printf("初始化LIRC失败\n");
        exit(EXIT_FAILURE);
    }

    // 读取lirc配置文件
    if (lirc_readconfig(NULL, &config, NULL) == 0) {
        // 无限循环，等待红外信号
        printf("3\n");
        while (lirc_nextcode(&code) == 0) {
            printf("1\n");
            if (code == NULL) continue;

            // 检查按键时间间隔，避免连续按键过快
            if (millis() - buttonTimer > 400) {
                printf("2\n");
                ircontrol(code);  // 调用ircontrol函数
                buttonTimer = millis();  // 更新时间
            }

            // 释放内存
            free(code);
        }

        // 释放配置资源
        lirc_freeconfig(config);
    }

    // 反初始化LIRC
    lirc_deinit();

    exit(EXIT_SUCCESS);
    return 0;
}
