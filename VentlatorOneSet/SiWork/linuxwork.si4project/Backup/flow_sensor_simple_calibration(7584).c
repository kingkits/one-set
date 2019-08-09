/*
	File: flow_sensor_simple_calibration.c
 */
#include "../global.h"
#include "../ADC/adc_manager.h"
#include "flow_sensor_simple_calibration.h"


#if SIMPLE_CLIBRATION_FLOW_ENABLE
#define MAX_SIMPLE_CLIBRATION_FLOW_WORK_NUM 256
CCMRAM uint16_t Calibration_flow_sensor_adc_data[MAX_SIMPLE_CLIBRATION_FLOW_WORK_NUM];
CCMRAM uint16_t Calibration_flow_sensor_adc_hdata[MAX_SIMPLE_CLIBRATION_FLOW_WORK_NUM];

CCMRAM int32_t Calibration_flow_data[MAX_SIMPLE_CLIBRATION_FLOW_WORK_NUM];
SIMPLE_CLIBRATION_FLOW_WORK_DATA simple_calibration_flow_work_data;
#define SIMPLE_CALIBRATION_FLOW_TIME_STEP 10000 // 10 second

#define SIMPLE_CALIBRATION_FLOW_SUM_NUM 16


void set_peep_valve_control_val(uint16_t val);
void set_blower_speed(uint16_t speed);

void reset_simple_calibration_flow_sum_data(void)
{
    simple_calibration_flow_work_data.flow_sum = 0;
    simple_calibration_flow_work_data.sensor_high_sum = 0;
    simple_calibration_flow_work_data.sensor_low_sum = 0;
    simple_calibration_flow_work_data.sum_count = 0;
}

uint8_t append_simple_calibration_flow_sum_data(void)
{
    uint32_t temp;
    simple_calibration_flow_work_data.flow_sum += get_inhale_flow();
    simple_calibration_flow_work_data.sensor_high_sum += adc_voltage.adc_flow_exhale_high;
    simple_calibration_flow_work_data.sensor_low_sum  += adc_voltage.adc_flow_exhale_low;
    simple_calibration_flow_work_data.sum_count ++;

    if(simple_calibration_flow_work_data.sum_count >= SIMPLE_CALIBRATION_FLOW_SUM_NUM)
    {
        temp = simple_calibration_flow_work_data.sensor_low_sum / simple_calibration_flow_work_data.sum_count;
        Calibration_flow_sensor_adc_data[simple_calibration_flow_work_data.count] = (uint16_t) (temp & 0xffff);
        temp = simple_calibration_flow_work_data.sensor_high_sum / simple_calibration_flow_work_data.sum_count;
        Calibration_flow_sensor_adc_hdata[simple_calibration_flow_work_data.count] = (uint16_t) (temp & 0xffff);
        temp = simple_calibration_flow_work_data.flow_sum / simple_calibration_flow_work_data.sum_count;
        Calibration_flow_data[simple_calibration_flow_work_data.count] = temp ;

        reset_simple_calibration_flow_sum_data();
        simple_calibration_flow_work_data.count ++;
        return EM_TRUE;
    }
    return EM_FALSE;
}

void reset_simple_calibration_flow_data(void)
{
    int i;
    for(i = 0; i < MAX_SIMPLE_CLIBRATION_FLOW_WORK_NUM; i++)
    {
        Calibration_flow_sensor_adc_data[i] = 0;
        Calibration_flow_sensor_adc_hdata[i] = 0;
        Calibration_flow_data[i] = 0;
    }

    reset_simple_calibration_flow_sum_data();

    simple_calibration_flow_work_data.peep_val = 3000;
    simple_calibration_flow_work_data.speed = 35000;
    simple_calibration_flow_work_data.count = 0;
    simple_calibration_flow_work_data.time_start = 0;
    simple_calibration_flow_work_data.status = EM_SIMPLE_CALIBRATION_FLOW_STOP;
}

void simple_calibration_flow_start(void)
{
    // 设置PEEP
    set_peep_valve_control_val(simple_calibration_flow_work_data.peep_val);
    // blower
    set_blower_speed(simple_calibration_flow_work_data.speed);
    // set time base
    simple_calibration_flow_work_data.time_start = ms_1_count;

    simple_calibration_flow_work_data.status = EM_SIMPLE_CALIBRATION_FLOW_INCREASE;
}

void simple_calibration_flow_increase(void)
{
    // 如果时间未到，则返回
    if(ms_1_count < simple_calibration_flow_work_data.time_start + SIMPLE_CALIBRATION_FLOW_TIME_STEP) return;
    if(simple_calibration_flow_work_data.status != EM_SIMPLE_CALIBRATION_FLOW_INCREASE) return;
    // 存储数据
    // 可以考虑读8-64个进行平均
    if(!append_simple_calibration_flow_sum_data()) return;

    // reset time base!!!
    simple_calibration_flow_work_data.time_start = ms_1_count;

    // 判断 防止溢出
    if(simple_calibration_flow_work_data.count > 250)
    {
        // complete
        // 把涡轮速度降下来
        set_blower_speed(5000);
        // 停止运行
        simple_calibration_flow_work_data.status = EM_SIMPLE_CALIBRATION_FLOW_STOP;

        // 直接返回了
        return;
    }
    // next status
    if(simple_calibration_flow_work_data.peep_val >= 50)
    {
        // 如果PEEP阀没有完全打开则尝试打开，每次调整100
        simple_calibration_flow_work_data.peep_val -= 50;
        set_peep_valve_control_val(simple_calibration_flow_work_data.peep_val);
    }
    else if(simple_calibration_flow_work_data.speed <= 40000)
    {
        // 如果PEEP阀调整完成了，就尝试调整涡轮
        simple_calibration_flow_work_data.speed += 500;
        set_blower_speed(simple_calibration_flow_work_data.speed);

    }
    else
    {
        // complete
        // 把涡轮速度降下来
        set_blower_speed(5000);
        // 停止运行
        simple_calibration_flow_work_data.status = EM_SIMPLE_CALIBRATION_FLOW_STOP;
    }
}

void do_simple_calibration_flow_work(void)
{
    switch(simple_calibration_flow_work_data.status)
    {
    case EM_SIMPLE_CALIBRATION_FLOW_RESET:
        reset_simple_calibration_flow_data();
        break;

    case EM_SIMPLE_CALIBRATION_FLOW_START:
        simple_calibration_flow_start();
        break;

    case EM_SIMPLE_CALIBRATION_FLOW_INCREASE:
        simple_calibration_flow_increase();
        break;

    case EM_SIMPLE_CALIBRATION_FLOW_STOP:
        // just waiting;
        break;
    default:
        break;
    }
}
#else
__weak void reset_simple_calibration_flow_data(void) {}
__weak void do_simple_calibration_flow_work(void) {}
__weak void simple_calibration_flow_start(void) {}

#endif
// end
