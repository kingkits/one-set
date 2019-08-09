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
    uint32_t inhale_time_protected_gate; // 吸气时间保护门限
    uint32_t exhale_time_protected_gate; // 呼气时间保护门限
    uint16_t current_speed;  // current work speed
    uint16_t low_speed; 	 // speed for peep
    uint16_t c_flex_speed;	 // speed for cflex
    uint16_t inhale_speed;	 // speed for inhale
    uint16_t exhale_speed;	 // speed for exhale
    uint16_t protect_active_flag;

    // inhale Plow
    int16_t inhale_press_low;
    int16_t exhale_press_high;

    //  时间戳
    uint32_t time_stemp;
} ST_CPAP_CONTROL_DAT;
ST_CPAP_CONTROL_DAT cpap_control_dat;

// inline
/**
 * [reset_cpap_inhale_time_gate description]
 * @method reset_cpap_inhale_time_gate
 * @return [description]
 */
#define reset_cpap_inhale_time_gate() {cpap_control_dat.inhale_time_protected_gate = ms_1_count + 250, cpap_control_dat.protect_active_flag = 1;}

/**
 * [reset_cpap_exhale_time_gate description]
 * @method reset_cpap_exhale_time_gate
 * @return [description]
 */
#define reset_cpap_exhale_time_gate() {cpap_control_dat.exhale_time_protected_gate = ms_1_count + 250, cpap_control_dat.protect_active_flag = 1;}

/**
 * [is_cpap_inhale_protect_time_over description]
 * @method is_cpap_inhale_protect_time_over
 * @return [description]
 */
#define is_cpap_inhale_protect_time_over() (system_direct_compare_tm1_be_tm2(ms_1_count,cpap_control_dat.inhale_time_protected_gate))

/**
 * [is_cpap_exhale_protect_time_over description]
 * @method is_cpap_exhale_protect_time_over
 * @return [description]
 */
#define is_cpap_exhale_protect_time_over() (system_direct_compare_tm1_be_tm2(ms_1_count,cpap_control_dat.exhale_time_protected_gate))

/**
 * [is_cpap_protect_time_active description]
 * @method is_cpap_protect_time_active
 * @return [description]
 */
#define is_cpap_protect_time_active() (cpap_control_dat.protect_active_flag)

/**
 * [set_cpap_protect_active_flag description]
 * @method set_cpap_protect_active_flag
 * @return [description]
 */
#define set_cpap_protect_active_flag() (cpap_control_dat.protect_active_flag = 1)

/**
 * [reset_cpap_protect_active_flag description]
 * @method reset_cpap_protect_active_flag
 * @return [description]
 */
#define reset_cpap_protect_active_flag() (cpap_control_dat.protect_active_flag = 0)

/**
 * [is_cpap_force_change_to_inhale_detect description]
 * @method is_cpap_force_change_to_inhale_detect
 * @return [description]
 */
#define is_cpap_force_change_to_inhale_detect() (system_direct_compare_tm1_be_tm2(ms_1_count,display_temp_data.T_stamp+3000))

/**
 * [is_cpap_long_time_breath description]
 * @method is_cpap_long_time_breath
 * @return [description]
 */
#define is_cpap_long_time_breath() (system_direct_compare_tm1_be_tm2(ms_1_count,display_temp_data.T_stamp+6000))

/**
 * [cpap_caculate_c_flex_blower_speed description]
 * @method cpap_caculate_c_flex_blower_speed
 * @param  set_speed                         [description]
 */
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

/**
 * [reset_cpap_time_gate description]
 * @method reset_cpap_time_gate
 */
void reset_cpap_time_gate(void)
{
    reset_cpap_inhale_time_gate();
    reset_cpap_exhale_time_gate();
}

/**
 * [reset_cpap_control_common_dat description]
 * @method reset_cpap_control_common_dat
 */
