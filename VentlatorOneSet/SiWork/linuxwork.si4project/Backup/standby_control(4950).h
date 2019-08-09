// File: standby_control.h

#include "stm32f4xx_hal.h"
#ifndef __STANDBY_CONTROL_H
#define __STANDBY_CONTROL_H
#define reset_rap_actions()  (rap_control_action_flag = EM_RAP_ACTION_IDLE)

uint8_t rap_control_action_flag;


#endif

