#include <Arduino.h>
#include "display.h"
#include "vars.h"

//------------------------------------------ class lcd DSPLay for soldering IRON -----------------------------
void DSPL::init(void) {
    LiquidCrystal_I2C::begin();
    LiquidCrystal_I2C::clear();
    for (uint8_t i = 0; i < 3; ++i)
        LiquidCrystal_I2C::createChar(i+1, (uint8_t *)custom_symbols[i]);
    full_second_line = false;
    temp_units = 'C';
    backlight();
}

void DSPL::tSet(uint16_t t, bool Celsius) {
    char buff[10];
    if (Celsius) {
        temp_units = 'C';
    } else {
        temp_units = 'F';
    }
    LiquidCrystal_I2C::setCursor(0, 0);
    sprintf(buff, "Set:%3d%c%c", t, (char)1, temp_units);
    LiquidCrystal_I2C::print(buff);
}

void DSPL::tCurr(uint16_t t, bool Celsius) {
    char buff[6];
    LiquidCrystal_I2C::setCursor(0, 1);
    if (t < 1000) {
        sprintf(buff, "%3d%c%c", t, (char)1, Celsius?'C':'F');
    } else {
        LiquidCrystal_I2C::print(F("xxxx"));
        return;
    }
    LiquidCrystal_I2C::print(buff);
    if (full_second_line) {
        LiquidCrystal_I2C::print(F("           "));
        full_second_line = false;
    }
}

void DSPL::tInternal(uint16_t t) {
    char buff[6];
    LiquidCrystal_I2C::setCursor(0, 1);
    if (t < 1023) {
        sprintf(buff, "%4d ", t);
    } else {
        LiquidCrystal_I2C::print(F("xxxx"));
        return;
    }
    LiquidCrystal_I2C::print(buff);
    if (full_second_line) {
        LiquidCrystal_I2C::print(F("           "));
        full_second_line = false;
    }
}

void DSPL::calibReady(bool on) {
	LiquidCrystal_I2C::setCursor(10, 0);
	LiquidCrystal_I2C::print(on?'?':' ');
}

void DSPL::fanSpeed(uint16_t s) {
    char buff[6];
    s = map(s, 0, max_fan_speed, 0, 99);
    sprintf(buff, " %c%2d%c", (char)2, s, '%');
    LiquidCrystal_I2C::setCursor(11, 1);
    LiquidCrystal_I2C::print(buff);
}

void DSPL::appliedPower(uint8_t p, bool show_zero) {
    char buff[6];
    if (p > 99) p = 99;
    LiquidCrystal_I2C::setCursor(5, 1);
    if (p == 0 && !show_zero) {
        LiquidCrystal_I2C::print(F("     "));
    } else {
        sprintf(buff, " %c%2d%c", (char)3, p, '%');
        LiquidCrystal_I2C::print(buff);
    }
}

void DSPL::setupMode(byte mode) {
    LiquidCrystal_I2C::clear();
    LiquidCrystal_I2C::print(F("setup"));
    LiquidCrystal_I2C::setCursor(1,1);
    switch (mode) {
        case 0:                                                             // tip calibrate
            LiquidCrystal_I2C::print(F("calibrate"));
            break;
        case 1:                                                             // tune
            LiquidCrystal_I2C::print(F("tune"));
            break;
        case 2:                                                             // save
            LiquidCrystal_I2C::print(F("save"));
            break;
        case 3:                                                             // cancel
            LiquidCrystal_I2C::print(F("cancel"));
            break;
        case 4:                                                             // set defaults
            LiquidCrystal_I2C::print(F("reset config"));
            break;
        default:
            break;
    }
}

void DSPL::msgON(void) {
    LiquidCrystal_I2C::setCursor(10, 0);
    LiquidCrystal_I2C::print(F("    ON"));
}

void DSPL::msgOFF(void) {
    LiquidCrystal_I2C::setCursor(10, 0);
    LiquidCrystal_I2C::print(F("   OFF"));
}


void DSPL::msgReady(void) {
    LiquidCrystal_I2C::setCursor(10, 0);
    LiquidCrystal_I2C::print(F(" Ready"));
}

void DSPL::msgCold(void) {
    LiquidCrystal_I2C::setCursor(10, 0);
    LiquidCrystal_I2C::print(F("  Cold"));
}

void DSPL::msgFail(void) {
    LiquidCrystal_I2C::setCursor(0, 1);
    LiquidCrystal_I2C::print(F(" -== Failed ==- "));
}

void DSPL::msgTune(void) {
    LiquidCrystal_I2C::setCursor(0, 0);
    LiquidCrystal_I2C::print(F("Tune"));
}

