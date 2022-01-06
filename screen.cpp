#include <Arduino.h>
#include "screen.h"

//---------------------------------------- class mainSCREEN [the hot air gun is OFF] ---------------------------

void mainSCREEN::init(void) {
    pHG->switchPower(false);
    uint16_t temp_set   = pHG->presetTemp();
    uint16_t tempH      = pCfg->tempHuman(temp_set);                        // The preset temperature in the human readable units
    pEnc->reset(tempH, temp_minC, temp_maxC, 1, 5);
    used = !pHG->isCold();
    cool_notified = !used;
    if (used) {                                                             // the hot gun was used, we should save new data in EEPROM
        pCfg->save(temp_set, pHG->presetFan());
    }
    mode_change = 0;														// Start adjusting temperature mode
    clear_used_ms = 0;
    pD->clear();
    forceRedraw();
}

void mainSCREEN::rotaryValue(int16_t value) {
    if (mode_change == 0) {                                         		// set hot gun temperature
        uint16_t temp = pCfg->tempInternal(value);
        pHG->setTemp(temp);
        pD->tSet(value);
    } else {                                                                // set fan speed
        pHG->setFan(value);
        pD->fanSpeed(value);
        mode_change = millis() + fan_adjust_to;
    }
    update_screen  = millis() + period;
}

SCREEN* mainSCREEN::show(void) {
    if (millis() < update_screen) return this;
    update_screen = millis() + period;

    if (clear_used_ms && (millis() > clear_used_ms)) {
        clear_used_ms = 0;
        used = false;
        pD->msgOFF();
    }

    if (mode_change && millis() > mode_change)								// Return to temperature adjustment mode
    	menu();

    uint16_t temp_set = pHG->presetTemp();
    pD->tSet(pCfg->tempHuman(temp_set));
    uint16_t temp  = pHG->averageTemp();
    uint16_t tempH = pCfg->tempHuman(temp);
    if (pHG->isCold()) {
        if (used) {
            pD->msgCold();
        } else {
            pD->msgOFF();
        }
        if (used && !cool_notified) {
            pBz->shortBeep();
            cool_notified = true;
            clear_used_ms = millis() + cool_notify_period;
            pD->fanSpeed(pHG->fanSpeed());
        }
    } else {
        pD->msgOFF();
    }
    pD->tCurr(tempH);
    pD->appliedPower(0, false);
    pD->fanSpeed(pHG->fanSpeed());
    return this;
}

SCREEN* mainSCREEN::menu(void) {
    if (mode_change == 0) {                                             	// Prepare to adjust the fan speed
        uint8_t fs = pHG->presetFan();
        pEnc->reset(fs, min_fan_speed, max_fan_speed, 5, 20);
        mode_change = millis() + fan_adjust_to;
Serial.println("Fan mode");
    } else {                                                                // Prepare to adjust the preset temperature
        uint16_t temp_set   = pHG->presetTemp();
        uint16_t tempH      = pCfg->tempHuman(temp_set);
        pEnc->reset(tempH, temp_minC, temp_maxC, 1, 5);
        mode_change = 0;
Serial.println("Temp mode");
    }
    return this;
}

SCREEN* mainSCREEN::reedSwitch(bool on) {
    if (on && this->on)
        return this->on;
    return this; 
}

//---------------------------------------- class workSCREEN [the hot air gun is ON] ----------------------------
void workSCREEN::init(void) {
	uint16_t temp_set   = pHG->presetTemp();
	uint16_t tempH      = pCfg->tempHuman(temp_set);
	pEnc->reset(tempH, temp_minC, temp_maxC, 1, 5);							// By default adjust the temperature
	mode_change	= 0;
    pHG->switchPower(true);
    ready = false;
    pD->clear();
    forceRedraw();
}

void workSCREEN::rotaryValue(int16_t value) {                               // Setup new preset temperature by rotating the encoder
    if (mode_change == 0) {													// Adjust temperature
        ready = false;
        uint16_t temp = pCfg->tempInternal(value);                          // Translate human readable temperature into internal value
        pHG->setTemp(temp);
        pD->tSet(value);
    } else {																// Adjust fan speed
        pHG->setFan(value);
        pD->fanSpeed(value);
        mode_change = millis() + fan_adjust_to;
    }
    update_screen = millis() + period;
}

