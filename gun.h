#ifndef _GUN_H_
#define _GUN_H_

#include <stdint.h>
#include "stat.h"
#include "vars.h"

//------------------------------------------ class PID algoritm to keep the temperature -----------------------
/*  The PID algorithm 
 *  Un = Kp*(Xs - Xn) + Ki*summ{j=0; j<=n}(Xs - Xj) + Kd(Xn - Xn-1),
 *  Where Xs - is the setup temperature, Xn - the temperature on n-iteration step
 *  In this program the interactive formula is used:
 *    Un = Un-1 + Kp*(Xn-1 - Xn) + Ki*(Xs - Xn) + Kd*(Xn-2 + Xn - 2*Xn-1)
 *  With the first step:
 *  U0 = Kp*(Xs - X0) + Ki*(Xs - X0); Xn-1 = Xn;
 *  
 *  PID coefficients history:
 *  10/14/2017  [768,     32, 328]
 *  11/27/2019  [ 2009, 1600,  20]
 *  04/27/2020  [   50,   16,  50]
 */
class PID {
    public:
        PID(void) {
            Kp		= 50;
            Ki		= 16;
            Kd		= 50;
        }
        void 	resetPID(int temp = -1);									// reset PID algorithm history parameters
        long 	reqPower(int temp_set, int temp_curr);						// Calculate the power to be applied
        int  	changePID(uint8_t p, int k);								// set or get (if parameter < 0) PID parameter
    private:
        void  	debugPID(int t_set, int t_curr, long kp, long ki, long kd, long delta_p);
        int   	temp_h0		= 0;											// previously measured temperature
        int 	temp_h1		= 0;
        bool	pid_iterate	= false;										// Whether the iterative process is used
        long	i_summ		= 0;											// Ki summary multiplied by denominator
        long	power		= 0;											// The power iterative multiplied by denominator
        long	Kp, Ki, Kd;													// The PID algorithm coefficients multiplied by denominator
        const uint8_t denominator_p = 11;									// The common coefficient denominator power of 2 (11 means divide by 2048)
};

//--------------------- High frequency PWM signal calss on D9 pin -----------------------------------------
class FastPWM_D9 {
    public:
        FastPWM_D9()										{ }
        void		init(void);
        void        duty(uint16_t d)						{ OCR1A = d; }
        uint16_t    fanSpeed(void)                  		{ return OCR1A; }
};

//--------------------- Hot air gun manager using complete sine shape to power on the hardware ---------------
const uint8_t    hot_gun_hist_length         = 10;  // The history data length of Hot Air Gun average values
class HOTGUN_HW {
    public:
        HOTGUN_HW(uint8_t HG_sen_pin, uint8_t HG_pwr_pin, uint8_t HG_ac_relay_pin);
        void        init(void);
        uint8_t     appliedPower(void)                      { return actual_power;                      }
        uint16_t    tempDispersion(void)                    { return h_temp.dispersion();               }
        bool        areExternalInterrupts(void)             { return millis() - last_period < period * 15; }
        bool        syncCB(void);                           // Return true at the end of the power period
    protected:
        HIST        h_temp;                                 // Hot Air Gun temperature
        void        safetyRelay(bool activate);
        volatile    uint8_t     relay_ready_cnt = 0;        // The relay ready counter, see HOTHUN::power()
        volatile    uint8_t     actual_power;               // Actual power supplied to the heater
        const       uint8_t     period          = 100;
    private:
        volatile    bool        active;                     // Is the heater active (PWM sigthal phase)
        volatile    uint32_t    last_period;                // The time in ms when the counter reset
        uint8_t     sen_pin;                                // The temperature sensor pin
        uint8_t     gun_pin;                                // The Hot Gun heater management pin
        uint8_t     ac_relay_pin;                           // The safety relay pin
        uint32_t    check_sw                    = 0;        // Time when check reed switch status (ms)
        volatile    uint8_t     cnt             = 0;        // The AC sine counter (simulate PWM signal)
        const       uint32_t    relay_activate  = 1;        // The relay activation delay (loops of TIM1, 1 time per second)
};

class HOTGUN : public HOTGUN_HW, public PID {
    public:
        typedef enum { POWER_OFF, POWER_ON, POWER_FIXED, POWER_COOLING } PowerMode;
        HOTGUN(uint8_t HG_sen_pin, uint8_t HG_pwr_pin, uint8_t HG_ac_relay_pin) :
        HOTGUN_HW(HG_sen_pin, HG_pwr_pin, HG_ac_relay_pin), h_power(hot_gun_hist_length) { }
        void        init(void);
        bool        isOn(void)                              { return (mode == POWER_ON || mode == POWER_FIXED); }
        uint16_t    presetTemp(void)                        { return temp_set;                              }
        uint16_t    presetFan(void)                         { return fan_speed;                             }
        uint16_t    averageTemp(void)                       { return h_temp.read();                         }
        uint16_t    averagePower(void)                      { return h_power.read();                        }
        uint8_t     getMaxFixedPower(void)                  { return max_fix_power;                         }
        bool        isCold(void)                            { return h_temp.read() < temp_gun_cold;         }
        bool        isFanWorking(void)                      { return (fanSpeed() >= min_fan_speed);         }
        uint16_t    maxFanSpeed(void)                       { return max_fan_speed;                         }
        uint16_t    pwrDispersion(void)                     { return d_power.read();                        }
        void        setTemp(uint16_t temp)                  { temp_set  = constrain(temp, 0, temp_max); 	}
        void        updateTemp(uint16_t value)              { h_temp.update(value);   						}
        void        setFan(uint16_t fan)                    { fan_speed = constrain(fan, min_fan_speed, max_fan_speed);   }
        void        fanFixed(uint16_t fan)					{ hg_fan.duty(fan);                             }
        uint16_t    getCurrTemp(void)                       { return h_temp.last();                         }
        void        switchPower(bool On);
        uint8_t     avgPowerPcnt(void);
        uint16_t    fanSpeed(void)                          { return hg_fan.fanSpeed();                     }
        void        fixPower(uint8_t Power);                // Set the specified power to the the hot gun
        uint8_t     presetFanPcnt(void);
        void        keepTemp(void);                         // Calculate Hot Air Gun power to keep the preset temperature
    private:
        void        shutdown(void);
        FastPWM_D9  hg_fan;
        PowerMode   mode                = POWER_OFF;
        uint8_t     fix_power           = 0;                // Fixed power value of the Hot Air Gun (or zero if off)
        bool        chill               = false;            // Chill the Hot Air gun if it is over heating
        uint16_t    temp_set            = 0;                // The preset temperature of the hot air gun (internal units)
        uint16_t    fan_speed           = 0;                // Preset fan speed
        uint32_t    fan_off_time        = 0;                // Time when the fan should be powered off in cooling mode (ms)
        uint32_t	extra_cooling		= 0;				// Time when completely power off the fan
        EMP_AVERAGE h_power;                                // Exponential average of applied power
        EMP_AVERAGE d_power;                                // Exponential average of power dispersion
        EMP_AVERAGE zero_temp;                              // Exponential average of minimum (zero) temperature
        const       uint8_t     max_fix_power   = 70;
        const       uint8_t     max_power       = 99;
        const       uint16_t    max_cool_fan    = 1700;
        const       uint16_t    temp_gun_cold   = 20;       // The temperature of the cold Hot Air Gun
        const       uint32_t    fan_off_timeout = 5*60*1000;// The timeout to turn the fan off in cooling mode
        const       uint8_t     ec              = 200;      // Exponential average coefficient (default value)
};

#endif
