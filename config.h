#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <stdint.h>
#include <string.h>
#include <EEPROM.h>
#include "vars.h"

//------------------------------------------ Configuration data ------------------------------------------------
/* Config record in the EEPROM has the following format:
 * uint32_t ID                           each time increment by 1
 * struct cfg                            config data, 9 bytes
 * byte CRC                              the checksum
*/
struct cfg {
    uint32_t    calibration;                                                // Packed calibration data by three temperature points
    uint16_t    temp;                                                       // The preset temperature of the IRON in internal units
    uint16_t    fan;                                                        // The preset fan speed 0 - max_fan_speed
    uint8_t     dspl_bright;                                                // The display brightness
};

class CONFIG {
    public:
        CONFIG() {
            can_write     = false;
            buffRecords   = 0;
            rAddr = wAddr = 0;
            eLength       = 0;
            nextRecID     = 0;
            uint8_t rs = sizeof(struct cfg) + 5;                             // The total config record size
            // Select appropriate record size; The record size should be power of 2, i.e. 8, 16, 32, 64, ... bytes
            for (record_size = 8; record_size < rs; record_size <<= 1);
        }
        void init();
        bool load(void);
        void getConfig(struct cfg &Cfg);                                    // Copy config structure from this class
        void updateConfig(struct cfg &Cfg);                                 // Copy updated config into this class
        bool save(void);                                                    // Save current config copy to the EEPROM
        bool saveConfig(struct cfg &Cfg);                                   // write updated config into the EEPROM
    protected:
        struct   cfg Config;
    private:
        bool     readRecord(uint16_t addr, uint32_t &recID);
        bool     can_write;                                                 // The flag indicates that data can be saved
        uint8_t  buffRecords;                                               // Number of the records in the outpt buffer
        uint16_t rAddr;                                                     // Address of thecorrect record in EEPROM to be read
        uint16_t wAddr;                                                     // Address in the EEPROM to start write new record
        uint16_t eLength;                                                   // Length of the EEPROM, depends on arduino model
        uint32_t nextRecID;                                                 // next record ID
        uint8_t  record_size;                                               // The size of one record in bytes
};

//------------------------------------------ class HOT GUN CONFIG ----------------------------------------------
class HOTGUN_CFG : public CONFIG {
    public:
        HOTGUN_CFG()                                                        { }
        void        init(void);
        uint16_t    tempPreset(void);                                       // The preset temperature in internal units
        uint16_t    fanPreset(void);                                        // The preset fan speed 0 - 255 
        uint16_t    tempInternal(uint16_t temp);                            // Translate the human readable temperature into internal value
        uint16_t    tempHuman(uint16_t temp);                               // Translate temperature from internal units to the Celsius
        uint8_t     dsplBright(void)                                        { return Config.dspl_bright; }
        void        saveBright(uint8_t br)                                  { Config.dspl_bright = br;   }
        void        save(uint16_t temp, uint16_t fanSpeed);                 // Save preset temperature in the internal units and fan speed
        void        applyCalibrationData(uint16_t tip[3]);
        void        getCalibrationData(uint16_t tip[3]);
        void        saveCalibrationData(uint16_t tip[3]);
        void        setDefaults(bool Write);                                // Set default parameter values if failed to load data from EEPROM
    private:
        uint16_t t_tip[3];
        const   uint16_t def_tip[3] = {587, 751, 850};                      // Default values of internal sensor readings at reference temperatures
        const   uint16_t min_temp   = 50;
        const   uint16_t max_temp   = 900;
        const   uint16_t def_temp   = 600;                                  // Default preset temperature
        const   uint16_t def_fan    = 200;                                  // Default preset fan speed 0 - max_fan_speed
        const   uint8_t  def_br     = 128;                                  // Default display brightness
        const   uint16_t ambient_temp = 67;
        const   uint16_t ambient_tempC= 25;
};

#endif