SCREEN* workSCREEN::show(void) {
    if (millis() < update_screen) return this;
    update_screen = millis() + period;

    if (mode_change && millis() > mode_change)								// Return to the temperature adjustment mode
    	menu();
    int temp_set  = pHG->presetTemp();
    int tempH_set = pCfg->tempHuman(temp_set);
    pD->tSet(tempH_set);
    int temp      = pHG->averageTemp();
    int tempH     = pCfg->tempHuman(temp);
    pD->tCurr(tempH);
    pD->msgON();
    uint8_t p   = pHG->appliedPower();
    pD->appliedPower(p);
    pD->fanSpeed(pHG->fanSpeed());

    if ((abs(temp_set - temp) < 5) && (pHG->tempDispersion() <= 60))  {
        if (!ready) {
            pBz->shortBeep();
            ready = true;
            pD->msgReady();
            update_screen = millis() + (period << 2);
            return this;
        }
    }
    return this;
}

SCREEN* workSCREEN::menu(void) {
    if (mode_change == 0) {														// Adjusting temperature, activate fan adjust
        uint16_t    fs = pHG->fanSpeed();
        pEnc->reset(fs, min_fan_speed, max_fan_speed, 5, 20);
        mode_change = millis() + fan_adjust_to;
    } else {
        uint16_t temp_set   = pHG->presetTemp();
        uint16_t tempH      = pCfg->tempHuman(temp_set);
        pEnc->reset(tempH, temp_minC, temp_maxC, 1, 5);
        mode_change = 0;
    }
    return this;
}

SCREEN* workSCREEN::reedSwitch(bool on) {
    if (!on && next)
        return next;
    return this; 
}

//---------------------------------------- class configSCREEN [configuration menu] -----------------------------
void configSCREEN::init(void) {
    pHG->switchPower(false);
    mode = 0;
    pEnc->reset(mode, 0, 4, 1, 0, true);          
    pD->clear();
    pD->setupMode(0);
    this->scr_timeout = 30;                                                 // This variable is defined in the superclass
}

SCREEN* configSCREEN::show(void) {
    if (millis() < update_screen) return this;
    update_screen = millis() + period;
    pD->setupMode(mode);
    return this;
}

SCREEN* configSCREEN::menu(void) {
    switch (mode) {
        case 0:                                                             // calibrate hotgun
            if (calib) return calib;
            break;
        case 1:                                                             // Tune potentiometer
            if (tune) return tune;
            break;
        case 2:                                                             // Save configuration data
            menu_long();
            break;
        case 3:                                                             // Cancel, Return to the main menu
            if (next) return next;
            break;
        case 4:                                                             // Save defaults
            pCfg->setDefaults(true);
            if (next) return next;
            break;
    }
    forceRedraw();
    return this;
}

void configSCREEN::rotaryValue(int16_t value) {
    mode = value;
    forceRedraw();
}

//---------------------------------------- class calibSCREEN [ Manual Hot Air Gun Calibration ] ----------------
void calibSCREEN::init(void) {
    pHG->switchPower(false);
    ref_temp_index 		= 0;
    pEnc->reset(ref_temp_index, 0, 2, 1, 1, true);							// Select reference temperature point using Encoder
    pCfg->getCalibrationData(calib_temp);									// Load Current calibration data
    pHG->setFan(tune_fan_speed);
    ref_temp_index 		= 0;
    ready				= false;
    tuning				= false;
    temp_setready_ms	= 0;
    update_screen		= 0;
    pD->clear();
    pD->msgOFF();
    pD->fanSpeed(tune_fan_speed);
    pD->tSet(temp_tip[ref_temp_index]);
    forceRedraw();
}

SCREEN* calibSCREEN::show(void) {
    if (millis() < update_screen) return this;
    update_screen = millis() + period;

    int16_t temp        = pHG->averageTemp(); 								// Actual GUN temperature
    int16_t temp_set    = pHG->presetTemp();
    uint16_t tempH      = pCfg->tempHuman(temp);
    uint8_t  power		= pHG->avgPowerPcnt();
    uint16_t pwr_disp	= pHG->pwrDispersion();

    if (tuning && (abs(temp_set - temp) <= 4) && (pwr_disp <= pwr_disp_max) && power > 1)  {
    	if (!ready && temp_setready_ms && (millis() > temp_setready_ms)) {
    		pBz->shortBeep();
    		ready 				= true;
    		temp_setready_ms	= 0;
    		pD->calibReady(true);
    	}
    }

    if (!tuning || !ready)
    	pD->tInternal(temp);
    pD->appliedPower(power);
    return this;
}

