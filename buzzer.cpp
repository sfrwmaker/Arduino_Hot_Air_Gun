#include "buzzer.h"


void BUZZER::init(void) {
    pinMode(buzzer_pin, OUTPUT);
    noTone(buzzer_pin);
}
