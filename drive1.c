#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <wiringPi.h>
#include <softPwm.h>

#define PWMA 1
#define AIN2 2
#define AIN1 3
#define PWMB 4
#define BIN2 5
#define BIN1 6

int getch(void)
{
    struct termios oldt, newt;
    int ch;
    
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    
    return ch;
}

void move_forward(unsigned int speed)
{
    digitalWrite(AIN2, 0);
    digitalWrite(AIN1, 1);
    softPwmWrite(PWMA, speed);
    
    digitalWrite(BIN2, 0);
    digitalWrite(BIN1, 1);
    softPwmWrite(PWMB, speed);
}

void move_backward(unsigned int speed)
{
    digitalWrite(AIN2, 1);
    digitalWrite(AIN1, 0);
    softPwmWrite(PWMA, speed);
    
    digitalWrite(BIN2, 1);
    digitalWrite(BIN1, 0);
    softPwmWrite(PWMB, speed);
}

void turn_left(unsigned int speed)
{
    digitalWrite(AIN2, 1);
    digitalWrite(AIN1, 0);
    softPwmWrite(PWMA, speed);
    
    digitalWrite(BIN2, 0);
    digitalWrite(BIN1, 1);
    softPwmWrite(PWMB, speed);
}

void turn_right(unsigned int speed)
{
    digitalWrite(AIN2, 0);
    digitalWrite(AIN1, 1);
    softPwmWrite(PWMA, speed);
    
    digitalWrite(BIN2, 1);
    digitalWrite(BIN1, 0);
    softPwmWrite(PWMB, speed);
}

void stop_motors()
{
    digitalWrite(AIN2, 0);
    digitalWrite(AIN1, 0);
    softPwmWrite(PWMA, 0);
    
    digitalWrite(BIN2, 0);
    digitalWrite(BIN1, 0);
    softPwmWrite(PWMB, 0);
}

void show_controls()
{
    printf("\nCar Control System\n");
    printf("w - Move Forward\n");
    printf("s - Move Backward\n");
    printf("a - Turn Left\n");
    printf("d - Turn Right\n");
    printf("space - Stop\n");
    printf("q - Quit\n");
    printf("Press key to move, release to stop\n");
}

int main()
{
    if (wiringPiSetup() == -1) {
        printf("Setup wiringPi failed\n");
        return 1;
    }
    
    pinMode(PWMA, OUTPUT);
    pinMode(AIN2, OUTPUT);
    pinMode(AIN1, OUTPUT);
    pinMode(PWMB, OUTPUT);
    pinMode(BIN2, OUTPUT);
    pinMode(BIN1, OUTPUT);
    
    softPwmCreate(PWMA, 0, 100);
    softPwmCreate(PWMB, 0, 100);
    
    printf("Car control system started\n");
    show_controls();
    
    int speed = 50;
    char command;
    
    while(1) {
        command = getch();
        
        switch(command) {
            case 'w':
                printf("Moving Forward\n");
                move_forward(speed);
                break;
            case 's':
                printf("Moving Backward\n");
                move_backward(speed);
                break;
            case 'a':
                printf("Turning Left\n");
                turn_left(speed);
                break;
            case 'd':
                printf("Turning Right\n");
                turn_right(speed);
                break;
            case ' ':
                printf("Stopped\n");
                stop_motors();
                break;
            case 'q':
                printf("Exiting program\n");
                stop_motors();
                return 0;
            default:
                stop_motors();
        }
        
        while(getch() != command);
        stop_motors();
    }
    
    return 0;
}
