//file OxygenMixter.h
#ifndef __OXYGENmixer_H
#define __OXYGENmixer_H
#include "stm32f4xx_hal.h"
#include "cmsis_os.h"
#include "arm_math.h"
#include "stm32f4xx_hal_def.h"

#include "../lib/PID.h"

#define OXYGEN_21_DATA 0x208
#define OXYGEN_100_DATA 0x450
extern uint8_t Oxygenmixer_actived_flag;
typedef struct _ST_OXYGEN_CONTROL_DATA
{
    // status
    uint8_t work_status; // 工作状态

    // control
    float64_t set_concentration;
    // pid data
    PID_t *pid_ptr;

    // pwm
    uint16_t control_data;
    float64_t base_val;
    float64_t adjust_mul;
    float64_t adjust_add;
} ST_OXYGEN_CONTROL_DATA;

typedef enum
{
    ENUM_PID_OXYGEN_MIXER_NOT_ACTIVED,        // 未工作
    ENUM_PID_OXYGEN_CONCENTRATION_LOW_START,  // 从低于设置的氧浓度值开始
    ENUM_PID_OXYGEN_CONCENTRATION_HIGH_START, // 从高于设置的氧浓度值开始
    ENUM_PID_OXYGEN_MIXER_ACTIVED             // 正常工作
} ENUM_OXYGEN_MIXER_PID_WORK_MOED_DEFINES;

typedef struct _ST_OXYGEN_MIXER_PID
{
    uint8_t time_count;
    uint8_t mode; //ENUM_OXYGEN_MIXER_PID_WORK_MOED_DEFINES
    PID_t pid;
} ST_OXYGEN_MIXER_PID;

void set_concentration_value(uint8_t val);

extern ST_OXYGEN_CONTROL_DATA oxygen_mixer_ctrol;
void oxygen_pid_reset(void);
void oxygen_pid_adjust(void);
void oxygen_pid_refresh_mode(uint8_t mode);
void oxygen_adjust(void);
void oxygen_start(void);
void oxygen_stop(void);

void reset_oxygen_valve_control_data(void);
void set_oxygen_valve_control_val(uint16_t val);
#endif
// ----- Oxygenmixer.h end

