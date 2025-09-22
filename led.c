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
    printf("\nLED����:\n");
    printf("1 - LED1�� LED2��\n");
    printf("2 - LED1�� LED2��\n");
    printf("3 - ��������\n");
    printf("4 - ��������\n");
    printf("a - �Զ���ˮ��\n");
    printf("q - �˳�\n");
    printf("ѡ��: ");
}

void auto_flow(void)
{
    printf("�Զ�ģʽ��ʼ...���س�����\n");
    
    while(1)
    {
        digitalWrite(LED1, HIGH);
        digitalWrite(LED2, LOW);
        printf("״̬: LED1�� LED2��\r");
        fflush(stdout);
        delay(300);
        
        if(getchar() != EOF) {
            while(getchar() != '\n');
            break;
        }
        
        digitalWrite(LED1, LOW);
        digitalWrite(LED2, HIGH);
        printf("״̬: LED1�� LED2��\r");
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
        printf("wiringPi��ʼ��ʧ��!\n");
        return 1;
    }
    
    led_init();
    printf("LED���Ƴ�������\n");
    
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
                        printf("LED1��, LED2��\n");
                        break;
                    case '2':
                        digitalWrite(LED1, LOW);
                        digitalWrite(LED2, HIGH);
                        printf("LED1��, LED2��\n");
                        break;
                    case '3':
                        digitalWrite(LED1, HIGH);
                        digitalWrite(LED2, HIGH);
                        printf("����LED����\n");
                        break;
                    case '4':
                        digitalWrite(LED1, LOW);
                        digitalWrite(LED2, LOW);
                        printf("����LED����\n");
                        break;
                    case 'a':
                    case 'A':
                        auto_flow();
                        break;
                    case 'q':
                    case 'Q':
                        printf("�˳�����\n");
                        digitalWrite(LED1, LOW);
                        digitalWrite(LED2, LOW);
                        return 0;
                    default:
                        printf("��Ч����\n");
                        break;
                }
            }
            else
            {
                printf("�����뵥���ַ�\n");
            }
        }
        
        delay(100);
    }
    
    return 0;
}
