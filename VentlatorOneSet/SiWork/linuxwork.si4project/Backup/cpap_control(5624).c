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
#include "cpap_control.h"
#include "trigger_breath.h"
#include "../alarm/alarm.h"
#define CONST_CPAP_EXHALE_FLOW 30000

void set_peep_valve_control_val(uint16_t val);
void oxygen_start(void);

typedef struct __ST_CPAP_CONTROL_DAT
{
    uint32_t cycles;
    uint32_t inhale_time_protected_gate; // 吸气时间保护门限
    uint32_t exhale_time_protected_gate; // 呼气时间保护门限
    uint16_t current_speed;  // current work speed 还没想好怎么用，感觉应该有这样一个变量
    uint16_t low_speed; 	 // speed for peep     在没有压力补偿时，使用这个
    uint16_t c_flex_speed;	 // speed for cflex    压力释放的强度
    uint16_t inhale_speed;	 // speed for inhale   吸气时的补偿（或许需要一个函数或PID，用于动态补偿）
    uint16_t exhale_speed;	 // speed for exhale   呼气时的补偿（或许需要一个函数或PID，用于动态补偿）
    uint16_t protect_active_flag; // 用于保障最小吸呼气时间，还有避免可能的来自自身控制产生的压力干扰

    uint16_t peep_val;
    // inhale Plow
    int16_t inhale_press_low;
    int16_t exhale_press_high;

    //  时间戳
    uint32_t time_stemp; // 暂时没有用，先放这里
} ST_CPAP_CONTROL_DAT;
ST_CPAP_CONTROL_DAT cpap_control_dat;


#define CPAP_PRESS_DEFAULT_SET_VALUE 10000
/*******************************************************************/
// inline codes block
/*******************************************************************/

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

#define is_cpap_bad_breath() (system_direct_compare_tm1_be_tm2(ms_1_count,display_temp_data.T_stamp+10000))

/*******************************************************************/
// code start
/*******************************************************************/

/**
 * [cpap_caculate_c_flex_blower_speed description] 计算c-flex控制的涡轮风机转速
 * @method cpap_caculate_c_flex_blower_speed
 * @param  set_speed                         [description]
 */
void cpap_caculate_c_flex_blower_speed(uint16_t set_speed)
{
    uint16_t speed;
    if(set_speed < 5000)
        speed = 0;
    else if(set_speed > 25000)
        speed = 25000;
    speed >>= 2; // /4

    if(main_control_data.c_flex_level > 4)
        main_control_data.c_flex_level = 4;

    cpap_control_dat.c_flex_speed = main_control_data.c_flex_level * speed;
    cpap_control_dat.c_flex_speed += 5000;
}

/**
 * [reset_cpap_time_gate description] 复位保护时间
 * @method reset_cpap_time_gate
 */
void reset_cpap_time_gate(void)
{
    reset_cpap_inhale_time_gate();
    reset_cpap_exhale_time_gate();
}

/**
 * [reset_cpap_control_common_dat description] 重复复位的内容，单独列起来容易管理
 * @method reset_cpap_control_common_dat
 */
void reset_cpap_control_common_dat(void)
{
    // 涡轮风机
    cpap_control_dat.low_speed = get_blower_speed_for_press_and_flow(main_control_data.epap, CONST_CPAP_EXHALE_FLOW);
    cpap_control_dat.current_speed = cpap_control_dat.low_speed;
    cpap_caculate_c_flex_blower_speed(cpap_control_dat.low_speed);

    // peep阀
    cpap_control_dat.peep_val = get_peep_value_with_press_and_flow(main_control_data.epap, CONST_CPAP_EXHALE_FLOW);
    set_peep_valve_control_val(cpap_control_dat.peep_val);
    // 吸气
    cpap_control_dat.inhale_speed = cpap_control_dat.low_speed + 2000;
    // 呼气
    cpap_control_dat.exhale_speed = cpap_control_dat.low_speed - 2000;

    cpap_control_dat.inhale_press_low = CPAP_PRESS_DEFAULT_SET_VALUE;
    cpap_control_dat.exhale_press_high = CPAP_PRESS_DEFAULT_SET_VALUE;
}

void refresh_cpap_inhale_press_low(int16_t press, int32_t flow)
{
    if(cpap_control_dat.inhale_press_low == CPAP_PRESS_DEFAULT_SET_VALUE)
    {
        cpap_control_dat.inhale_press_low = press;
        return;
    }
    if(flow < 1000) return; // do nothing
    if(cpap_control_dat.inhale_press_low > press)
        cpap_control_dat.inhale_press_low = press;
}

