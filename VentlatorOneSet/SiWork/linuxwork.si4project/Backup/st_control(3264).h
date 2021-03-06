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
    // 基础数据-- 涡轮速度
    uint16_t high_speed;   // S mode High-speed
    uint16_t low_speed;    // S mode Low-speed
    uint16_t C_Flex_speed; // S mode C-Flex-speed	暂时不用

    // 模式控制-- 压力保持

    // 模式控制-- 压力下降（呼气）

    // 模式控制-- 触发

    // 模式控制-- 叩击-->使用PCV的数据
} ST_STV_CONTROL_DAT;

extern ST_STV_CONTROL_DAT stv_control_dat;
#endif

