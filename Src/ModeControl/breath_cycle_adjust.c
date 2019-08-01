// File: breath_cycle_adjust.c
#include "stm32f4xx_hal.h"
#include "../ControlTask/base_control.h"
#include "../global.h"

#include "breath_cycle_adjust.h"
typedef struct __BREATH_CYCLES_ADJUST_DATA
{
    // *******************************
    // status
    // *******************************
    // press
    int32_t last_press_e;
    int32_t last_press_i;
    int32_t last_press_t;

    // flow
    int32_t last_flow_e;
    int32_t last_flow_i;
    int32_t last_flow_t;

    // *******************************
    // Control data
    // *******************************
    // PEEP valve
    uint16_t last_peep_e;
    uint16_t last_peep_i;
    uint16_t last_peep_t;

    // blower
    uint16_t last_speed_e;
    uint16_t last_speed_i;
    uint16_t last_speed_t;
    //uint16_t last_speed_increse_step;

    // 回流阀

    // 负压风机
} BREATH_CYCLES_ADJUST_DATA;

BREATH_CYCLES_ADJUST_DATA breath_cycles_adjust_data;

void breath_cycles_set_direct(int16_t last_peep, int16_t P_peak)
{
    breath_cycles_adjust_data.last_press_e = last_peep;
    breath_cycles_adjust_data.last_press_i = P_peak;
    breath_cycles_adjust_data.last_press_t = P_peak;
}
uint8_t is_breath_cycles_adjust_data_ready(void)
{
    if(breath_cycles_adjust_data.last_press_e == 0)
        return 0;
    if(breath_cycles_adjust_data.last_flow_e == 0)
        return 0;
    return 1;
}
// 清零
void init_breath_cycles_adjust_data(void)
{
    breath_cycles_adjust_data.last_press_e  = 0;
    breath_cycles_adjust_data.last_press_i  = 0;
    breath_cycles_adjust_data.last_press_t  = 0;

    breath_cycles_adjust_data.last_flow_e  = 0;
    breath_cycles_adjust_data.last_flow_i  = 0;
    breath_cycles_adjust_data.last_flow_t  = 0;


    breath_cycles_adjust_data.last_peep_e  = 0;
    breath_cycles_adjust_data.last_peep_i  = 0;
    breath_cycles_adjust_data.last_peep_t  = 0;

    breath_cycles_adjust_data.last_speed_e = 0;
    breath_cycles_adjust_data.last_speed_i = 0;
    breath_cycles_adjust_data.last_speed_t = 0;
}


void breath_cycles_save_peep_data(uint8_t types, uint16_t peep_val, int32_t flow_val)
{
    switch(types)
    {
    default:
    case EM_BREATH_ADJUST_SAVE_NOTHING:
        break;
    case EM_BREATH_ADJUST_SAVE_E_CONTROL:
        breath_cycles_adjust_data.last_peep_e = peep_val;
        breath_cycles_adjust_data.last_flow_e = flow_val;
        break;
    case EM_BREATH_ADJUST_SAVE_I_CONTROL:
    case EM_BREATH_ADJUST_SAVE_T_CONTROL:
        breath_cycles_adjust_data.last_peep_i = peep_val;
        breath_cycles_adjust_data.last_flow_i = flow_val;
        breath_cycles_adjust_data.last_peep_t = peep_val;
        breath_cycles_adjust_data.last_flow_t = flow_val;
        break;
    }
}

