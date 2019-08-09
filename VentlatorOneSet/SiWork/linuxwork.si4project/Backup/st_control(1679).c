// FIle: st_control.c
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
#include "pcv_control.h"
#include "st_control.h"
#include "trigger_breath.h"
#include "../alarm/alarm.h"

ST_STV_CONTROL_DAT stv_control_dat;

/**
 * [is_stv_protect_time_active description]
 * @method is_stv_protect_time_active
 * @return [description]
 */
#define is_stv_protect_time_active() (stv_control_dat.protect_active_flag)

/**
 * [set_stv_protect_active_flag description]
 * @method set_stv_protect_active_flag
 * @return [description]
 */
#define set_stv_protect_active_flag() (stv_control_dat.protect_active_flag = 1)

/**
 * [reset_stv_protect_active_flag description]
 * @method reset_stv_protect_active_flag
 * @return [description]
 */
#define reset_stv_protect_active_flag() (stv_control_dat.protect_active_flag = 0)


/**
 * [reset_cpap_inhale_time_gate description]
 * @method reset_cpap_inhale_time_gate
 * @return [description]
 */
#define reset_stv_inhale_time_gate() {stv_control_dat.inhale_time_protected_gate = ms_1_count + 250, stv_control_dat.protect_active_flag = 1;}

/**
 * [reset_stv_exhale_time_gate description]
 * @method reset_stv_exhale_time_gate
 * @return [description]
 */
#define reset_stv_exhale_time_gate() {stv_control_dat.exhale_time_protected_gate = ms_1_count + 250, stv_control_dat.protect_active_flag = 1;}

/**
 * [is_stv_inhale_protect_time_over description]
 * @method is_stv_inhale_protect_time_over
 * @return [description]
 */
#define is_stv_inhale_protect_time_over() (system_direct_compare_tm1_be_tm2(ms_1_count,stv_control_dat.inhale_time_protected_gate))

/**
 * [is_stv_exhale_protect_time_over description]
 * @method is_stv_exhale_protect_time_over
 * @return [description]
 */
#define is_stv_exhale_protect_time_over() (system_direct_compare_tm1_be_tm2(ms_1_count,stv_control_dat.exhale_time_protected_gate))

void copy_stv_parameter_to_main_control_data(void)
{
    main_control_data.epap          = ui_set_data.epap;
    main_control_data.ipap          = ui_set_data.ipap;
    main_control_data.T_freq        = ui_set_data.T_freq;
    main_control_data.T_inhale_time = ui_set_data.T_inhale_time;
    main_control_data.T_rising_time = ui_set_data.T_rising_time;
}

void stv_set_blower_speed(uint16_t speed)
{
    stv_control_dat.current_speed = speed;
    set_blower_speed(speed);
}

void prepare_stv_control_data(void)
{
    // T mode data (PCV -- Pressure Controlled Ventilation)
    prepare_PCV_control_data();

    // S mode data (Pressure Spontaneous Ventilation)
    stv_control_dat.high_speed = pcv_control_dat.high_speed;
    stv_control_dat.low_speed = pcv_control_dat.low_speed;
    cpap_caculate_c_flex_blower_speed(pcv_control_dat.low_speed);
    stv_control_dat.C_Flex_speed = get_cpap_c_flex_speed();
    stv_control_dat.inhale_peak_flow = 10000; //缺省
    // 触发 重新设置触发点
    trigger_simple_set_base_inhale(1000, main_control_data.epap);
    start_trigger(EM_TRIGGER_TYPE_E_INS);

    // 初始状态
    set_patient_status(EM_PATIENT_INHALE_DETECT);
}

// Spontaneous模式的控制参数调整
void adjust_stv_speed(void)
{
    // blower S-high-speed

    // blower S-low-speed

    // 如果必要
    // blower S-C-flex-speed
}


// S模式吸气开始时都需要调用这个函数
void refresh_stv_control_data(void)
{
    // 要调整一下时间
    prepare_pcv_time();

    // 修正S模式下的控制参数
    adjust_stv_speed();
}


/**
 * [check_breath_stv_parameter_change description]
 * @method check_breath_stv_parameter_change
 */
