// File:cpap_control.c
#include "stm32f4xx_hal.h"
#include "../ControlTask/base_control.h"
#include "../global.h"
#include "breath_rap.h"
#include "../blower/blower.h"
#include "../adc/adc_manager.h"
#include "../lib/leak_flow_count.h"
#include "../lib/VentLIB.h"
#include "breath_Atomizer.h"
#include "breath_cycle_adjust.h"
#include "pcv_press_increase_data.c"
#include "cpap_control.h"
#include "trigger_breath.h"
#include "../alarm/alarm.h"

typedef struct __ST_CPAP_CONTROL_DAT
{
    uint32_t cycles;
    uint16_t current_speed;  // current work speed
    uint16_t low_speed;      // speed for peep
    uint16_t c_flex_speed;   // speed for cflex
    uint16_t inhale_speed;   // speed for inhale
    uint16_t exhale_speed;   // speed for exhale

    uint32_t inhale_time_protected_gate; // 吸气时间保护门限
    uint32_t exhale_time_protected_gate; // 呼气时间保护门限

} ST_CPAP_CONTROL_DAT;
ST_CPAP_CONTROL_DAT cpap_control_dat;

#define reset_cpap_inhale_time_gate() (cpap_control_dat.inhale_time_protected_gate = ms_1_count + 250)
#define reset_cpap_exhale_time_gate() (cpap_control_dat.exhale_time_protected_gate = ms_1_count + 250)

void cpap_caculate_c_flex_blower_speed(uint16_t set_speed)
{
    uint16_t speed;
    if(set_speed < 5000)
        speed = 0;
    else if(set_speed > 30000)
        speed = 25000;
    speed >>= 2; // /4

    if(main_control_data.c_flex_level > 4)
        main_control_data.c_flex_level = 4;

    cpap_control_dat.c_flex_speed = main_control_data.c_flex_level * speed;
    cpap_control_dat.c_flex_speed += 5000;
}

void reset_cpap_time_gate(void)
{
    reset_cpap_inhale_time_gate();
    reset_cpap_exhale_time_gate();
}

void reset_cpap_control_common_dat(void)
{
    cpap_control_dat.low_speed = get_blower_speed_for_press_and_flow(main_control_data.epap, CONST_EXHALE_FLOW);
    cpap_control_dat.current_speed = cpap_control_dat.low_speed;
    cpap_caculate_c_flex_blower_speed();
    // 吸气
    cpap_control_dat.inhale_speed = cpap_control_dat.low_speed + 2000;
    // 呼气
    cpap_control_dat.exhale_speed = cpap_control_dat.low_speed - 2000;

    reset_cpap_time_gate();

    start_trigger(EM_TRIGGER_TYPE_E_INS);

    set_patient_status(EM_PATIENT_T_INHALE_DETECT);
}
void prepare_cpap_control_dat(void)
{
    if(!is_ui_change_parameters())
    {
        reset_display_data(); // 初始化显示数据
    }

    mark_breath_T_stmap();// 吸呼气 时间戳		强制

    // 基本控制数据
    cpap_control_dat.cycles = 0;

    reset_cpap_control_common_dat();
}
// 切换到CPAP模式
/**
 * [to_breath_cpap_mode description]
 * @method to_breath_cpap_mode
 */
void to_breath_cpap_mode(void)
{
}

void cpap_set_speed_normal(void)
{
    cpap_control_dat.current_speed = cpap_control_dat.low_speed;
    set_blower_speed(cpap_control_dat.low_speed);
}

void cpap_set_speed_inhale(void)
{
    cpap_control_dat.current_speed = cpap_control_dat.inhale_speed;
    set_blower_speed(cpap_control_dat.inhale_speed);
}

void cpap_set_speed_exhale(void)
{
    cpap_control_dat.current_speed = cpap_control_dat.exhale_speed;
    set_blower_speed(cpap_control_dat.exhale_speed);
}

void cpap_set_speed_c_flex(void)
{
    cpap_control_dat.current_speed = cpap_control_dat.c_flex_speed;
    set_blower_speed(cpap_control_dat.c_flex_speed;
}
void copy_CPAP_parameter_to_main_control_data(void)
{
    main_control_data.epap         = ui_set_data.epap;
    main_control_data.c_flex_level = ui_set_data.c_flex_level;
}

void check_breath_CPAP_parameter_change(void)
{
    if(ui_set_data.change_parameter_flag)
    {
        // copy
        copy_CPAP_parameter_to_main_control_data();
        // reset common data
        reset_cpap_control_common_dat();
    }
}

void breath_cpap_mode(void)
{
    switch(get_patient_status())
    {
    /* *******************************************************************/
    // 非正常状态
    /* *******************************************************************/
    case EM_PATIENT_NOT_WORK:       // 待机 standby
        break;

    case EM_PATIENT_PIPE_OUT:       // 管路处于脱落状态
        break;

    /* *******************************************************************/
    // 正常工作状态
    /* *******************************************************************/
    case EM_PATIENT_BREATH_DETECT:  // 专门用于测试的状态
        break;


    case EM_PATIENT_INHALE_DETECT:
        trigger_set_current_status();
        if(is_patient_actived_breath())
        {
            if(pcv_control_dat.cycles ++ == 0)
            {
                display_set_last_work_status(EM_PATIENT_BREATH_DETECT);
            }
            cpap_set_speed_inhale();
            set_patient_status(EM_PATIENT_T_INHALE_START);
        }
        break;
    case EM_PATIENT_INHALE_START:
        // 检查UI设置是否有变化
        if(check_mode_change_flag())
            return;

        check_breath_CPAP_parameter_change();
        break;
    case EM_PATIENT_INHALE_PERIOD:
        break;
    case EM_PATIENT_BREATH_HOLD:
        break;
    case EM_PATIENT_EXHALE_DETECT:	// 监测呼气触发条件
        break;
    case EM_PATIENT_EXHALE_START:
        break;
    case EM_PATIENT_EXHALE_PERIOD:
        break;

    ////////////////////////////////////////////////////
    // 状态结束标记
    case EM_PATIENT_STATUS_ENDS:
        break;
    default:
        break;
    }

}
// file end

