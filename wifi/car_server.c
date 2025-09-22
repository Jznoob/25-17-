/*
 * 树莓派智能小车 + 云台 WIFI 远程控制
 * 任务24 + 任务25 整合
 * 运行：sudo ./car_server 2001
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <wiringPi.h>
#include <softPwm.h>
#include "pca9685.h"

/* -------------------- 电机引脚定义 -------------------- */
#define PWMA  1   // wiringPi 编号
#define AIN1  3
#define AIN2  2
#define PWMB  4
#define BIN1  6
#define BIN2  5

/* -------------------- 云台舵机定义 -------------------- */
#define PIN_BASE 300
#define SERVO_PAN   0   // 左右
#define SERVO_TILT  1   // 上下
int pan_angle  = 90;
int tilt_angle = 90;

/* -------------------- 初始化 -------------------- */
void car_init(void)
{
    wiringPiSetup();
    pinMode(PWMA, OUTPUT); pinMode(AIN1, OUTPUT); pinMode(AIN2, OUTPUT);
    pinMode(PWMB, OUTPUT); pinMode(BIN1, OUTPUT); pinMode(BIN2, OUTPUT);
    softPwmCreate(PWMA, 0, 100);
    softPwmCreate(PWMB, 0, 100);
}

void servo_init(void)
{
    int fd = pca9685Setup(PIN_BASE, 0x40, 50);
    if (fd < 0) {
        printf("PCA9685 init failed\n");
        exit(1);
    }
    pca9685PWMReset(fd);
}

/* -------------------- 小车运动 -------------------- */
void car_forward(int speed)
{
    digitalWrite(AIN1, 1); digitalWrite(AIN2, 0); softPwmWrite(PWMA, speed);
    digitalWrite(BIN1, 1); digitalWrite(BIN2, 0); softPwmWrite(PWMB, speed);
}
void car_back(int speed)
{
    digitalWrite(AIN1, 0); digitalWrite(AIN2, 1); softPwmWrite(PWMA, speed);
    digitalWrite(BIN1, 0); digitalWrite(BIN2, 1); softPwmWrite(PWMB, speed);
}
void car_left(int speed)
{
    digitalWrite(AIN1, 0); digitalWrite(AIN2, 1); softPwmWrite(PWMA, speed);
    digitalWrite(BIN1, 1); digitalWrite(BIN2, 0); softPwmWrite(PWMB, speed);
}
void car_right(int speed)
{
    digitalWrite(AIN1, 1); digitalWrite(AIN2, 0); softPwmWrite(PWMA, speed);
    digitalWrite(BIN1, 0); digitalWrite(BIN2, 1); softPwmWrite(PWMB, speed);
}
void car_stop(void)
{
    softPwmWrite(PWMA, 0); softPwmWrite(PWMB, 0);
}

/* -------------------- 云台控制 -------------------- */
void set_angle(int ch, int *angle, int inc)
{
    *angle += inc;
    if (*angle < 0)   *angle = 0;
    if (*angle > 180) *angle = 180;
    int tick = (int)(0.5 * 4096 / 20.0 + (*angle / 180.0) * (2.5 - 0.5) * 4096 / 20.0 + 0.5);
    pwmWrite(PIN_BASE + ch, tick);
}

/* -------------------- TCP 服务器 -------------------- */
int main(int argc, char *argv[])
{
    if (argc != 2) {
        printf("Usage: %s <port>\n", argv[0]);
        return 1;
    }

    int port = atoi(argv[1]);
    int server_fd, client_fd;
    struct sockaddr_in addr;
    char buf[32];

    car_init();
    servo_init();

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) { perror("socket"); return 1; }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind"); return 1;
    }

    if (listen(server_fd, 5) < 0) {
        perror("listen"); return 1;
    }

    printf("Server listening on port %d...\n", port);

    while (1) {
        client_fd = accept(server_fd, NULL, NULL);
        if (client_fd < 0) { perror("accept"); continue; }

        while (1) {
            int len = recv(client_fd, buf, sizeof(buf) - 1, 0);
            if (len <= 0) break;
            buf[len] = '\0';

            for (int i = 0; i < len; ++i) {
                switch (buf[i]) {
                    case 'A': car_forward(50); break;
                    case 'B': car_back(50);    break;
                    case 'C': car_left(50);    break;
                    case 'D': car_right(50); break;
                    case 'E': car_stop();      break;
                    case 'L': set_angle(SERVO_PAN,  &pan_angle,  -10); break;
                    case 'I': set_angle(SERVO_PAN,  &pan_angle,  +10); break;
                    case 'K': set_angle(SERVO_TILT, &tilt_angle, +10); break;
                    case 'J': set_angle(SERVO_TILT, &tilt_angle, -10); break;
                    case '0':
                        pan_angle = tilt_angle = 90;
                        set_angle(SERVO_PAN,  &pan_angle,  0);
                        set_angle(SERVO_TILT, &tilt_angle, 0);
                        break;
                    default: break;
                }
            }
        }
        close(client_fd);
    }
    return 0;
}