void refresh_cpap_exhale_press_high(int16_t press, int32_t flow)
{
    if(cpap_control_dat.inhale_press_low == CPAP_PRESS_DEFAULT_SET_VALUE)
    {
        cpap_control_dat.inhale_press_low = press;
        return;
    }

    if(flow > -1500) return; // do nothing
    if(cpap_control_dat.exhale_press_high < press)
        cpap_control_dat.exhale_press_high = press;
}

/**
 * [prepare_cpap_control_dat description] 切换到CPAP模式时，数据处理函数
 * @method prepare_cpap_control_dat
 */
void prepare_cpap_control_dat(void)
{
    if(!is_ui_change_parameters())
    {
        reset_display_data(); // 初始化显示数据
    }

    mark_breath_T_stmap();// 吸呼气 时间戳		强制

    // 基本控制数据 需要复位
    cpap_control_dat.cycles = 0;
    reset_cpap_time_gate();
    reset_cpap_control_common_dat();

    // 触发
    trigger_simple_set_base_inhale(1000, main_control_data.epap);
    trigger_simple_set_base_exhale(1000, main_control_data.epap);
    //start_trigger(EM_TRIGGER_TYPE_E_INS);
    start_trigger(EM_TRIGGER_TYPE_E_EXP_INS);

    // 初始状态
    //set_patient_status(EM_PATIENT_T_INHALE_DETECT);
    set_patient_status(EM_PATIENT_BREATH_DETECT);
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
    set_blower_speed(cpap_control_dat.c_flex_speed);
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

// 不考虑压力的影响，只考虑泄漏漏量及PeeP阀的流量
void correct_cpap_peep(void)
{
}

/**
 * [correct_cpap_speed description]调整转速
 * 说明：由于系统的诸多不确定性，因此这个函数需要更多的磨合才有可能最终确定调整方案
 * 注意事项：调整后的转速可能出乎预料，比如c-flex-speed会比inhale_speed高这类的现象
 *           需要最终按照实际情况加以修正
 * 这个函数不是必须调用的，如果数据不合适，应该避免调用这个函数
 * 调用这个函数会有潜在的危险---就是控制失败，导致实际的控制波形与预期越来越远
 * @method correct_cpap_speed
 */
void correct_cpap_speed(void)
{
    int16_t d_press;

    // 如果数据无效则退出
    if(cpap_control_dat.inhale_press_low == CPAP_PRESS_DEFAULT_SET_VALUE) return;
    if(cpap_control_dat.exhale_press_high == CPAP_PRESS_DEFAULT_SET_VALUE) return;

    // step1
    // for exhale blower speed
    d_press = cpap_control_dat.exhale_press_high - main_control_data.epap;
    if((d_press > 0))
    {
        if(d_press < 10) goto CORRECT_CPAP_SPEED_NEXT1;

        cpap_control_dat.exhale_speed -= d_press * 30;//100; 只调整30%
    }
    else
    {
        cpap_control_dat.exhale_speed -= d_press * 20;//100; 只调整20%
    }
CORRECT_CPAP_SPEED_NEXT1:
    //step2
    // for inhale blower speed
    d_press = main_control_data.epap - cpap_control_dat.inhale_press_low;
    if((d_press > 0))
    {
        if(d_press < 10) goto CORRECT_CPAP_SPEED_NEXT2;

        cpap_control_dat.inhale_speed += d_press * 30;//100; 只调整30%
    }
    else
    {
        cpap_control_dat.inhale_speed -= d_press * 20;//100; 只调整20%
    }

CORRECT_CPAP_SPEED_NEXT2:
    // step3
    // for normal blower speed
    d_press = main_control_data.epap - display_count_data.P_peep;
    if((d_press > 0))
    {
        if(d_press < 10) goto CORRECT_CPAP_SPEED_NEXT3;

        cpap_control_dat.low_speed += d_press * 30;//100; 只调整30%
    }
    else
    {
        cpap_control_dat.low_speed -= d_press * 20;//100; 只调整20%
    }

CORRECT_CPAP_SPEED_NEXT3:
    // step 4
    // 纠正不规范的调整数据
    if(cpap_control_dat.exhale_speed < 5000)
        cpap_control_dat.exhale_speed = 5000;
    // correct c-flex
    if(cpap_control_dat.c_flex_speed < cpap_control_dat.exhale_speed)
        cpap_control_dat.c_flex_speed = cpap_control_dat.exhale_speed - 1000;
    if(cpap_control_dat.inhale_speed < cpap_control_dat.exhale_speed)
        cpap_control_dat.inhale_speed = cpap_control_dat.exhale_speed + 1000;
    if(cpap_control_dat.low_speed < cpap_control_dat.exhale_speed)
        cpap_control_dat.low_speed = cpap_control_dat.exhale_speed;

    // need reset low and high record
    cpap_control_dat.inhale_press_low = CPAP_PRESS_DEFAULT_SET_VALUE;
    cpap_control_dat.exhale_press_high = CPAP_PRESS_DEFAULT_SET_VALUE;
}

// 切换到CPAP模式
/**
 * [to_breath_cpap_mode description]
 * @method to_breath_cpap_mode
 */
void to_breath_cpap_mode(void)
{
    ui_set_data.change_parameter_flag = 0;

    // 如果当前工作模式就是PCV模式
    if(get_patient_breath_mode() != EM_VENTLATOR_CPAP_MODE)
    {
        set_patient_breath_mode(EM_VENTLATOR_CPAP_MODE);
        oxygen_start();
    }

    copy_CPAP_parameter_to_main_control_data();
    prepare_cpap_control_dat();
}


/**
 * [breath_cpap_mode description]CPAP 主调用入口函数
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
    case EM_PATIENT_BREATH_DETECT:  // 当吸气时间或呼气时间超过10秒，进入这个状态可能会好一些
        trigger_set_current_status();
        // 要判断呼气流速状态，以决定是否开启吸气检测
        switch(is_patient_actived_breath())
        {
        case 1:
            cpap_control_dat.cycles = 0;
            display_set_last_work_status(EM_PATIENT_BREATH_DETECT);

            cpap_set_speed_inhale();
            stop_trigger();
            set_patient_status(EM_PATIENT_T_INHALE_START);
            break;
        case 2:
            stop_trigger();
            set_patient_status(EM_PATIENT_EXHALE_START);
            break;
        default:
            break;
        }
    case EM_PATIENT_INHALE_DETECT:
        //刷新状态
        trigger_set_current_status();
        // 检测
        if(is_patient_actived_breath())
        {
            if(cpap_control_dat.cycles ++ == 0)
            {
                // 第一个周期不做呼吸周期的计算
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
                    // 这一呼吸周期有效，需计算相关数据
                    display_set_last_work_status(EM_PATIENT_INHALE_DETECT);

                    // 这里要考虑通气控制的调整问题
                    correct_cpap_speed();
                }
            }
            cpap_set_speed_inhale();
            stop_trigger();
            set_patient_status(EM_PATIENT_T_INHALE_START);
        }

        // 如果超过10秒则进入EM_PATIENT_BREATH_DETECT
        if(is_cpap_bad_breath())
        {
            stop_trigger();
            start_trigger(EM_TRIGGER_TYPE_E_EXP_INS);
            set_patient_status(EM_PATIENT_BREATH_DETECT);
            break;
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
                start_trigger(EM_TRIGGER_TYPE_CPAP_INS_FINISHED);
            }
        }
        refresh_cpap_inhale_press_low(display_count_data.Press, display_count_data.Flow);
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
            break;
        }

        // 如果超过10秒则进入EM_PATIENT_BREATH_DETECT
        if(is_cpap_bad_breath())
        {
            stop_trigger();
            start_trigger(EM_TRIGGER_TYPE_E_EXP_INS);
            set_patient_status(EM_PATIENT_BREATH_DETECT);
            break;
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
        refresh_cpap_exhale_press_high(display_count_data.Press, display_count_data.Flow);
        trigger_set_current_status();
        // 要判断呼气流速状态，以决定是否开启吸气检测
        if(is_patient_actived_breath())
        {
            //correct_cpap_speed();
            cpap_set_speed_normal();

            // 先停止触发检测
            stop_trigger();

            // 计算呼气潮气量，忽略之后的潮气量计算
            breath_count_Te();

            // start peep count
            set_peep_start_flag();

            // 暂时在这里调整PEEP阀
            correct_cpap_peep();

            // change work status
            set_patient_status(EM_PATIENT_INHALE_DETECT);
            break;
        }

        // 如果呼气时间超过3秒，则强制进入呼气检测
        if(is_cpap_force_change_to_inhale_detect())
        {
            //correct_cpap_speed();
            cpap_set_speed_normal();

            // 先停止触发检测
            stop_trigger();

            // 计算呼气潮气量
            breath_count_Te();

            // start peep count
            set_peep_start_flag();

            // 暂时在这里调整PEEP阀
            correct_cpap_peep();

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
