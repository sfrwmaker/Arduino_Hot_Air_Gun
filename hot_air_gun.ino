/*
 * Hot air gun controller based on Arduino nano board
 * Version 2.0
 * Released Jan 06, 2022
 */
#include <avr/io.h>
#include <avr/interrupt.h>
#include "encoder.h"
#include "config.h"
#include "buzzer.h"
#include "display.h"
#include "stat.h"
#include "gun.h"
#include "screen.h"
#include "vars.h"

const uint8_t AC_SYNC_PIN   = 2;                                            // Outlet 220 v synchronization pin. Do not change!
const uint8_t HOT_GUN_PIN   = 7;                                            // Hot gun heater management pin
const uint8_t FAN_GUN_PIN   = 9;                                            // Hot gun fan management pin. Do not change! 
const uint8_t TEMP_GUN_PIN	= A0;                                           // Hot gun temperature checking pin
const uint8_t FAN_PWR_PIN	= 9;											// Hot gun fan power pin, do not change! Used in FastPWM_D9 class. see gun.h

const uint8_t R_MAIN_PIN	= 3;                                            // Rotary encoder main pin. Do not change!
const uint8_t R_SECD_PIN	= 4;                                            // Rotary encoder secondary pin
const uint8_t R_BUTN_PIN	= 5;                                            // Rotary encoder button pin

const uint8_t REED_SW_PIN   = 8;                                            // Reed switch pin
const uint8_t BUZZER_PIN	= 6;                                            // Buzzer pin
const uint8_t AC_RELAY_PIN  = 12;                                           // Safety AC relay

HOTGUN 		hg(TEMP_GUN_PIN, HOT_GUN_PIN, AC_RELAY_PIN);
DSPL       	disp;
RENC    	rotEncoder(R_MAIN_PIN, R_SECD_PIN, R_BUTN_PIN);
SWITCH      reedSwitch;
HOTGUN_CFG 	hgCfg;
BUZZER     	simpleBuzzer(BUZZER_PIN);

mainSCREEN   offScr(&hg,  &disp, &rotEncoder, &simpleBuzzer, &hgCfg);
workSCREEN   wrkScr(&hg,  &disp, &rotEncoder, &simpleBuzzer, &hgCfg);
configSCREEN cfgScr(&hg,  &disp, &rotEncoder, &hgCfg);
calibSCREEN  clbScr(&hg,  &disp, &rotEncoder, &simpleBuzzer, &hgCfg);
tuneSCREEN   tuneScr(&hg, &disp, &rotEncoder, &simpleBuzzer);
errorSCREEN  errScr(&hg,  &disp, &simpleBuzzer);
pidSCREEN    pidScr(&hg,  &rotEncoder);

SCREEN 	*pCurrentScreen = &offScr;

volatile bool	end_of_power_period = false;

void syncAC(void) {
    end_of_power_period = hg.syncCB();
}

void rotEncChange(void) {
	rotEncoder.encoderIntr();
}

void checkReedStatus(void) {
	static uint32_t check_sw = 0;
	if (millis() > check_sw) {
		uint16_t on = 0;
		if (digitalRead(REED_SW_PIN)) on = 100;
		reedSwitch.update(on);												// If reed switch open, write 100;
		check_sw = millis() + 100;
	}
}

void setup() {
    analogReference(EXTERNAL);
	Serial.begin(115200);
	disp.init();

	// Load configuration parameters
	hgCfg.init();
	hg.init();
	uint16_t temp 	= hgCfg.tempPreset();
	uint16_t fan	= hgCfg.fanPreset();
	hg.setTemp(temp);
	hg.setFan(fan);

    pinMode(REED_SW_PIN, INPUT_PULLUP);
    reedSwitch.init(10, 30, 60);

	// Initialize rotary encoder
	rotEncoder.init();
	delay(500);
	attachInterrupt(digitalPinToInterrupt(R_MAIN_PIN), rotEncChange,   CHANGE);
	attachInterrupt(digitalPinToInterrupt(AC_SYNC_PIN), syncAC, RISING);

	// Initialize SCREEN hierarchy
	offScr.next     = &cfgScr;
    offScr.on       = &wrkScr;
    wrkScr.next     = &offScr;
    cfgScr.next     = &offScr;
    cfgScr.calib    = &clbScr;
    cfgScr.tune     = &tuneScr;
    clbScr.next     = &offScr;
    tuneScr.next    = &offScr;
	errScr.next     = &offScr;

    pCurrentScreen->init();
}

void loop() {
    static bool     reset_encoder   = true;
	static int16_t  old_pos 	    = 0;
	static uint32_t ac_check 	    = 5000;

	checkReedStatus();
	int16_t pos = rotEncoder.read();
    if (reset_encoder) {
        old_pos = pos;
        reset_encoder = false;
    } else {
	    if (old_pos != pos) {
		    pCurrentScreen->rotaryValue(pos);
		    old_pos = pos;
	    }
    }

    SCREEN* nxt = pCurrentScreen->reedSwitch(reedSwitch.status());
    if (nxt != pCurrentScreen) {
        pCurrentScreen = nxt;
        pCurrentScreen->init();
        reset_encoder = true;
        return;
    }
    
	uint8_t bStatus = rotEncoder.buttonCheck();
	switch (bStatus) {
		case 2:                                     						// long press;
			nxt = pCurrentScreen->menu_long();
			if (nxt != pCurrentScreen) {
				pCurrentScreen = nxt;
				pCurrentScreen->init();
                reset_encoder = true;
			}
			break;
		case 1:                                     						// short press
			nxt = pCurrentScreen->menu();
			if (nxt != pCurrentScreen) {
				pCurrentScreen = nxt;
				pCurrentScreen->init();
                reset_encoder = true;
			}
			break;
		case 0:                                     						// Not pressed
		default:
			break;
	}

	nxt = pCurrentScreen->show();
	if (nxt && pCurrentScreen != nxt) {           							// Be paranoid, the returned value must not be null
		pCurrentScreen = nxt;
		pCurrentScreen->init();
        reset_encoder = true;
	}
	
	if (end_of_power_period) {												// Calculate the required power
		hg.keepTemp();
		end_of_power_period = false;
	}

	if (millis() > ac_check) {
		ac_check = millis() + 1000;
		if (!hg.areExternalInterrupts()) {
			nxt = &errScr;
			if (nxt != pCurrentScreen) {
				pCurrentScreen = nxt;
				pCurrentScreen->init();
                reset_encoder = true;
			}
		}
	}
}
