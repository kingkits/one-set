// file:breath_Atomizer.c
#include "../global.h"
#include "breath_Atomizer.h"
#include "../alarm/alarm.h"
//static uint32_t nebulizer_time_stemp;
static uint8_t nebulizer_valve_work_status;
static void system_change_nebulizer(uint8_t status)
{
    if(status == EM_ON)
    {
        nebulizer_valve_work_status = EM_ON;
        open_valve(DEFAULT_NEBULIZER_VALVE_CHANNEL);
    }
    else
    {
        nebulizer_valve_work_status = EM_OFF;
        close_valve(DEFAULT_NEBULIZER_VALVE_CHANNEL);
    }
}

uint8_t is_nebulizer_valve_open(void)
{
    return nebulizer_valve_work_status;
}

void breath_turn_on_nebulizer(void)
{
    //set_nebulizer_work_status(EM_ON);

    // 操作雾化器
    system_change_nebulizer(EM_ON);
}

void breath_turn_off_nebulizer(void)
{
    //set_nebulizer_work_status(EM_OFF);

    // 操作雾化器
    system_change_nebulizer(EM_OFF);
}

void breath_change_nebulizer(uint8_t status)
{
    if(!status)
        breath_turn_off_nebulizer();
    else
        breath_turn_on_nebulizer();
}

void breath_force_operation_nebulizer(uint8_t status)
{
    if(status == EM_ON)
    {
        if(!is_breath_nebulizer_turned_on())return;
        else system_change_nebulizer(EM_ON);// 操作雾化器
    }
    else
    {
        // 直接操作雾化器
        system_change_nebulizer(EM_OFF);
    }
}

// end of file:Atomizer.c

