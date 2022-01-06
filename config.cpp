#include <Arduino.h>
#include "config.h"

//------------------------------------------ Configuration data ------------------------------------------------
/* Config record in the EEPROM has the following format:
 * uint32_t ID                           each time increment by 1
 * struct cfg                            config data, 9 bytes
 * byte CRC                              the checksum
*/
// Read the records until the last one, point wAddr (write address) after the last record
void CONFIG::init(void) {
    eLength = EEPROM.length();
    uint32_t recID;
    uint32_t minRecID = 0xffffffff;
    uint16_t minRecAddr = 0;
    uint32_t maxRecID = 0;
    uint16_t maxRecAddr = 0;
    uint8_t  records = 0;

    nextRecID = 0;

    // read all the records in the EEPROM find min and max record ID
    for (uint16_t addr = 0; addr < eLength; addr += record_size) {
        if (readRecord(addr, recID)) {
            ++records;
            if (minRecID > recID) {
                minRecID = recID;
                minRecAddr = addr;
            }
            if (maxRecID < recID) {
                maxRecID = recID;
                maxRecAddr = addr;
            }
        } else {
            break;
        }
    }

    if (records == 0) {
        wAddr = rAddr = 0;
        can_write = true;
        return;
    }

    rAddr = maxRecAddr;
    if (records < (eLength / record_size)) {                                // The EEPROM is not full
        wAddr = rAddr + record_size;
        if (wAddr > eLength) wAddr = 0;
    } else {
        wAddr = minRecAddr;
    }
    can_write = true;
}

void CONFIG::getConfig(struct cfg &Cfg) {
    memcpy(&Cfg, &Config, sizeof(struct cfg));
}

void CONFIG::updateConfig(struct cfg &Cfg) {
    memcpy(&Config, &Cfg, sizeof(struct cfg));
}

bool CONFIG::saveConfig(struct cfg &Cfg) {
    updateConfig(Cfg);
    return save();                                                          // Save new data into the EEPROM
}

bool CONFIG::save(void) {
    if (!can_write) return can_write;
    if (nextRecID == 0) nextRecID = 1;

    uint16_t startWrite = wAddr;
    uint32_t nxt = nextRecID;
    uint8_t summ = 0;
    for (uint8_t i = 0; i < 4; ++i) {
        EEPROM.write(startWrite++, nxt & 0xff);
        summ <<=2; summ += nxt;
        nxt >>= 8;
    }
    uint8_t* p = (uint8_t *)&Config;
    for (uint8_t i = 0; i < sizeof(struct cfg); ++i) {
        summ <<= 2; summ += p[i];
        EEPROM.write(startWrite++, p[i]);
    }
    summ ++;                                                                // To avoid empty records
    EEPROM.write(wAddr+record_size-1, summ);

    rAddr = wAddr;
    wAddr += record_size;
    if (wAddr > EEPROM.length()) wAddr = 0;
    nextRecID ++;                                                           // Get ready to write next record
    return true;
}

bool CONFIG::load(void) {
    bool is_valid = readRecord(rAddr, nextRecID);
    nextRecID ++;
    return is_valid;
}

bool CONFIG::readRecord(uint16_t addr, uint32_t &recID) {
    uint8_t Buff[record_size];

    for (uint8_t i = 0; i < record_size; ++i) 
        Buff[i] = EEPROM.read(addr+i);
  
    uint8_t summ = 0;
    for (uint8_t i = 0; i < sizeof(struct cfg) + 4; ++i) {
        summ <<= 2; summ += Buff[i];
    }
    summ ++;                                                                // To avoid empty fields
    if (summ == Buff[record_size-1]) {                                      // Checksumm is correct
        uint32_t ts = 0;
        for (char i = 3; i >= 0; --i) {
            ts <<= 8;
            ts |= Buff[uint8_t(i)];
        }
        recID = ts;
        memcpy(&Config, &Buff[4], sizeof(struct cfg));
        return true;
    }
    return false;
}

//------------------------------------------ class HOT GUN CONFIG ----------------------------------------------
void HOTGUN_CFG::init(void) {
    CONFIG::init();
    if (!CONFIG::load()) setDefaults(false);                                // If failed to load the data from EEPROM, initialize the config data with the default values
    uint32_t   cd = Config.calibration;
    t_tip[0] = cd & 0x3FF; cd >>= 10;                                       // 10 bits per calibration parameter, because the ADC readings are 10 bits
    t_tip[1] = cd & 0x3FF; cd >>= 10;
    t_tip[2] = cd & 0x3FF;
    // Check the tip calibration is correct
    if ((t_tip[0] >= t_tip[1]) || (t_tip[1] >= t_tip[2])) {
        setDefaults(false);
        for (uint8_t i = 0; i < 3; ++i)
            t_tip[i] = def_tip[i];
    }
    return;
}

