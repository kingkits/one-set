// File: breath_cough_mode.h
#ifndef __BREATH_COUGH_MODE_H
#define __BREATH_COUGH_MODE_H
#include "stm32f4xx_hal.h"

#define COUGH_VALVE_OPEN() open_valve(EM_VALVE_CHANEL_6)
#define COUGH_VALVE_CLOSE() close_valve(EM_VALVE_CHANEL_6)
#define INHALE_VALVE_OPEN() close_valve(EM_VALVE_CHANEL_3)
#define INHALE_VALVE_CLOSE() open_valve(EM_VALVE_CHANEL_3)
void init_vacuum_pump_control_data(void);

#endif
void start_cough_blower_work(void);


