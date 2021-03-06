// File: trigger_breath.h

#include "stm32f4xx_hal.h"

#ifndef __TRIGGER_BREATH_H
#define __TRIGGER_BREATH_H

typedef enum
{
    EM_TRIGGER_TYPE_IDLE              = 0, // 未工作
    EM_TRIGGER_TYPE_E_EXP             = 1, // 水平压力，检测主动呼气
    EM_TRIGGER_TYPE_E_INS             = 2, // 水平压力，检测主动吸气
    EM_TRIGGER_TYPE_E_EXP_INS         = 3, // 未知通气状态，检测双向（CPAP）（待定）
    EM_TRIGGER_TYPE_CPAP_INS_FINISHED = 4, // CPAP 检测吸气结束
    EM_TRIGGER_TYPE_CPAP_EXP_FINISHED = 5, // CPAP 检测呼气结束
    EM_TRIGGER_TYPE_STV_INS_FINISHED  = 6, // ST 检测吸气结束
    EM_TRIGGER_TYPE_STV_EXP_FINISHED  = 7, // ST 检测呼气结束
    EM_TRIGGER_TYPE_DEFINES_ENDS           // 结束标记，用于可能的需要
} ENUM_TRIGGER_DETECTED_TYPES;

#define TRIGGER_INHALE_FLOW_GATE   2000
#define TRIGGER_EXHALE_FLOW_GATE  -2000
#define TRIGGER_INHALE_PRESS_GATE  20
#define TRIGGER_EXHALE_PRESS_GATE -20

void    stop_trigger(void);
void    start_trigger(uint8_t type);
uint8_t is_patient_actived_breath(void);
void    trigger_set_current_status(void);
void    trigger_simple_set_base_inhale(int32_t flow, int16_t press);
void    trigger_simple_set_base_exhale(int32_t flow1, int16_t press);
void    trigger_force_confirm_type(uint8_t type);

#endif//__TRIGGER_BREATH_H

