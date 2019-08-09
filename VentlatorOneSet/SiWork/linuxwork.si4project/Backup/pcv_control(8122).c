/**
 * File:pcv_control.c
 */

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
#include "pcv_control.h"


ST_PCV_CONTROL_DAT st_pcv_control_dat;
__weak void prepare_PCV_control_data(void)
{
    uint32_t t1;
    uint16_t speed;

    if(!is_ui_change_parameters())
    {
        reset_display_data(); // 初始化显示数据
    }
    mark_breath_T_stmap();// 吸呼气 时间戳

    // 暂时认为T_press = ipap
    main_control_data.T_press = main_control_data.ipap;

    // Based 必须先计算这个控制速度，然后才能计算后面的
    // IPAP/EPAP for fast PID
    reset_PID_blower_control_data();
    set_PID_blower_control_base_speed(EM_BASE_SPEED_EPAP); // Low_speed
    set_PID_blower_control_base_speed(EM_BASE_SPEED_IPAP); // High_speed //！！这个可能用不到
    set_PID_blower_control_base_speed(EM_BASE_SPEED_BASE); // T_High_speed
#if PEEP_VALVE_CHANGE_ENABLE
    if(basic_control_parameters.invasive_support == 1)
    {
        PID_blower_control_data.High_peep = MAX_PWM_VAL;
    }
    else
    {
        PID_blower_control_data.High_peep = get_peep_value_with_press_and_flow(main_control_data.T_press, main_control_data.flow);
    }
    PID_blower_control_data.Low_peep  = get_peep_value_with_press_and_flow(main_control_data.epap, main_control_data.flow);
#endif
#if USE_BREATH_CYCLES_ADJUST
    breath_cycles_save_peep_data(EM_BREATH_ADJUST_SAVE_I_CONTROL, PID_blower_control_data.High_peep, main_control_data.flow);
    //breath_cycles_save_peep_data(EM_BREATH_ADJUST_SAVE_T_CONTROL, PID_blower_control_data.High_peep, main_control_data.flow);
    breath_cycles_save_peep_data(EM_BREATH_ADJUST_SAVE_E_CONTROL, PID_blower_control_data.Low_peep,  main_control_data.flow);
    breath_cycles_save_blower_speed_data(EM_BREATH_ADJUST_SAVE_E_CONTROL, PID_blower_control_data.Low_speed, main_control_data.epap);
    //breath_cycles_save_blower_speed_data(EM_BREATH_ADJUST_SAVE_I_CONTROL, PID_blower_control_data.High_speed, main_control_data.ipap);
    breath_cycles_save_blower_speed_data(EM_BREATH_ADJUST_SAVE_T_CONTROL, PID_blower_control_data.T_High_speed, main_control_data.T_press);
#endif
    // 压力上升相关的
    // Press increse base + status
    // 计数，用于计算上升时间
    t1 = main_control_data.T_rising_time;// 注意：main_control_data.T_rising_time 的单位 = 1ms
    if(t1 < VAL_PRESS_INCREASE_STEP_TIME * 2)
    {
        t1 = VAL_PRESS_INCREASE_STEP_TIME * 2; // 防止除0，保障上升时间最小100ms
    }
    // 压力上升时的步进（每 VAL_PRESS_INCREASE_STEP_TIME 毫秒调整一次）
    st_PCV_mode_control_data.rising_time_steps        = t1 / VAL_PRESS_INCREASE_STEP_TIME;
    speed = PID_blower_control_data.T_High_speed - PID_blower_control_data.Low_speed;
    st_PCV_mode_control_data.blower_increase_speed    = speed / st_PCV_mode_control_data.rising_time_steps;

#if BLOWER_ADJUST_PID_PRESS_INCREASE_ENABLE
    // 用于PID（blower）控制中的目标压力   -
    //!!! 这里有问题，关键是压力数值比较小，但调整的步数可能超过压力值，这会导致压力值不变
    PCV_increase_press_data.d_press                 = main_control_data.T_press - main_control_data.epap;
    PCV_increase_press_data.total_steps               = st_PCV_mode_control_data.rising_time_steps;
#endif
    // reset;
    st_PCV_mode_control_data.T_target_press           = main_control_data.epap;

    // 时间控制相关的
    t1 = 60000 / main_control_data.T_freq; // 计算每一周期的时间，用于启动T模式
    st_PCV_mode_control_data.T_one_breath_time        = t1;
    // 由呼吸频率决定，确定T触发时间
    st_PCV_mode_control_data.T_next_breath_start_gate = ms_1_count + t1;
    st_PCV_mode_control_data.T_expire_start_gate      = ms_1_count + main_control_data.T_inspire_time;

    if(t1 > main_control_data.T_inspire_time + 500)
    {
        st_PCV_mode_control_data.T_expire_3_4_gate    = ((t1 - main_control_data.T_inspire_time) * 3 / 4) +
                st_PCV_mode_control_data.T_expire_start_gate;
    }
    else
    {
        st_PCV_mode_control_data.T_expire_3_4_gate    = st_PCV_mode_control_data.T_next_breath_start_gate - 200;
    }


    PID_blower_control_data.base_speed            = PID_blower_control_data.Low_speed;
    set_blower_speed(PID_blower_control_data.base_speed);

    // PCV PID control
    Blower_start_pcv_control();

    //	set_PID_blower_control_status(EM_BREATH_PCV_PRESS_INC);
    //    set_PID_blower_adjust_mode(ENUM_PID_FAST_FOR_PRESS);
    // 吸气相--相关的 触发 门限/阈值
    // TBD
}


