#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LED1 21
#define LED2 22

void led_init(void)
{
    pinMode(LED1, OUTPUT);
    pinMode(LED2, OUTPUT);
    digitalWrite(LED1, LOW);
    digitalWrite(LED2, LOW);
}

void show_help(void)
{
    printf("\nLED控制:\n");
    printf("1 - LED1亮 LED2灭\n");
    printf("2 - LED1灭 LED2亮\n");
    printf("3 - 两个都亮\n");
    printf("4 - 两个都灭\n");
    printf("a - 自动流水灯\n");
    printf("q - 退出\n");
    printf("选择: ");
}

void auto_flow(void)
{
    printf("自动模式开始...按回车返回\n");
    
    while(1)
    {
        digitalWrite(LED1, HIGH);
        digitalWrite(LED2, LOW);
        printf("状态: LED1● LED2○\r");
        fflush(stdout);
        delay(300);
        
        if(getchar() != EOF) {
            while(getchar() != '\n');
            break;
        }
        
        digitalWrite(LED1, LOW);
        digitalWrite(LED2, HIGH);
        printf("状态: LED1○ LED2●\r");
        fflush(stdout);
        delay(300);
        
        if(getchar() != EOF) {
            while(getchar() != '\n');
            break;
        }
    }
    printf("\n");
}

int main(void)
{
    if(wiringPiSetup() == -1) {
        printf("wiringPi初始化失败!\n");
        return 1;
    }
    
    led_init();
    printf("LED控制程序启动\n");
    
    char input[10];
    
    while(1)
    {
        show_help();
        
        if(fgets(input, sizeof(input), stdin) != NULL)
        {
            input[strcspn(input, "\n")] = 0;
            
            if(strlen(input) == 1)
            {
                switch(input[0])
                {
                    case '1':
                        digitalWrite(LED1, HIGH);
                        digitalWrite(LED2, LOW);
                        printf("LED1亮, LED2灭\n");
                        break;
                    case '2':
                        digitalWrite(LED1, LOW);
                        digitalWrite(LED2, HIGH);
                        printf("LED1灭, LED2亮\n");
                        break;
                    case '3':
                        digitalWrite(LED1, HIGH);
                        digitalWrite(LED2, HIGH);
                        printf("两个LED都亮\n");
                        break;
                    case '4':
                        digitalWrite(LED1, LOW);
                        digitalWrite(LED2, LOW);
                        printf("两个LED都灭\n");
                        break;
                    case 'a':
                    case 'A':
                        auto_flow();
                        break;
                    case 'q':
                    case 'Q':
                        printf("退出程序\n");
                        digitalWrite(LED1, LOW);
                        digitalWrite(LED2, LOW);
                        return 0;
                    default:
                        printf("无效输入\n");
                        break;
                }
            }
            else
            {
                printf("请输入单个字符\n");
            }
        }
        
        delay(100);
    }
    
    return 0;
}
