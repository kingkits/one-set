// File: Blower.h
#ifndef __BLOWER_H
#define __BLOWER_H
#include "stm32f4xx_hal.h"
#include "cmsis_os.h"

#include "stm32f4xx_hal_def.h"

#include "../lib/PID.h"

// for work status
typedef enum
{
    EM_BLOWER_STOP = 0,  // speed = 0
    EM_BLOWER_RUNNING,   // in working
    EM_BLOWER_ERROR      // not in control
} __ENUM_BLOWER_WORK_SATTUS_DEFINES;

typedef enum
{
    EM_FORWARD,
    EM_REWARD
} __ENUM_BLOWER_DIRECTION;

typedef struct _ST_BLOWER_CONTROL_DATA
{
    // status
    uint8_t work_status; // 工作状态

    uint16_t temperature_status; // 温度状态
    // control
    uint8_t direction;
    uint16_t set_speed;

    // pid data
    PID_t *pid_ptr;

    // pwm
    uint16_t control_data;
    uint16_t overflow_control_data;
} ST_BLOWER_CONTROL_DATA;
extern ST_BLOWER_CONTROL_DATA blower_main;


typedef struct _ST_BLOWER_STATUS
{
    uint8_t actived_flag;
    uint16_t speed; // 速度状态

    // 中间变量
    // 16秒的数据
    uint32_t speed_s;//[16];
    //数据指针
    int data_ptr;
} ST_BLOWER_STATUS;
extern ST_BLOWER_STATUS blower_status;



void reset_blower(void);
void set_blower_speed(uint16_t speed);
void Blower_enable(void);
void Blower_disable(void);

void Overflow_enable(void);
void Overflow_disable(void);

void set_overflow_valve(uint16_t data);
#endif

// file end: Blower.h