//	float *inc_data_ptr;  //数据指针
//	int inc_data_count;   //计数（）
//	int max_inc_data_num; //最大计数
void set_pcv_increase_data(void)
{
    if(main_control_data.T_rising_time > 4) main_control_data.T_rising_time = 4;
    st_pcv_control_dat.inc_data_ptr = (float *) PCV_EXP_INCREASE_DATA[main_control_data.T_rising_time];
    st_pcv_control_dat.max_inc_data_num = PCV_INCREASE_DATA_LEN[main_control_data.T_rising_time];
    st_pcv_control_dat.inc_data_count = 0; // reset
}

/**
 * [prepare_PCV_control_data description]
 * @method prepare_PCV_control_data
 */
void prepare_PCV_control_data(void)
{
    // 基础数据
    // 基础数据-- 数据
    // 基础数据-- 时间

    // 基础数据-- 涡轮速度
    st_pcv_control_dat.low_speed = get_blower_speed_for_press_and_flow(main_control_data.epap, CONST_EXHALE_FLOW);
    st_pcv_control_dat.high_speed = get_blower_speed_for_press_and_flow(main_control_data.ipap, CONST_EXHALE_FLOW);
    // 模式控制-- 压力上升
    set_pcv_increase_data();
    // 模式控制-- 压力保持

    // 模式控制-- 压力下降（呼气）

    // 模式控制-- 触发

    // 模式控制-- 叩击

    // 模式控制-- 进入pcv的状态
    set_patient_status(EM_PATIENT_T_INSPIRE_DETECT);
}

/**
 * [breath_pcv_mode description]
 * @method breath_pcv_mode
 */
void breath_pcv_mode(void)
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
    /* *******************************************************************/
    // 机械通气过程
    /* *******************************************************************/
case EM_PATIENT_T_INSPIRE_START:
    break;
case EM_PATIENT_T_INSPIRE_PERIOD:
    break;
case EM_PATIENT_T_EXPIRE_START:
    break;
case EM_PATIENT_T_EXPIRE_DETECT:
    break;
case EM_PATIENT_T_EXPIRE_PERIOD:
    break;
case EM_PATIENT_T_INSPIRE_DETECT:
    break;
    /* *******************************************************************/
    // 状态结束标记
    /* *******************************************************************/
case EM_PATIENT_STATUS_ENDS:
    break;
default:
    break;
}