void reset_cpap_control_common_dat(void)
{
    cpap_control_dat.low_speed = get_blower_speed_for_press_and_flow(main_control_data.epap, CONST_EXHALE_FLOW);
    cpap_control_dat.current_speed = cpap_control_dat.low_speed;
    cpap_caculate_c_flex_blower_speed(cpap_control_dat.low_speed);
    // 吸气
    cpap_control_dat.inhale_speed = cpap_control_dat.low_speed + 2000;
    // 呼气
    cpap_control_dat.exhale_speed = cpap_control_dat.low_speed - 2000;

    cpap_control_dat.inhale_press_low = main_control_data.epap + 30;
    cpap_control_dat.exhale_press_high = 0;

    start_trigger(EM_TRIGGER_TYPE_E_INS);

    set_patient_status(EM_PATIENT_T_INHALE_DETECT);
}

/**
 * [prepare_cpap_control_dat description]
 * @method prepare_cpap_control_dat
 */
void prepare_cpap_control_dat(void)
{
    if(!is_ui_change_parameters())
    {
        reset_display_data(); // 初始化显示数据
    }

    mark_breath_T_stmap();// 吸呼气 时间戳		强制

    // 基本控制数据
    cpap_control_dat.cycles = 0;
    reset_cpap_time_gate();
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

/**
 * [cpap_set_speed_normal description]
 * @method cpap_set_speed_normal
 */
void cpap_set_speed_normal(void)
{
    cpap_control_dat.current_speed = cpap_control_dat.low_speed;
    set_blower_speed(cpap_control_dat.low_speed);
}

/**
 * [cpap_set_speed_inhale description]
 * @method cpap_set_speed_inhale
 */
void cpap_set_speed_inhale(void)
{
    cpap_control_dat.current_speed = cpap_control_dat.inhale_speed;
    set_blower_speed(cpap_control_dat.inhale_speed);
}

/**
 * [cpap_set_speed_exhale description]
 * @method cpap_set_speed_exhale
 */
void cpap_set_speed_exhale(void)
{
    cpap_control_dat.current_speed = cpap_control_dat.exhale_speed;
    set_blower_speed(cpap_control_dat.exhale_speed);
}

/**
 * [cpap_set_speed_c_flex description]
 * @method cpap_set_speed_c_flex
 */
void cpap_set_speed_c_flex(void)
{
    cpap_control_dat.current_speed = cpap_control_dat.c_flex_speed;
    set_blower_speed(cpap_control_dat.c_flex_speed;
}

/**
 * [copy_CPAP_parameter_to_main_control_data description]
 * @method copy_CPAP_parameter_to_main_control_data
 */
void copy_CPAP_parameter_to_main_control_data(void)
{
    main_control_data.epap         = ui_set_data.epap;
    main_control_data.c_flex_level = ui_set_data.c_flex_level;
}

/**
 * [check_breath_CPAP_parameter_change description]
 * @method check_breath_CPAP_parameter_change
 */
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

/**
 * [correct_cpap_speed description]
 * @method correct_cpap_speed
 */
void correct_cpap_speed(void)
{
    int16_t d_press;

    // for exhale blower speed
    d_press = cpap_control_dat.exhale_press_high - main_control_data.epap;
    if((d_press > 0))
    {
        if(d_press < 10) goto CORRECT_CPAP_SPEED_NEXT1;

        cpap_control_dat.exhale_speed -= d_press * 30；//100; 只调整一半
    }
    else
    {
        cpap_control_dat.exhale_speed -= d_press * 20；//100; 只调整40%
    }
CORRECT_CPAP_SPEED_NEXT1:
    // for inhale blower speed
    d_press = main_control_data.epap - cpap_control_dat.inhale_press_low;
    if((d_press > 0))
    {
        if(d_press < 10) goto CORRECT_CPAP_SPEED_NEXT2;

        cpap_control_dat.inhale_speed += d_press * 30；//100; 只调整30%
    }
    else
    {
        cpap_control_dat.inhale_speed -= d_press * 20；//100; 只调整20%
    }

CORRECT_CPAP_SPEED_NEXT2:

    // for normal blower speed
    d_press = main_control_data.epap - display_count_data.P_peep;
    if((d_press > 0))
    {
        if(d_press < 10) goto CORRECT_CPAP_SPEED_NEXT3;

        cpap_control_dat.low_speed += d_press * 50；//100; 只调整一半
    }
    else
    {
        cpap_control_dat.low_speed -= d_press * 20；//100; 只调整20%
    }

CORRECT_CPAP_SPEED_NEXT3:
    if(cpap_control_dat.exhale_speed < 5000)
        cpap_control_dat.exhale_speed = 5000;
    // correct c-flex
    if(cpap_control_dat.c_flex_speed < cpap_control_dat.exhale_speed)
        cpap_control_dat.c_flex_speed = cpap_control_dat.exhale_speed - 1000;
    if(cpap_control_dat.inhale_speed < cpap_control_dat.exhale_speed)
        cpap_control_dat.inhale_speed = cpap_control_dat.exhale_speed + 1000;
    if(cpap_control_dat.low_speed < cpap_control_dat.exhale_speed)
        cpap_control_dat.low_speed = cpap_control_dat.exhale_speed;
}

/**
 * [breath_cpap_mode description]
 * @method breath_cpap_mode
 */
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
            if(cpap_control_dat.cycles ++ == 0)
            {
                display_set_last_work_status(EM_PATIENT_BREATH_DETECT);
            }
            else
            {
                if(is_cpap_long_time_breath())
                {
                    // 如果呼气时间超过6秒，则不计算上一周期的数据----有待商榷
                    display_set_last_work_status(EM_PATIENT_BREATH_DETECT);
                }
                else
                {
                    display_set_last_work_status(EM_PATIENT_INHALE_DETECT);
                }
            }
            cpap_set_speed_inhale();
            stop_trigger();
            set_patient_status(EM_PATIENT_T_INHALE_START);
        }
        break;

    case EM_PATIENT_INHALE_START:
        // 检查UI设置是否有变化
        if(check_mode_change_flag())
            return;

        check_breath_CPAP_parameter_change();

        reset_cpap_inhale_time_gate();
        set_patient_status(EM_PATIENT_INHALE_PERIOD);
        break;

    case EM_PATIENT_INHALE_PERIOD:
        if(is_cpap_protect_time_active())
        {
            if(!(is_cpap_inhale_protect_time_over()))
                break;
            else
            {
                cpap_set_speed_inhale();
                reset_cpap_protect_active_flag();
                // start trigger
                start_trigger(EM_TRIGGER_TYPE_CPAP_EXP_FINISHED);
            }
        }

        trigger_set_current_status();
        // 要判断吸气流速状态，以决定是否开启呼气检测
        if(is_patient_actived_breath())
        {
            cpap_set_speed_inhale();
            set_patient_status(EM_PATIENT_EXHALE_DETECT);
            stop_trigger();
        }
        break;

    //		case EM_PATIENT_BREATH_HOLD:
    //			break;

    case EM_PATIENT_EXHALE_DETECT:	// 监测呼气触发条件
        trigger_set_current_status();
        if(is_patient_actived_breath())
        {
            stop_trigger();
            set_patient_status(EM_PATIENT_EXHALE_START);
        }
        break;

    case EM_PATIENT_EXHALE_START:
        cpap_set_speed_c_flex();
        reset_cpap_exhale_time_gate();

        set_patient_status(EM_PATIENT_EXHALE_PERIOD);
        break;

    case EM_PATIENT_EXHALE_PERIOD:
        if(is_cpap_protect_time_active())
        {
            if(is_cpap_exhale_protect_time_over())
            {
                cpap_set_speed_exhale();
                reset_cpap_protect_active_flag();
                // start trigger
                start_trigger(EM_TRIGGER_TYPE_CPAP_EXP_FINISHED);

                break;
            }
            else break;
        }

        trigger_set_current_status();
        // 要判断呼气流速状态，以决定是否开启吸气检测
        if(is_patient_actived_breath())
        {
            correct_cpap_speed();
            cpap_set_speed_normal();
            stop_trigger();
            // 计算呼气潮气量
            breath_count_Te();
            // start peep count
            set_peep_start_flag();

            // change work status
            set_patient_status(EM_PATIENT_INHALE_DETECT);
            break;
        }

        // 如果呼气时间超过3秒，则强制进入呼气检测
        if(is_cpap_force_change_to_inhale_detect())
        {
            correct_cpap_speed();
            cpap_set_speed_normal();
            stop_trigger();
            // 计算呼气潮气量
            breath_count_Te();
            // start peep count
            set_peep_start_flag();

            // change work status
            set_patient_status(EM_PATIENT_INHALE_DETECT);
        }
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
