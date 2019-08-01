
// File: breath_cough_mode.c
#include "../global.h"
#include "../blower/blower.h"
#include "../lib/insert_data.h"
#include "../ControlTask/base_control.h"

extern uint32_t vacuum_pump_press_data[];
extern uint32_t vacuum_pump_press_index[];
extern ST_INSERT_DATA vacuum_pump_press_control_data;

void init_vacuum_pump_control_data(void)
{
    init_insert_data(&vacuum_pump_press_control_data, 13, vacuum_pump_press_index, vacuum_pump_press_data, ENUM_DATA_IS_INCREASE);
}

void vacuum_pressure_blower_start(int16_t press)
{
    //PWM_change_val(PWM_CHANEL_OVERFLOW, get_insert_inc_data((uint32_t)press, &vacuum_pump_press_control_data));
    set_vacuum_blower_pwm_comtrol_data(get_insert_inc_data((uint32_t)press, &vacuum_pump_press_control_data));
    PWM_change_val(PWM_CHANEL_VACUUM_BLOWER, get_vacuum_blower_pwm_comtrol_data());
}

void vacuum_pressure_blower_stop(void)
{
    PWM_change_val(PWM_CHANEL_VACUUM_BLOWER, 0);
    set_vacuum_blower_pwm_comtrol_data(0);
    //Overflow_disable();
}

// 关闭所有跟咳痰相关的操作
// 这个程序完成之后，必须再次发送(START COUGH)指令才能触发咳痰; (REPEAT COUGH)指令无效
void stop_cough_blower_work(void)
{
    vacuum_pressure_blower_stop();
    set_vacuum_blower_work_status(EM_OFF);
}

// 根据预设的压力，启动真空风机
void start_cough_blower_work(void)
{
    vacuum_pressure_blower_start(ui_set_data.cough_start_press);
    // 允许REPEAT COUGH 指令
    set_vacuum_blower_work_status(EM_ON);
}
///////////////////////////////////////////////////////////
// File End
///////////////////////////////////////////////////////////