uint16_t breath_cycles_adjust_peep_data(uint8_t types, int32_t target_flow)
{
    int32_t flow_diff;
    uint32_t abs_diff;
    uint16_t last_peep_val;
    switch(types)
    {
    default:
    case EM_BREATH_ADJUST_SAVE_NOTHING:
        return breath_cycles_adjust_data.last_peep_e;
    case EM_BREATH_ADJUST_SAVE_E_CONTROL:
        flow_diff = target_flow - breath_cycles_adjust_data.last_flow_e;
        last_peep_val = breath_cycles_adjust_data.last_peep_e;
        break;
    case EM_BREATH_ADJUST_SAVE_I_CONTROL:
        flow_diff = target_flow - breath_cycles_adjust_data.last_flow_i;
        last_peep_val = breath_cycles_adjust_data.last_peep_i;
        break;
    case EM_BREATH_ADJUST_SAVE_T_CONTROL:
        flow_diff = target_flow - breath_cycles_adjust_data.last_flow_t;
        last_peep_val = breath_cycles_adjust_data.last_peep_t;
        break;
    }

    flow_diff /= 1000;
    if(flow_diff > 0)
        abs_diff = flow_diff;
    else
        abs_diff = -flow_diff;

    // 偏差小于1升/分
    switch(abs_diff)
    {
    case 0:
        break;
    case 1:
        if(flow_diff > 0)
        {
            last_peep_val -= 3;
        }
        else
        {
            last_peep_val += 2;
        }
        break;
    case 2:
        if(flow_diff > 0)
        {
            last_peep_val -= 5;
        }
        else
        {
            last_peep_val += 3;
        }
        break;
    case 4:
        if(flow_diff > 0)
        {
            last_peep_val -= 7;
        }
        else
        {
            last_peep_val += 3;
        }
        break;
    case 8:
        if(flow_diff > 0)
        {
            last_peep_val -= 11;
        }
        else
        {
            last_peep_val += 5;
        }
        break;
    default:
        if(flow_diff > 0)
        {
            last_peep_val -= 13;
        }
        else
        {
            last_peep_val += 5;
        }
        break;
    }

    if(last_peep_val <  PEEP_VALVE_MIN_SET_VALUE)
    {
        last_peep_val =  PEEP_VALVE_MIN_SET_VALUE;
    }
    if(last_peep_val > PEEP_VALVE_MAX_SET_VALUE)
    {
        last_peep_val = PEEP_VALVE_MAX_SET_VALUE;
    }
    return last_peep_val;
}

void breath_cycles_save_blower_speed_data(uint8_t types, uint16_t speed_val, int32_t press_val)
{
    switch(types)
    {
    default:
    case EM_BREATH_ADJUST_SAVE_NOTHING:
        break;
    case EM_BREATH_ADJUST_SAVE_E_CONTROL:
        breath_cycles_adjust_data.last_speed_e = speed_val;
        breath_cycles_adjust_data.last_press_e = press_val;
        break;
    case EM_BREATH_ADJUST_SAVE_I_CONTROL:
    case EM_BREATH_ADJUST_SAVE_T_CONTROL:
        breath_cycles_adjust_data.last_speed_i = speed_val;
        breath_cycles_adjust_data.last_press_i = press_val;
        breath_cycles_adjust_data.last_speed_t = speed_val;
        breath_cycles_adjust_data.last_press_t = press_val;
        break;
    }
}

uint16_t breath_cycles_adjust_speed_data(uint8_t types, int32_t target_press)
{
    int32_t press_diff;
    uint32_t abs_diff;
    int32_t last_speed_val;
    switch(types)
    {
    default:
    case EM_BREATH_ADJUST_SAVE_NOTHING:
        return breath_cycles_adjust_data.last_speed_e;
    case EM_BREATH_ADJUST_SAVE_E_CONTROL:
        press_diff = target_press - breath_cycles_adjust_data.last_press_e;
        last_speed_val = (int32_t)breath_cycles_adjust_data.last_speed_e;
        break;
    case EM_BREATH_ADJUST_SAVE_I_CONTROL:
        press_diff = target_press - breath_cycles_adjust_data.last_press_i;
        last_speed_val = (int32_t)breath_cycles_adjust_data.last_speed_i;
        break;
    case EM_BREATH_ADJUST_SAVE_T_CONTROL:
        press_diff = target_press - breath_cycles_adjust_data.last_press_t;
        last_speed_val = (int32_t)breath_cycles_adjust_data.last_speed_t;
        break;
    }
    if(press_diff > 0)
    {
        abs_diff = press_diff;
    }
    else
    {
        abs_diff = -press_diff;
    }


    if(abs_diff < 20)
    {
        ;
    }
    else if(abs_diff < 30)
    {
        last_speed_val += press_diff * 3;
    }
    else if(abs_diff < 40)
    {
        last_speed_val += press_diff * 10;
    }
    else if(press_diff < 0)
    {
        last_speed_val -= 500;
    }
    else last_speed_val += 500;

    //    if(last_speed_val <  MIN_BLOWER_SPEED)
    //    {
    //        last_speed_val =  MIN_BLOWER_SPEED;
    //    }
    //    if(last_speed_val > MAX_BLOWER_SPEED)
    //    {
    //        last_speed_val = MAX_BLOWER_SPEED;
    //    }
    NO_LESS_THAN(last_speed_val, MIN_BLOWER_SPEED);
    NO_MORE_THAN(last_speed_val, MAX_BLOWER_SPEED);
    return (uint16_t) last_speed_val;
}

// end of flie breath_cycle_adjust.c
