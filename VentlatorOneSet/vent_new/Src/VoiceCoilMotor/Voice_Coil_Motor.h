#ifndef __VOICE_COIL_MOTOR_H
#define __VOICE_COIL_MOTOR_H
#include "stm32f4xx_hal.h"
typedef struct __VOICE_COIL_MOTOR_BASE
{
    uint16_t open_val;
    uint16_t full_val;
    double   scale;
    uint32_t chanel;
    uint16_t current_set; // 用千分比来表示会更精确 0-4000
} VOICE_COIL_MOTOR_BASE;


/*
        a    b     c   d
          ________
         /        \
		/          \________

 */
typedef struct __VOICE_COIL_MOTOR_CONTROL
{
    VOICE_COIL_MOTOR_BASE base;
    // 状态
    uint8_t               mode;           // 模式
    uint8_t               status;         // 当前的状态
    uint16_t              cycles;         // 当前运行的循环数
    // 控制 A 段 上升段
    uint16_t              control_a_val;  // 每次调整增量值 0-4000
    uint16_t              control_a_time; // 控制时间
    // 控制 B 段 第一保持段（高位）
    uint16_t              control_b_val;  // 保持值 0-4000
    uint16_t              control_b_time; // 保持时间
    // 控制   C 段 下降段
    uint16_t              control_c_val;  // 每次调整增量值 0-4000
    uint16_t              control_c_time; // 控制时间
    // 控制 D 段 第二保持段（低位）
    uint16_t              control_d_val;  // 保持值 0-4000
    uint16_t              control_d_time; // 保持时间

    //当前的记录 （运行时的临时变量，有些特殊控制，需要临时设定/更改）
    uint16_t              current_control_val; // 当前阀控制值
    uint16_t              current_clk;    // 当前的计数
    uint16_t              target_clk;     // 目标计数
} VOICE_COIL_MOTOR_CONTROL;
//extern VOICE_COIL_MOTOR_CONTROL voice_coil_motor_control_data;
typedef enum
{
    EM_VOICE_COIL_MOTOR_STOP, // 停止状态
    EM_VOICE_COIL_MOTOR_A_PERIOD,
    EM_VOICE_COIL_MOTOR_B_PERIOD,
    EM_VOICE_COIL_MOTOR_C_PERIOD,
    EM_VOICE_COIL_MOTOR_D_PERIOD,
    EM_VOICE_COIL_MOTOR_NEXT_LOOPS
} __ENUM_VOICE_COIL_MOTOR_ACTION_TYPES;
typedef enum
{
    EM_VOICE_COIL_MOTOR_BANK_A,
    EM_VOICE_COIL_MOTOR_BANK_B,
    EM_VOICE_COIL_MOTOR_BANK_C,
    EM_VOICE_COIL_MOTOR_BANK_D,
    EM_VOICE_COIL_MOTOR_BANK_X
} __ENUM_VOICE_COIL_MOTOR_DATA_TYPES;

typedef enum
{
    EM_VOICE_COIL_MOTOR_MODE_STANDBY,
    EM_VOICE_COIL_MOTOR_MODE_RUNNING,
    EM_VOICE_COIL_MOTOR_MODE_PAUSE,
    EM_VOICE_COIL_MOTOR_MODE_STATIC_WORKING
} __ENUM_VOICE_COIL_MOTOR_MODE_TYPES;


#endif
