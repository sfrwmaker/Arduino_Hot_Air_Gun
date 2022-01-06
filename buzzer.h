#ifndef _BUZZER_H_
#define _BUZZER_H_

#include <Arduino.h>
//------------------------------------------ class BUZZER ------------------------------------------------------
class BUZZER {
    public:
        BUZZER(uint8_t buzzerP)             { buzzer_pin = buzzerP; }
        void    init(void);
        void    shortBeep(void)             { tone(buzzer_pin, 3520, 160); }
        void    lowBeep(void)               { tone(buzzer_pin,  880, 160); }
        void    doubleBeep(void)            { tone(buzzer_pin, 3520, 160); delay(300); tone(buzzer_pin, 3520, 160); }
        void    failedBeep(void)            { tone(buzzer_pin, 3520, 160); delay(170);
                                                tone(buzzer_pin,  880, 250); delay(260);
                                                tone(buzzer_pin, 3520, 160);
                                            }
    private:
        uint8_t buzzer_pin;
};

#endif
