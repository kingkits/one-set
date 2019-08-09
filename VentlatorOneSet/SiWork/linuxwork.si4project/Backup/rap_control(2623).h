// File: rap_control.h

#include "stm32f4xx_hal.h"
#ifndef __RAP_CONTROL_H
#define __RAP_CONTROL_H
#define reset_rap_actions()  (rap_control_action_flag = EM_RAP_ACTION_IDLE)

uint8_t rap_control_action_flag;


#endif

