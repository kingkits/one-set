// File: cpap_control.h

#include "stm32f4xx_hal.h"
#ifndef __CPAP_CONTROL_H
#define __CPAP_CONTROL_H
typedef struct __ST_CPAP_CONTROL_DAT
{
    uint32_t cycles;
    uint32_t inhale_time_protected_gate; // 吸气时间保护门限
    uint32_t exhale_time_protected_gate; // 呼气时间保护门限
    uint16_t current_speed;  // current work speed 还没想好怎么用，感觉应该有这样一个变量
    uint16_t low_speed; 	 // speed for peep     在没有压力补偿时，使用这个
    uint16_t c_flex_speed;	 // speed for cflex    压力释放的强度
    uint16_t inhale_speed;	 // speed for inhale   吸气时的补偿（或许需要一个函数或PID，用于动态补偿）
    uint16_t exhale_speed;	 // speed for exhale   呼气时的补偿（或许需要一个函数或PID，用于动态补偿）
    uint16_t inhale_speed_adjust;
    uint16_t exhale_speed_adjust;

    uint16_t protect_active_flag; // 用于保障最小吸呼气时间，还有避免可能的来自自身控制产生的压力干扰

    uint16_t peep_val;
    // inhale Press low
    int16_t inhale_press_low; // 吸气相的最低压力（用于调整涡轮风机）
    int16_t exhale_press_high;// 呼气相的最高压力（用于调整涡轮风机）

    //  时间戳
    uint32_t time_stemp; // 暂时没有用，先放这里

    // 调用次数
    uint16_t entry_cycles;
} ST_CPAP_CONTROL_DAT;
extern ST_CPAP_CONTROL_DAT cpap_control_dat;

// function protocol
uint16_t get_cpap_c_flex_speed(void);

#endif
