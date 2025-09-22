#include <stdio.h>
#include <wiringPi.h>

int main(void)
{
    wiringPiSetup();
    
    int pin = 0;
    int t = 70;
    
    pinMode(pin, OUTPUT);
    
    char *morse_code = "01 1000 1010 100 0 0010 110 0000";
    char *p = morse_code;
    
    while(*p != '\0')
    {
        switch(*p)
        {
            case '0':
                digitalWrite(pin, LOW);
                delay(t);
                digitalWrite(pin, HIGH);
                delay(t);
                break;
                
            case '1':
                digitalWrite(pin, LOW);
                delay(t * 3);
                digitalWrite(pin, HIGH);
                delay(t);
                break;
                
            case ' ':
                delay(t * 2);
                break;
        }
        p++;
    }
    
    return 0;
}
