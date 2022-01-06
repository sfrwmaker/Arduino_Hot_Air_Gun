#ifndef _VARS_H_
#define _VARS_H_

#include <stdint.h>

extern const uint16_t temp_minC;                // Maximum possible temperature
extern const uint16_t temp_maxC;                // Maximum possible temperature
extern const uint16_t temp_ambC;                // Average ambient temperature
extern const uint16_t temp_max;                 // Maximum possible temperature in internal units
extern const uint16_t temp_tip[3];              // Temperature reference points for calibration
extern const uint16_t max_fan_speed;            // Maximum Hot Air Gun Fan speed
extern const uint16_t min_fan_speed;
extern const uint16_t tune_fan_speed;

#endif
