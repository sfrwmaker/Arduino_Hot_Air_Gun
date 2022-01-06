#include <Arduino.h>
#include "gun.h"
#include "vars.h"

//------------------------------------------ class PID algoritm to keep the temperature -----------------------
void PID::resetPID(int temp) {
    temp_h0 = 0;
    power  = 0;
    i_summ = 0;
    pid_iterate = false;
    if ((temp > 0) && (temp < 1000))
        temp_h1 = temp;
    else
        temp_h1 = 0;
}

int PID::changePID(uint8_t p, int k) {
    switch(p) {
        case 1:
            if (k >= 0) Kp = k;
            return Kp;
        case 2:
            if (k >= 0) Ki = k;
            return Ki;
        case 3:
            if (k >= 0) Kd = k;
            return Kd;
        default:
        break;
    }
    return 0;
}

long PID::reqPower(int temp_set, int temp_curr) {
    if (temp_h0 == 0) {
        // When the temperature is near the preset one, reset the PID and prepare iterative formula                        
        if ((temp_set - temp_curr) < 30) {
            if (!pid_iterate) {
                pid_iterate = true;
                power = 0;
                i_summ = 0;
            }
        }
        i_summ += temp_set - temp_curr;                                     // first, use the direct formula, not the iterate process
        power = Kp*(temp_set - temp_curr) + Ki*i_summ;
    // If the temperature is near, prepare the PID iteration process
    } else {
        long kp = Kp * (temp_h1 - temp_curr);
        long ki = Ki * (temp_set - temp_curr);
        long kd = Kd * (temp_h0 + temp_curr - 2*temp_h1);
        long delta_p = kp + ki + kd;
        power += delta_p;                                                   // power kept multiplied by denominator!
    }
    if (pid_iterate) temp_h0 = temp_h1;
    temp_h1 = temp_curr;
    long pwr = power + (1 << (denominator_p-1));                            // prepare the power to delete by denominator, round the result
    pwr >>= denominator_p;                                                  // delete by the denominator
    return pwr;
}

//--------------------- High frequency PWM signal class on D9 pin -----------------------------------------
void FastPWM_D9::init(void) {
    pinMode(9, OUTPUT);
    digitalWrite(9, LOW);
    noInterrupts();
    TCNT1   = 0;
    TCCR1B  = _BV(WGM13);													// set mode as phase and frequency correct pwm, stop the timer
    TCCR1A  = 0;
    ICR1    = max_fan_speed+1;
    TCCR1B  = _BV(WGM13) | _BV(CS11) | _BV(CS10); 							// Top value = ICR1 (2000), prescale = 64; 16000000 / 64 / 2*2000 = 62.5 Hz PWM
    TCCR1A |= _BV(COM1A1);													// XOR D9 on OCR1A, detached from D10
    OCR1A   = 0;															// Switch-off the signal on pin 9;
    interrupts();
}

//--------------------- Hot air gun manager using complete sine shape to power on the hardware ---------------

HOTGUN_HW::HOTGUN_HW(uint8_t HG_sen_pin, uint8_t HG_pwr_pin, uint8_t HG_ac_relay_pin) {
    sen_pin         = HG_sen_pin;
    gun_pin         = HG_pwr_pin;
    ac_relay_pin    = HG_ac_relay_pin;
}

void HOTGUN_HW::init(void) {
    cnt             = 0;
    actual_power    = 0;
    active          = false;
    pinMode(sen_pin, INPUT);
    pinMode(gun_pin, OUTPUT);
    digitalWrite(gun_pin, LOW);
    pinMode(ac_relay_pin, OUTPUT);
    digitalWrite(ac_relay_pin, LOW);
    h_temp.reset();
    safetyRelay(false);                                     // Completely turn-off the power of Hot Air Gun
}

// We need some time to activate the relay, so we initialize the relay_ready_cnt variable.
void HOTGUN_HW::safetyRelay(bool activate) {
    if (activate) {
        digitalWrite(ac_relay_pin, HIGH);
        relay_ready_cnt = relay_activate;
    } else {
        digitalWrite(ac_relay_pin, LOW);
        relay_ready_cnt = 0;
    }
}

bool HOTGUN_HW::syncCB(void) {
    if (++cnt >= period) {
        cnt = 0;
        last_period = millis();                                             // Save the current time to check the external interrupts
        if (!active && (actual_power > 0)) {
            digitalWrite(gun_pin, HIGH);
            active = true;
        }
    } else if (cnt >= actual_power) {
        if (active) {
            digitalWrite(gun_pin, LOW);
            active = false;
        }
    }
    uint16_t t = analogRead(sen_pin);
    h_temp.update(t);														// Update hot gun temperature
    return (cnt == 0);                                                      // End of the Power period (period AC voltage shapes)
}

void HOTGUN::init(void) {
    mode            = POWER_OFF;                            // Completely stopped, no power on fan also
    fan_speed       = 0;
    fix_power       = 0;
    chill       = false;
    HOTGUN_HW::init();
    hg_fan.init();
    h_temp.reset();
    d_power.length(ec);
    resetPID();
}

uint8_t HOTGUN::avgPowerPcnt(void) {
    uint8_t pcnt = 0;
    if (mode == POWER_FIXED) {
        pcnt = map(fix_power, 0, max_fix_power, 0, 100);
    } else {
        pcnt = map(h_power.read(), 0, max_power, 0, 100);
    }
    if (pcnt > 100) pcnt = 100;
    return pcnt;
}