void calibSCREEN::rotaryValue(int16_t value) {  							// The Encoder rotated
    update_screen = millis() + period * 2;
   	if (tuning) {															// Preset temperature (internal units)
   		pHG->setTemp(value);
   		if (ready)
   			pD->calibReady(false);
    	ready = false;
    	temp_setready_ms = millis() + 5000;    								// Prevent beep just right the new temperature setup
    	pD->tInternal(value);
    } else {
    	pD->tSet(temp_tip[value]);
    }
}

SCREEN* calibSCREEN::menu(void) {               							// Rotary encoder pressed
	if (tuning) {															// New reference temperature was confirmed
		pHG->switchPower(false);
	    if (ready) {														// The temperature has been stabilized
	    	ready = false;
	    	uint16_t temp	= pHG->averageTemp();							// The temperature of the Hot Air Gun in internal units
		    uint8_t ref 	= ref_temp_index;
		    calib_temp[ref] = temp;
		    uint16_t gun[3];
		    for (uint8_t i = 0; i < 3; ++i) {
		    	gun[i] = calib_temp[i];
		    }
		    buildCalibration(gun, ref);										// ref is 0 for 200 degrees and 2 for 400 degrees
		    pCfg->applyCalibrationData(gun);
		    pD->calibReady(false);
	    }
	    tuning	= false;
	    pEnc->reset(ref_temp_index, 0, 2, 1, 1, true);						// Turn back to the reference temperature point selection mode
	} else {																// Reference temperature index was selected from the list
		ref_temp_index 	= pEnc->read();
		tuning 			= true;
		uint16_t tempH 	= temp_tip[ref_temp_index];							// Read the preset temperature from encoder (see vars.cpp)
		uint16_t temp 	= pCfg->tempInternal(tempH);
		pEnc->reset(temp, 100, temp_max, 1, 5, false); 						// temp_max declared in vars.cpp
		pHG->setTemp(temp);
		pHG->switchPower(true);
	}
	update_screen = 0;
	return this;
}

SCREEN* calibSCREEN::menu_long(void) {      								// Save new Hot Air Gun calibration data
    pHG->switchPower(false);
	buildCalibration(calib_temp, 10); 										// 10 is bigger then the last index in the reference temp. Means build final calibration
	pCfg->applyCalibrationData(calib_temp);
	pCfg->saveCalibrationData(calib_temp);
	uint16_t temp = pCfg->tempPreset();
	pHG->setTemp(temp);
	uint8_t fan = pCfg->fanPreset();
	pCfg->save(temp, fan);
	pBz->doubleBeep();
    if (next) return next;
    return this;
}

void calibSCREEN::buildCalibration(uint16_t gun[], uint8_t ref_point) {
	if (gun[2] > temp_max) gun[2] = temp_max;								// int_temp_max is a maximum possible temperature (vars.cpp)

	const int req_diff = 200;
	if (ref_point <= 2) {													// gun[0-2] - internal temperature readings for the Hot Air Gun at reference points (200-400)
		for (uint8_t i = ref_point; i <= 2; ++i) {							// ref_point is 0 for 200 degrees and 3 for 400 degrees
			int diff = (int)gun[i+1] - (int)gun[i];
			if (diff < req_diff) {
				gun[i+1] = gun[i] + req_diff;								// Increase right neighborhood temperature to keep the difference
			}
		}
		if (gun[2] > temp_max)												// The high temperature limit is exceeded, temp_max. Lower all calibration
			gun[2] = temp_max;

		for (int8_t i = 2; i > 0; --i) {
			int diff = (int)gun[i] - (int)gun[i-1];
			if (diff < req_diff) {
				int t = (int)gun[i] - req_diff;					// Decrease left neighborhood temperature to keep the difference
				if (t < 0) t = 0;
				gun[i-1] = t;
			}
		}
	}
}

//---------------------------------------- class tuneSCREEN [tune the potentiometer ] --------------------------
void tuneSCREEN::init(void) {
    pHG->switchPower(false);
    max_power = pHG->getMaxFixedPower();
    pEnc->reset(max_power >> 2, 0, max_power, 1, 2);                        // Rotate the encoder to change the power supplied
    on = false;
    heat_ms = 0;
    pHG->setFan(tune_fan_speed);											// See vars.cpp
    pD->clear();
    pD->msgTune();
    pD->msgOFF();
    forceRedraw();
}

void tuneSCREEN::rotaryValue(int16_t value) {
    if (on) {
        heat_ms = millis();
        pHG->fixPower(value);
        pD->appliedPower(value);
    }
    forceRedraw();
}

