// File:cough_control.c
#include "stm32f4xx_hal.h"
#include "../ControlTask/base_control.h"
#include "../global.h"
#include "breath_rap.h"
#include "../blower/blower.h"
#include "../adc/adc_manager.h"
#include "../lib/leak_flow_count.h"
#include "../lib/VentLIB.h"
#include "breath_Atomizer.h"
#include "breath_cycle_adjust.h"
#include "pcv_press_increase_data.c"
#include "pcv_control.h"
#include "trigger_breath.h"
#include "../alarm/alarm.h"



// cough inhale
// normal inhale
// 普通吸气（上一呼吸参数）
void do_cough_normal_inhale_action(void)
{
}

// 用力吸气（胸部扩张）
void do_cough_force_inhale_action(void)
{
}


// 咳痰--前置吸气（咳痰准备）
void do_cough_real_inhale_action(void)
{
}

// cough exhale
// 普通吸气（上一呼吸参数）
void do_cough_normal_exhale_action(void)
{
}

// 用力吸气（胸部扩张）
void do_cough_force_exhale_action(void)
{
}


// 咳痰--前置吸气（咳痰准备）
void do_cough_real_exhale_action(void)
{
}


// file cough_control.c end