void check_breath_stv_parameter_change(void)
{
    if(ui_set_data.change_parameter_flag)
    {
        // 把控制数据拷贝过来
        copy_stv_parameter_to_main_control_data();

        // 重新计算一呼吸周期的控制数据
        prepare_stv_control_data();
        // 重置标记
        ui_set_data.change_parameter_flag = 0;
        main_control_data.change_parameter_flag = 0;
    }
}

void stv_start_s_inhale(void)
{
    if(ui_set_data.change_parameter_flag)
    {
        // 把控制数据拷贝过来
        copy_stv_parameter_to_main_control_data();

        // 重新计算一呼吸周期的控制数据
        prepare_stv_control_data();

        // 重置标记
        ui_set_data.change_parameter_flag = 0;
        main_control_data.change_parameter_flag = 0;
    }
    else
        refresh_stv_control_data();

    // 雾化器
    breath_force_operation_nebulizer(EM_ON);

    // 吸气时间保护
    reset_stv_inhale_time_gate();

    // set speed high, must start inhale period
    stv_set_blower_speed(stv_control_dat.high_speed);
    // next work status
    set_patient_status(EM_PATIENT_INHALE_PERIOD);
}

void to_breath_stv_mode(void)
{
    ui_set_data.change_parameter_flag = 0;
    // 如果当前工作模式就是ST模式
    if(get_patient_breath_mode() != EM_VENTLATOR_BIPAP_ST_MODE)
    {
        set_patient_breath_mode(EM_VENTLATOR_BIPAP_ST_MODE);
        oxygen_start();
    }

    copy_stv_parameter_to_main_control_data();
    prepare_stv_control_data();
}

// 由PCV回到Spontaneous
void stv_resume_to_s_exhale(void)
{

    set_patient_status(EM_PATIENT_EXHALE_PERIOD);
}

uint8_t is_stv_in_inhale_protect(void)
{
    if(is_stv_protect_time_active())
    {
        if(is_stv_inhale_protect_time_over())
        {
            reset_stv_protect_active_flag();
            // start trigger
            start_trigger(EM_TRIGGER_TYPE_STV_INS_FINISHED);
            return EM_TRUE;
        }
    }
    return EM_FALSE;
}

/**
 * [breath_stv_mode description]
 * @method breath_stv_mode
 */
void breath_stv_mode(void)
{
    switch(global_patient_status)
    {
    case EM_PATIENT_NOT_WORK://standby
        prepare_stv_control_data();
        break;
    case EM_PATIENT_BREATH_DETECT:
        break;

    case EM_PATIENT_INHALE_DETECT:
        prepare_stv_control_data();
        break;

    case EM_PATIENT_INHALE_START:
        stv_start_s_inhale();
        break;

    case EM_PATIENT_INHALE_PERIOD:
        // 吸气时间保护
        if(is_stv_in_inhale_protect()) break;

        // 强制进入下一状态
        set_patient_status(EM_PATIENT_EXHALE_DETECT);
        break;

    case EM_PATIENT_BREATH_HOLD:
        // 强制进入下一状态
        start_trigger(EM_TRIGGER_TYPE_STV_INS_FINISHED);// 防止没有设置触发数据
        set_patient_status(EM_PATIENT_EXHALE_DETECT);
        break;

    case EM_PATIENT_EXHALE_DETECT:	// 监测呼气触发条件
        // 检测吸气结束状态

        // 如果超时，将进入PCV- T_INHALE_START
        break;

    case EM_PATIENT_EXHALE_START:
        // 建立呼气时间保护
        reset_stv_exhale_time_gate();
        break;

    case EM_PATIENT_EXHALE_PERIOD:
        break;

    // 机械通气过程
    case EM_PATIENT_T_INHALE_START:
        // to PCV
        breath_pcv_mode();
        break;

    case EM_PATIENT_T_INHALE_PERIOD:
        // to PCV
        breath_pcv_mode();
        break;

    case EM_PATIENT_T_EXHALE_DETECT:
        // to PCV
        breath_pcv_mode();
        break;

    case EM_PATIENT_T_EXHALE_START:
        // To S-exhale-start
        stv_resume_to_s_exhale();
        break;

    // 状态结束标记
    case EM_PATIENT_STATUS_ENDS:
        prepare_stv_control_data();
        break;
    default:
        prepare_stv_control_data();
        break;
    }

}

// ==================================
// file end =========================
// ==================================


