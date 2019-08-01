// File: rap_control.h

#include "stm32f4xx_hal.h"
#ifndef __RAP_CONTROL_H
#define __RAP_CONTROL_H
typedef enum
{
    EM_RAP_ACTION_IDLE,
    EM_RAP_ACTION_ACTIVE,
    EM_RAP_ACTION_STOP
} ENUM_RAP_WORK_STATUS_TYPES;

#define reset_rap_actions()  (rap_control_action_flag = EM_RAP_ACTION_IDLE)
#define is_rap_in_work() (rap_control_action_flag == EM_RAP_ACTION_ACTIVE)
extern uint8_t rap_control_action_flag;
void do_rap_actions(int type);

#endif

