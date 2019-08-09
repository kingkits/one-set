/*
	File:flow_sensor_simple_calibration.h
 */
#include "stm32f4xx_hal.h"
#ifndef __FLOW_SENSOR_SIMPLE_CALIBRATION_H
#define __FLOW_SENSOR_SIMPLE_CALIBRATION_H

#if SIMPLE_CLIBRATION_FLOW_ENABLE

typedef enum
{
    EM_SIMPLE_CALIBRATION_FLOW_RESET,
    EM_SIMPLE_CALIBRATION_FLOW_START,
    EM_SIMPLE_CALIBRATION_FLOW_INCREASE,
    EM_SIMPLE_CALIBRATION_FLOW_STOP
} __SIMPLE_CLIBRATION_FLOW_WORK_STATUS;

typedef struct __SIMPLE_CLIBRATION_FLOW_WORK_DATA
{
    uint32_t time_start;
    uint16_t count;
    uint8_t  status;
    uint16_t peep_val;
    uint16_t speed;

    // temp data
    uint32_t flow_sum;
    uint32_t real_sum;
    uint32_t tsiV_sum;
	uint32_t tsiT_sum;

    uint32_t sensor_low_sum;
    uint32_t sensor_high_sum;
    uint16_t sum_count;
} SIMPLE_CLIBRATION_FLOW_WORK_DATA;
extern SIMPLE_CLIBRATION_FLOW_WORK_DATA simple_calibration_flow_work_data;
#endif

void reset_simple_calibration_flow_data(void);
void do_simple_calibration_flow_work(void);
void simple_calibration_flow_start(void);

#endif