uint16_t HOTGUN_CFG::tempPreset(void) {
    return Config.temp;
}

uint16_t HOTGUN_CFG::fanPreset(void) {
    return Config.fan;
}

uint16_t HOTGUN_CFG::tempInternal(uint16_t t) {                             // Translate the human readable temperature into internal value
    t = constrain(t, temp_minC, temp_maxC);
    uint16_t left   = 0;
    uint16_t right  = 1023;                                                 // Maximum temperature value in internal units
    uint16_t temp = map(t, temp_tip[0], temp_tip[2], t_tip[0], t_tip[2]);

    if (temp > (left+right)/ 2) {
        temp -= (right-left) / 4;
    } else {
        temp += (right-left) / 4;
    }

    for (uint8_t i = 0; i < 20; ++i) {
        uint16_t tempH = tempHuman(temp);
        if (tempH == t) {
            return temp;
        }
        uint16_t new_temp;
        if (tempH < t) {
            left = temp;
             new_temp = (left+right)/2;
            if (new_temp == temp)
                new_temp = temp + 1;
        } else {
            right = temp;
            new_temp = (left+right)/2;
            if (new_temp == temp)
                new_temp = temp - 1;
        }
        temp = new_temp;
    }
    return temp;
}

// Thanslate temperature from internal units to the human readable value (Celsius or Fahrenheit)
uint16_t HOTGUN_CFG::tempHuman(uint16_t temp) {
    uint16_t tempH = 0;
    
    if (temp <= ambient_temp) {
        tempH = ambient_tempC;
    } else if (temp < t_tip[0]) {
        tempH = map(temp, ambient_temp, t_tip[0], ambient_tempC, temp_tip[0]);
    } else if (temp <= t_tip[1]) {
        tempH = map(temp, t_tip[0], t_tip[1], temp_tip[0], temp_tip[1]);
    } else if (temp <= t_tip[2]) {
        tempH = map(temp, t_tip[1], t_tip[2], temp_tip[1], temp_tip[2]);
    } else {
        tempH = map(temp, t_tip[0], t_tip[2], temp_tip[0], temp_tip[2]);
    }
    return tempH;
}

void HOTGUN_CFG::save(uint16_t temp, uint16_t fanSpeed) {
    Config.temp        = constrain(temp, min_temp, max_temp);
    Config.fan         = fanSpeed;
    CONFIG::save();                                                         // Save new data into the EEPROM
}

void HOTGUN_CFG::applyCalibrationData(uint16_t tip[3]) {
    if (tip[0] < ambient_temp) {
        uint16_t t = ambient_temp + tip[1];
        tip[0] = t >> 1;
    }
    t_tip[0] = tip[0];
    t_tip[1] = tip[1];
    if (tip[2] > max_temp) tip[2] = max_temp; 
    t_tip[2] = tip[2];
}

void HOTGUN_CFG::getCalibrationData(uint16_t tip[3]) {
    tip[0] = t_tip[0];
    tip[1] = t_tip[1];
    tip[2] = t_tip[2];
}

void HOTGUN_CFG::saveCalibrationData(uint16_t tip[3]) {
    if (tip[2] > max_temp) tip[2] = max_temp;
    uint32_t cd = tip[2] & 0x3FF; cd <<= 10;                                // Pack tip calibration data in one 32-bit word: 10-bits per value
    cd |= tip[1] & 0x3FF; cd <<= 10;
    cd |= tip[0];
    Config.calibration = cd;
    t_tip[0] = tip[0];
    t_tip[1] = tip[1];
    t_tip[2] = tip[2];
}

void HOTGUN_CFG::setDefaults(bool Write) {
    uint32_t c = def_tip[2] & 0x3FF; c <<= 10;
    c |= def_tip[1] & 0x3FF;         c <<= 10;
    c |= def_tip[0] & 0x3FF;
    Config.calibration  = c;
    Config.temp         = def_temp;
    Config.fan          = def_fan;
    Config.dspl_bright  = def_br;
    if (Write) {
        CONFIG::save();
    }
}
