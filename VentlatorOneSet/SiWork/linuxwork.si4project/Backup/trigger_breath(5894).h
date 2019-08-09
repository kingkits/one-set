// File: trigger_breath.h

#include "stm32f4xx_hal.h"

#ifndef __TRIGGER_BREATH_H
#define __TRIGGER_BREATH_H

typedef enum
{
    EM_TRIGGER_TYPE_IDLE,
    EM_TRIGGER_TYPE_E_EXP     = 1, // 水平压力，检测主动呼气
    EM_TRIGGER_TYPE_E_INS     = 2, // 水平压力，检测主动吸气
    EM_TRIGGER_TYPE_E_EXP_INS = 3
} ENUM_TRIGGER_DETECTED_TYPES;
void stop_trigger(void);
void start_trigger(uint8_t type);
uint8_t is_patient_actived_breath(void);
void trigger_set_current_status(void);

#endif//__TRIGGER_BREATH_H

