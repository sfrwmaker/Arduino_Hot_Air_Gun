#ifndef _DISPLAY_H_
#define _DISPLAY_H_

#include <stdint.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>

//------------------------------------------ class lcd DSPLay for soldering IRON -----------------------------
class DSPL : protected LiquidCrystal_I2C {
    public:
        DSPL() : LiquidCrystal_I2C(0x27, 16, 2)               				{ }
        virtual	~DSPL()														{ }
        void    init(void);
        void    clear(void)                                                 { LiquidCrystal_I2C::clear();   }
        void    tSet(uint16_t t, bool Celsius = true);                      // Show the preset temperature; The temperature units are Celsius always!
        void    tCurr(uint16_t t, bool Celsius = true);						// Show the current temperature
        void    tInternal(uint16_t t);                                      // Show the current temperature in internal units
        void	calibReady(bool on);										// Show calibrate ready sign
        void    fanSpeed(uint16_t s);                                       // Show the fan speed
        void    appliedPower(uint8_t p, bool show_zero = true);             // Show applied power (%)
        void    setupMode(uint8_t mode);
        void    msgON(void);                                                // Show message: "ON"
        void    msgOFF(void);
        void    msgReady(void);
        void    msgCold(void);
        void    msgFail(void);                                              // Show 'Fail' message
        void    msgTune(void);                                              // Show 'Tune' message
    private:
        bool    full_second_line	= false;								// Whether the second line is full with the message
        char    temp_units			= 'C';
        const   uint8_t custom_symbols[3][8] = {
                          { 0b00110,                                        // Degree
                            0b01001,
                            0b01001,
                            0b00110,
                            0b00000,
                            0b00000,
                            0b00000,
                            0b00000
                          },
                          { 0b00000,                                        // Fan sign
                            0b10001,
                            0b11011,
                            0b01110,
                            0b01110,
                            0b11011,
                            0b10001,
                            0b00000
                          },
                          { 0b00011,                                        // Power sign
                            0b00110,
                            0b01100,
                            0b11111,
                            0b00110,
                            0b01100,
                            0b01000,
                            0b10000
                          }
                        };
};

#endif