void HOTGUN::switchPower(bool On) {
    fan_off_time = 0;                                       // Disable fan offline by timeout
    switch (mode) {
        case POWER_OFF:
            if (fanSpeed() == 0) {                          // No power supplied to the Fan
                if (On) {                                   // !FAN && On
                    safetyRelay(true);                      // Supply AC power to the hot air gun socket
                    mode = POWER_ON;
                }
            } else {
                if (On) {
                    // FAN && On && connected
                    safetyRelay(true);
                    mode = POWER_ON;
                } else {
                    // FAN && !On && connected
                    if (isCold()) {                     	// FAN && !On && connected && cold
                        shutdown();
                    } else {                            	// FAN && !On && connected && !cold
                        mode = POWER_COOLING;
                        fan_off_time = millis() + fan_off_timeout;
                    }
                }
            }
            break;
        case POWER_ON:
            if (!On) {                                      // Start cooling the hot air gun
                mode = POWER_COOLING;
                fan_off_time = millis() + fan_off_timeout;
            }
            break;
        case POWER_FIXED:
            if (fanSpeed()) {
                if (On) {                                   // FAN && On
                    mode = POWER_ON;
                } else {                                    // FAN && !On
                    // FAN && !On && connected
                    if (isCold()) {                     	// FAN && !On && connected && cold
                        shutdown();
                    } else {                            	// FAN && !On && connected && !cold
                        mode = POWER_COOLING;
                        fan_off_time = millis() + fan_off_timeout;
                    }
                }
            } else {                                        // !FAN
                if (!On) {                                  // !FAN && !On
                    shutdown();
                }
            }
            break;
        case POWER_COOLING:
            if (fanSpeed()) {
                if (On) {                                   // FAN && On
                    // FAN && On && connected
                    safetyRelay(true);                  	// Supply AC power to the hot air gun socket
                    mode = POWER_ON;
                } else {                                    // FAN && !On
                    if (isCold()) {                     	// FAN && !On && connected && cold
                        shutdown();
                    }
                }
            } else {
                if (On) {                                   // !FAN && On
                    safetyRelay(true);                      // Supply AC power to the hot air gun socket
                    mode = POWER_ON;
                }
            }
            break;
        default:
            break;
    }
    h_power.reset();
    d_power.reset();
}

void HOTGUN::fixPower(uint8_t Power) {
    if (Power == 0) {                                       // To switch off the hot gun, set the Power to 0
        switchPower(false);
        return;
    }

    if (Power > max_power) Power = max_power;
    mode = POWER_FIXED;
    safetyRelay(true);                                      // Supply AC power to the hot air gun socket
    fix_power   = Power;
}



void HOTGUN::keepTemp(void) {
    uint16_t t = h_temp.read();                             // Actual Hot Air Gun temperature

    if ((t >= temp_max + 30) || (t > (temp_set + 100))) {  	// Prevent global over heating (see vars.cpp)
        if (mode == POWER_ON) chill = true;                 // Turn off the power in main working mode only;
    }

    int32_t p = 0;                                          // The Hot Air Gun power value
    switch (mode) {
        case POWER_OFF:
            break;
        case POWER_ON:
            hg_fan.duty(fan_speed);                         // Turn on the fan immediately
            if (chill) {
                if (t < (temp_set - 2)) {
                    chill = false;
                    resetPID();
                } else {
                    break;
                }
            }
            if (relay_ready_cnt > 0) {                      // Relay is not ready yet
                --relay_ready_cnt;                          // Do not apply power to the HOT GUN till AC relay is ready
            } else {
                p = PID::reqPower(temp_set, t);
                p = constrain(p, 0, max_power);
            }
            break;
        case POWER_FIXED:
            if (relay_ready_cnt > 0) {                      // Relay is not ready yet
                --relay_ready_cnt;                          // Do not apply power to the HOT GUN till AC relay is ready
            } else {
                p = fix_power;
            }
            hg_fan.duty(fan_speed);
            break;
        case POWER_COOLING:
            if (fanSpeed() < min_fan_speed) {
                shutdown();
            } else {
                if (isCold()) {                         	// FAN && connected && cold
                	if (extra_cooling == 0) {
                		extra_cooling = millis() + 60000;	// Wait 1 minute to completely cool the Hot Air Gun
                	} else if (millis() > extra_cooling) {
                		shutdown();
                	}
                } else {                                	// FAN && connected && !cold
                    uint16_t fan = map(h_temp.read(), temp_gun_cold, temp_set, max_cool_fan, min_fan_speed);
                    fan = constrain(fan, min_fan_speed, max_fan_speed);
                    hg_fan.duty(fan);
                }
            }
            break;
        default:
            break;
    }

    // Only supply the power to the heater if the Hot Air Gun is connected
    if (fanSpeed() < min_fan_speed) p = 0;
    h_power.update(p);
    int32_t ap  	= h_power.average(p);
    int32_t diff    = ap - p;
    d_power.update(diff*diff);
    actual_power = constrain(p, 0, period-1);
    return;
}

uint8_t HOTGUN::presetFanPcnt(void) {
    uint16_t pcnt = map(fan_speed, 0, max_fan_speed, 0, 100);
    if (pcnt > 100) pcnt = 100;
    return pcnt;

}

// Can be called from the event handler.
void HOTGUN::shutdown(void) {
    mode = POWER_OFF;
    hg_fan.duty(0);
    safetyRelay(false);                                     // Stop supplying AC power to the hot air gun
	extra_cooling = 0;
}