SCREEN* tuneSCREEN::show(void) {
    if (millis() < update_screen) return this;
    update_screen = millis() + period;
    uint16_t temp   = pHG->getCurrTemp();
    uint8_t  power  = pHG->appliedPower();
    if (!on) {
        power = pEnc->read();
    }
    pD->tInternal(temp);
    pD->appliedPower(power);
    if (heat_ms && ((millis() - heat_ms) > 3000) && (pHG->tempDispersion() < 10) && (power > 1)) {
        pBz->shortBeep();
        heat_ms = 0;
    }
    return this;
}
  
SCREEN* tuneSCREEN::menu(void) {                                            // The rotary button pressed
    if (on) {
        pHG->fixPower(0);
        on = false;
        pD->msgOFF();
    } else {
        on = true;
        heat_ms = millis();
        uint8_t power = pEnc->read();                                       // applied power
        pHG->fixPower(power);
        pD->msgON();
    }
    return this;
}

SCREEN* tuneSCREEN::menu_long(void) {
    pHG->fixPower(0);                                                       // switch off the power
    pHG->switchPower(false);
    if (next) return next;
    return this;
}

//---------------------------------------- class pidSCREEN [tune the PID coefficients] -------------------------
void pidSCREEN::init(void) {
    temp_set = pHG->presetTemp();
    mode = 0;                                                               // select the element from the list
    pEnc->reset(1, 1, 5, 1, 1, true);                                       // 1 - Kp, 2 - Ki, 3 - Kd, 4 - temp, 5 - fan
    showCfgInfo();
    Serial.println("");
}

void pidSCREEN::rotaryValue(int16_t value) {
    if (mode == 0) {                                                        // select element from the menu
        showCfgInfo();
        switch (value) {
            case 1:
                Serial.println("Kp");
                break;
            case 2:
                Serial.println("Ki");
                break;
            case 4:
                Serial.println(F("Temp"));
                break;
            case 5:
                Serial.println(F("Fan"));
                break;
            case 3:
            default:
                Serial.println("Kd");
            break;
        }
    } else {
        switch (mode) {
            case 1:
                Serial.print(F("Kp = "));
                pHG->changePID(mode, value);
                break;
            case 2:
                Serial.print(F("Ki = "));
                pHG->changePID(mode, value);
                break;
            case 4:
                Serial.print(F("Temp = "));
                temp_set = value;
                pHG->setTemp(value);
                break;
            case 5:
                Serial.print(F("Fan Speed = "));
                pHG->setFan(value);
                break;
            case 3:
            default:
                Serial.print(F("Kd = "));
                pHG->changePID(mode, value);
                break;
        }
        Serial.println(value);
    }
}

SCREEN* pidSCREEN::show(void) {
    if (millis() < update_screen) return this;
    update_screen = millis() + period;
    if (pHG->isOn()) {
        char buff[80];
        int      temp   = pHG->getCurrTemp();
        uint8_t  pwr    = pHG->averagePower();
        uint16_t fs     = pHG->fanSpeed();
        fs = map(fs, 0, 255, 0, 100);
        sprintf(buff, "%3d: power = %3d%c, fan = %3d;", temp_set - temp, pwr, '%', fs);
        Serial.println(buff);
    }
    return this;
}
SCREEN* pidSCREEN::menu(void) {                                             // The encoder button pressed
    if (mode == 0) {                                                        // select upper or lower temperature limit
        mode = pEnc->read();
        if (mode > 0 && mode < 4) {
            int k = pHG->changePID(mode, -1);
            pEnc->reset(k, 0, 10000, 1, 10);
        } else if (mode == 4) {
            pEnc->reset(temp_set, 0, 970, 1, 5);
        } else {
            pEnc->reset(pHG->fanSpeed(), 0, max_fan_speed, 5, 20);
        }
    } else {    
        mode = 0;
        pEnc->reset(1, 1, 5, 1, 1, true);                                   // 1 - Kp, 2 - Ki, 3 - Kd, 4 - temp, 5 - fan speed
    }
    return this;
}

SCREEN* pidSCREEN::menu_long(void) {
    bool on = pHG->isOn();
    pHG->switchPower(!on);
    if (on)
        Serial.println(F("The air gun is OFF"));
    else
        Serial.println(F("The air gun is ON"));
  return this;
}

void pidSCREEN::showCfgInfo(void) {
    Serial.print(F("Temp set: "));
    Serial.print(temp_set, DEC);
    Serial.print(F(", fan speed = "));
    Serial.print(pHG->fanSpeed());
    Serial.print(F(", PID: ["));
    for (byte i = 1; i < 4; ++i) {
        int k = pHG->changePID(i, -1);
        Serial.print(k, DEC);
        if (i < 3) Serial.print(", ");
    }
    Serial.print("]; ");
}
