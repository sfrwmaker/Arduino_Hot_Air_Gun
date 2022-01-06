#ifndef _SCREEN_H_
#define _SCREEN_H_

#include <stdint.h>
#include "encoder.h"
#include "gun.h"
#include "display.h"
#include "buzzer.h"
#include "config.h"
#include "vars.h"

//------------------------------------------ class SCREEN ------------------------------------------------------
class SCREEN {
    public:
        SCREEN* next;                                       // Pointer to the next screen
        SCREEN() {
            next            = 0;
            update_screen   = 0;
            scr_timeout     = 0;
            time_to_return  = 0;
        }
        virtual			~SCREEN(void)									{ }
        virtual void    init(void)                          			{ }
        virtual SCREEN* show(void)                          			{ return this; }
        virtual SCREEN* menu(void)                          			{ return this; }
        virtual SCREEN* menu_long(void)                     			{ if (this->next != 0)  return this->next;  else return this; }
        virtual SCREEN* reedSwitch(bool on)                 			{ return this; }
        virtual void    rotaryValue(int16_t value)          			{ }
        void            forceRedraw(void)                   			{ update_screen = 0; }
    protected:
        uint32_t update_screen;                             			// Time in ms when the screen should be updated
        uint32_t scr_timeout;                               			// Timeout is sec. to return to the main screen, canceling all changes
        uint32_t time_to_return;                            			// Time in ms to return to main screen
};

//---------------------------------------- class mainSCREEN [the hot air gun is OFF] ---------------------------
class mainSCREEN : public SCREEN {
    public:
        mainSCREEN(HOTGUN* HG, DSPL* DSP, RENC* ENC, BUZZER* Buzz, HOTGUN_CFG* Cfg) {
            pHG     = HG;
            pD      = DSP;
            pEnc    = ENC;
            pBz     = Buzz;
            pCfg    = Cfg;
        }
        virtual void    init(void);
        virtual SCREEN* show(void);
        virtual SCREEN* menu(void);
        virtual SCREEN* reedSwitch(bool on);
        virtual void    rotaryValue(int16_t value);							// Setup the preset temperature
        SCREEN*     on						= 0;							// Screen mode when the power is
    private:
        HOTGUN*     pHG;													// Pointer to the hot air gun instance
        DSPL*       pD;														// Pointer to the DSPLay instance
        RENC*    	pEnc;													// Pointer to the rotary encoder instance
        BUZZER*     pBz;													// Pointer to the simple buzzer instance
        HOTGUN_CFG* pCfg;													// Pointer to the configuration instance
        uint32_t    clear_used_ms			= 0;							// Time in ms when used flag should be cleared (if > 0)
        uint32_t    mode_change				= 0;							// Preset mode: change temperature or change fan speed
        bool        used					= false;						// Whether the IRON was used (was hot)
        bool        cool_notified			= false;						// Whether there was cold notification played
        const uint16_t period               = 1000;                         // The period to update the screen
        const uint32_t cool_notify_period   = 120000;                       // The period to display 'cool' message (ms)
        const uint16_t show_temp            = 20000;                        // The period to show the preset temperature (ms)
        const uint32_t fan_adjust_to		= 5000;							// Fan adjustment timeout
};

//---------------------------------------- class workSCREEN [the hot air gun is ON] ----------------------------
class workSCREEN : public SCREEN {
    public:
        workSCREEN(HOTGUN* HG, DSPL* DSP, RENC* Enc, BUZZER* Buzz, HOTGUN_CFG* Cfg) {
            update_screen = 0;
            pHG     = HG;
            pD      = DSP;
            pBz     = Buzz;
            pEnc    = Enc;
            pCfg    = Cfg;
        }
        virtual void    init(void);
        virtual SCREEN* show(void);
        virtual SCREEN* menu(void);
        virtual SCREEN* reedSwitch(bool on);
        virtual void    rotaryValue(int16_t value);                         // Change the preset temperature
    private:
        HOTGUN*     pHG;                                                    // Pointer to the IRON instance
        DSPL*       pD;                                                     // Pointer to the DSPLay instance
        BUZZER*     pBz;                                                    // Pointer to the simple Buzzer instance
        RENC*    	pEnc;                                                   // Pointer to the rotary encoder instance
        HOTGUN_CFG* pCfg;                                                   // Pointer to the configuration instance
        bool        ready					= false;						// Whether the IRON have reached the preset temperature
        uint32_t	mode_change				= 0;							// Time when to return to the temperature change mode
        const uint16_t period				= 1000;                         // The period to update the screen (ms)
        const uint32_t fan_adjust_to		= 5000;							// Fan adjustment timeout
};

//---------------------------------------- class errorSCREEN [the error detected] ------------------------------
class errorSCREEN : public SCREEN {
    public:
        errorSCREEN(HOTGUN* HG, DSPL* DSP, BUZZER* Buzz) {
            pHG     = HG;
            pD      = DSP;
            pBz     = Buzz;
        }
        virtual void init(void)                                             { pHG->switchPower(false); pD->clear(); pD->msgFail(); pBz->failedBeep(); }
        virtual SCREEN* menu(void)                                          { if (this->next != 0)  return this->next;  else return this; }
    private:
        HOTGUN*     pHG;                                                    // Pointer to the got air gun instance
        DSPL*       pD;                                                     // Pointer to the display instance
        BUZZER*     pBz;                                                    // Pointer to the simple Buzzer instance
};

