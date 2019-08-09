// File: st_control.h

#include "stm32f4xx_hal.h"
#ifndef __ST_CONTROL_H
#define __ST_CONTROL_H

#define CONST_ST_EXHALE_FLOW 30000
typedef struct __ST_STV_CONTROL_DAT
{
    // 基础数据
    uint32_t cycles;
    // 基础数据-- 时间    -->使用PCV的时间数据
    uint32_t inhale_time_protected_gate; // 吸气时间保护门限
    uint32_t exhale_time_protected_gate; // 呼气时间保护门限
    uint8_t protect_active_flag;
    // 基础数据-- 涡轮速度
    uint16_t high_speed;   // S mode High-speed
    uint16_t low_speed;    // S mode Low-speed
    uint16_t C_Flex_speed; // S mode C-Flex-speed	暂时不用
    uint16_t current_speed;
    // S特有
    int32_t inhale_peak_flow;
    // 模式控制-- 压力保持

    // 模式控制-- 压力下降（呼气）

    // 模式控制-- 触发

    // 模式控制-- 叩击-->使用PCV的数据

    // blower 调整
    int16_t inhale_press_low; // 吸气相的最低压力（用于调整涡轮风机）
    int16_t exhale_press_high;// 呼气相的最高压力（用于调整涡轮风机）

    uint8_t err_blower_speed;
} ST_STV_CONTROL_DAT;

extern ST_STV_CONTROL_DAT stv_control_dat;
#endif