//---------------------------------------- class configSCREEN [configuration menu] -----------------------------
class configSCREEN : public SCREEN {
    public:
        configSCREEN(HOTGUN* HG, DSPL* DSP, RENC* Enc, HOTGUN_CFG* Cfg) {
            pHG     = HG;
            pD      = DSP;
            pEnc    = Enc;
            pCfg    = Cfg;
        }
        virtual void    init(void);
        virtual SCREEN* show(void);
        virtual SCREEN* menu(void);
        virtual void    rotaryValue(int16_t value);
        SCREEN*         calib				= 0;							// Pointer to the calibration SCREEN
        SCREEN*         tune				= 0;							// Pointer to the tune SCREEN
    private:
        HOTGUN*     pHG;                                                    // Pointer to the HOTGUN instance
        DSPL*       pD;                                                     // Pointer to the DSPLay instance
        RENC*    	pEnc;                                                   // Pointer to the rotary encoder instance
        HOTGUN_CFG* pCfg;                                                   // Pointer to the config instance
        uint8_t     mode					= 0;							// 0 - hotgun calibrate, 1 - tune, 2 - save, 3 - cancel, 4 - defaults
        const uint16_t period = 10000;                                      // The period in ms to update the screen
};

//---------------------------------------- class calibSCREEN [ Manual Hot Air Gun Calibration ] ----------------
class calibSCREEN : public SCREEN {
    public:
        calibSCREEN(HOTGUN* HG, DSPL* DSP, RENC* Enc, BUZZER* Buzz, HOTGUN_CFG* Cfg) {
            pHG     = HG;
            pD      = DSP;
            pEnc    = Enc;
            pCfg    = Cfg;
            pBz     = Buzz;
        }
        virtual void    init(void);
        virtual SCREEN* show(void);
        virtual void    rotaryValue(int16_t value);
        virtual SCREEN* menu(void);
        virtual SCREEN* menu_long(void);
    private:
        void		buildCalibration(uint16_t gun[], uint8_t ref_point);
        HOTGUN*     pHG;													// Pointer to the Hot Ait Gun instance
        DSPL*       pD;														// Pointer to the DSPLay instance
        RENC*    	pEnc;													// Pointer to the rotary encoder instance
        HOTGUN_CFG* pCfg;													// Pointer to the config instance
        BUZZER*     pBz;													// Pointer to the buzzer instance
        uint8_t		ref_temp_index		= 0;								// Which temperature reference to change: [0-3]
        uint16_t	calib_temp[3];											// The calibration temp. in internal units in reference points
        bool		ready				= 0;								// Whether the temperature has been established
        bool		tuning				= 0;								// Whether the reference temperature is modifying (else we select new reference point)
        uint32_t	temp_setready_ms	= 0;								// The time in ms when we should check the temperature is ready
        uint16_t	fan_speed			= 1500;								// The Hot Air Gun fan speed during calibration
        const uint32_t period = 1000;										// Update screen period
        const uint16_t pwr_disp_max	= 100;
};

//---------------------------------------- class tuneSCREEN [tune the potentiometer ] --------------------------
class tuneSCREEN : public SCREEN {
    public:
        tuneSCREEN(HOTGUN* HG, DSPL* DSP, RENC* Enc, BUZZER* Buzz) {
            pHG     = HG;
            pD      = DSP;
            pEnc    = Enc;
            pBz     = Buzz;
        }
        virtual void    init(void);
        virtual SCREEN* menu(void);
        virtual SCREEN* menu_long(void);
        virtual SCREEN* show(void);
        virtual void    rotaryValue(int16_t value);
    private:
        HOTGUN*     pHG;													// Pointer to the IRON instance
        DSPL*       pD;														// Pointer to the display instance
        RENC*    	pEnc;													// Pointer to the rotary encoder instance
        BUZZER*     pBz;													// Pointer to the simple Buzzer instance
        bool        on						= false;						// Wether the power is on
        uint32_t    heat_ms					= 0;							// Time in ms when power was on
        uint8_t     max_power				= 0;							// Maximum possible power to be applied
        const uint16_t period = 500;                                        // The period in ms to update the screen
};

//---------------------------------------- class pidSCREEN [tune the PID coefficients] -------------------------
class pidSCREEN : public SCREEN {
    public:
        pidSCREEN(HOTGUN* HG, RENC* ENC) {
            pHG     = HG;
            pEnc    = ENC;
        }
        virtual void    init(void);
        virtual SCREEN* menu(void);
        virtual SCREEN* menu_long(void);
        virtual SCREEN* show(void);
        virtual void    rotaryValue(int16_t value);
    private:
        void        showCfgInfo(void);										// show the main config information: Temp set, fan speed and PID coefficients
        HOTGUN*     pHG;													// Pointer to the IRON instance
        RENC*    	pEnc;													// Pointer to the rotary encoder instance
        uint8_t     mode					= 0;							// Which parameter to tune [0-5]: select element, Kp, Ki, Kd, temp, speed
        int         temp_set				= 0;
        const uint16_t period = 1100;
};

#endif
