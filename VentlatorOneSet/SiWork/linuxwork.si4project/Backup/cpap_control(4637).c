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
#define reset_cpap_inhale_time_gate() {cpap_control_dat.inhale_time_protected_gate = ms_1_count + 200, cpap_control_dat.protect_active_flag = 1;}

/**
 * [reset_cpap_exhale_time_gate description]
 * @method reset_cpap_exhale_time_gate
 * @return [description]
 */
#define reset_cpap_exhale_time_gate() {cpap_control_dat.exhale_time_protected_gate = ms_1_count + 200, cpap_control_dat.protect_active_flag = 1;}

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
 * [is_cpap_bad_breath description]如果超过10秒未能检测到吸气或呼气，则为一个不完整呼吸周期，这个10秒需要最后确认
 * @method is_cpap_bad_breath
 * @return [description]
 */
#define is_cpap_bad_breath() (system_direct_compare_tm1_be_tm2(ms_1_count,display_temp_data.T_stamp+10000))

/*******************************************************************/
// code start
/*******************************************************************/
/**
 * [get_cpap_c_flex_speed description]
 * @method get_cpap_c_flex_speed
 * @return                       [description]
 */
uint16_t get_cpap_c_flex_speed(void)
{
    return cpap_control_dat.c_flex_speed;
}

/**
 * [cpap_caculate_c_flex_blower_speed description] 计算c-flex控制的涡轮风机转速
 * @method cpap_caculate_c_flex_blower_speed
 * @param  set_speed                         [description]
 */
void cpap_caculate_c_flex_blower_speed(uint16_t set_speed)
{
    uint16_t speed;
    uint16_t old_speed;

    old_speed = set_speed;
    // 由于涡轮风机的最小可控制转速为5000rpm （这个可能是个硬件bug）,所以所有的涡轮速度算法都是从5000开始的
    if(set_speed < MIN_BLOWER_SPEED)
    {
        speed = 0;
        old_speed = MIN_BLOWER_SPEED;
    }
    else if(set_speed > 25000)
        speed = 20000;
    else
        speed = set_speed - MIN_BLOWER_SPEED;
    speed >>= 2; // /4

    cpap_control_dat.c_flex_speed = old_speed;
    switch(main_control_data.c_flex_level)
    {
    case 4:
        cpap_control_dat.c_flex_speed -= speed;
    case 3:
        cpap_control_dat.c_flex_speed -= speed;
    case 2:
        cpap_control_dat.c_flex_speed -= speed;
    case 1:
        cpap_control_dat.c_flex_speed -= speed;
    case 0:
    default:
        break;
    }
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
 * [reset_common_cpap_control_dat description] 重复复位的内容，单独列起来容易管理
 * @method reset_common_cpap_control_dat
 */
void reset_common_cpap_control_dat(void)
{
    // 涡轮风机
    cpap_control_dat.low_speed = get_blower_speed_for_press_and_flow(main_control_data.epap, CONST_CPAP_EXHALE_FLOW);
    cpap_control_dat.current_speed = cpap_control_dat.low_speed;
    cpap_caculate_c_flex_blower_speed(cpap_control_dat.low_speed);

    // peep阀
    cpap_control_dat.peep_val = get_peep_value_with_press_and_flow(main_control_data.epap, CONST_CPAP_EXHALE_FLOW);
	cpap_control_dat.peep_val += 100; //ZZX debug: just test 20190807
    set_peep_valve_control_val(cpap_control_dat.peep_val);
    // 吸气
    cpap_control_dat.inhale_speed_adjust = 0;
    cpap_control_dat.inhale_speed = cpap_control_dat.low_speed ;//+ 2000;
	
    // 呼气    
	cpap_control_dat.exhale_speed_adjust = 0;
    cpap_control_dat.exhale_speed = cpap_control_dat.low_speed;// - 2000;

    cpap_control_dat.inhale_press_low = CPAP_PRESS_DEFAULT_SET_VALUE;
    cpap_control_dat.exhale_press_high = CPAP_PRESS_DEFAULT_SET_VALUE;

	cpap_control_dat.entry_cycles = 0;
}

/**
 * [refresh_cpap_inhale_press_low description]
 * @method refresh_cpap_inhale_press_low
 * @param  press                         [description]
 * @param  flow                          [description]
 */
void refresh_cpap_inhale_press_low(int16_t press, int32_t flow)
{
    if(flow < TRIGGER_INHALE_FLOW_GATE) return; // do nothing
    if(cpap_control_dat.inhale_press_low == CPAP_PRESS_DEFAULT_SET_VALUE)
    {
        cpap_control_dat.inhale_press_low = press;
        return;
    }

    if(cpap_control_dat.inhale_press_low > press)
        cpap_control_dat.inhale_press_low = press;
}

/**
 * [refresh_cpap_exhale_press_high description]
 * @method refresh_cpap_exhale_press_high
 * @param  press                          [description]
 * @param  flow                           [description]
 */
void refresh_cpap_exhale_press_high(int16_t press, int32_t flow)
{
    if(flow > TRIGGER_EXHALE_FLOW_GATE) return; // do nothing

    if(cpap_control_dat.exhale_press_high == CPAP_PRESS_DEFAULT_SET_VALUE)
    {
        cpap_control_dat.exhale_press_high = press;
        return;
    }

    if(cpap_control_dat.exhale_press_high < press)
        cpap_control_dat.exhale_press_high = press;
}

/**
 * [cpap_start_epap_pid description]
 * @method cpap_start_epap_pid
 */
void cpap_start_epap_pid(void)
{
    PID_blower_control_data.Low_speed = cpap_control_dat.low_speed;
    PID_blower_control_data.blower_e_pap_adjust_val = 0;
    Base_set_speed_for_epap();//breath_continue_with_epap
}

void cpap_start_ipap_pid(void)
{
	set_blower_speed(cpap_control_dat.inhale_speed);
	Base_set_speed_for_ipap();
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
    reset_common_cpap_control_dat();

    // 触发
    trigger_simple_set_base_inhale(TRIGGER_INHALE_FLOW_GATE, main_control_data.epap);
    trigger_simple_set_base_exhale(TRIGGER_EXHALE_FLOW_GATE, main_control_data.epap); //0 先用呼气的-2000尝试一下效果，不行再改门限
    start_trigger(EM_TRIGGER_TYPE_E_INS);

    // 模式控制-- PID
    //set_PID_blower_adjust_mode(ENUM_PID_NO_ADJUST);
    cpap_start_epap_pid(); // 在这里启动压力控制的PID

    // 初始状态
    set_patient_status(EM_PATIENT_BREATH_DETECT);
}

/**
 * [cpap_set_speed_normal description]如果启动了PID控制，则这个函数就不应该起作用了
 * @method cpap_set_speed_normal
 */
void cpap_set_speed_normal(void)
{
    cpap_control_dat.current_speed = cpap_control_dat.low_speed;
    set_blower_speed(cpap_control_dat.low_speed);
}

/**
 * [cpap_set_speed_inhale description]如果启动了PID控制，则这个函数就不应该起作用了
 * @method cpap_set_speed_inhale
 */
void cpap_set_speed_inhale(void)
{
    cpap_control_dat.current_speed = cpap_control_dat.inhale_speed;
    set_blower_speed(cpap_control_dat.inhale_speed);
}

/**
 * [cpap_set_speed_exhale description]如果启动了PID控制，则这个函数就不应该起作用了
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
static void cpap_set_speed_c_flex(void)
{
	// 模式控制-- PID
    set_PID_blower_adjust_mode(ENUM_PID_NO_ADJUST);
    cpap_control_dat.current_speed = cpap_control_dat.c_flex_speed;
    set_blower_speed(cpap_control_dat.c_flex_speed);
}

/**
 * [copy_CPAP_parameter_to_main_control_data description]
 * @method copy_CPAP_parameter_to_main_control_data
 */
static void copy_CPAP_parameter_to_main_control_data(void)
{
    main_control_data.epap         = ui_set_data.epap;
	main_control_data.ipap         = ui_set_data.epap;// 确保ipap=epap
    main_control_data.c_flex_level = ui_set_data.c_flex_level;
}

/**
 * [check_breath_CPAP_parameter_change description]
 * @method check_breath_CPAP_parameter_change
 * @return                                    [description]
 */
int8_t check_breath_CPAP_parameter_change(void)
{
    if(ui_set_data.change_parameter_flag)
    {
        // copy
        copy_CPAP_parameter_to_main_control_data();
        // reset common data
        reset_common_cpap_control_dat();
		// correct status
		set_patient_status(EM_PATIENT_INHALE_START);
		return EM_TRUE;
    }
	return EM_FALSE;
}

/**
 * [correct_cpap_speed description]调整转速
 * 说明：由于系统的诸多不确定性，因此这个函数需要更多的磨合才有可能最终确定调整方案
 * 注意事项：调整后的转速可能出乎预料，比如c-flex-speed会比inhale_speed高这类的现象
 *           需要最终按照实际情况加以修正
 * 这个函数不是必须调用的，如果数据不合适，应该避免调用这个函数
 * 调用这个函数会有潜在的危险---就是在“特定条件”下的控制失败，导致实际的控制波形与
 *     预期越来越远
 * @method correct_cpap_speed
 */
void correct_cpap_speed(void)
{
    int16_t press;
	int32_t speed;
	// 仅仅调整 cpap_control_dat.low_speed
	
	press = get_current_press(); 
	if((press >= main_control_data.epap - 15) &&(press <= main_control_data.epap + 15))
	{
		//如果当前的压力满足控制要求，则保存当前的控制转速
		cpap_control_dat.low_speed = PID_blower_control_data.Low_speed + PID_blower_control_data.blower_e_pap_adjust_val;
	}
	else
	{
		// 需要调整Low-speed
		press = main_control_data.epap - press; // 目标是epap
		press *= 20;
		speed =	(int32_t)cpap_control_dat.low_speed;
		speed += press;
		NO_LESS_THAN(speed, MIN_BLOWER_SPEED);
		NO_MORE_THAN(speed, MAX_BLOWER_SPEED);
		
		cpap_control_dat.low_speed =speed;
	}
	return;
#if 0	
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
    //if(cpap_control_dat.exhale_speed < MIN_BLOWER_SPEED)  cpap_control_dat.exhale_speed = MIN_BLOWER_SPEED;
    NO_LESS_THAN(cpap_control_dat.exhale_speed, MIN_BLOWER_SPEED);
    // correct c-flex
    if(cpap_control_dat.c_flex_speed > cpap_control_dat.exhale_speed)
        cpap_control_dat.c_flex_speed = cpap_control_dat.exhale_speed - 1000;
    if(cpap_control_dat.inhale_speed < cpap_control_dat.exhale_speed)
        cpap_control_dat.inhale_speed = cpap_control_dat.exhale_speed + 1000;
    if(cpap_control_dat.low_speed < cpap_control_dat.exhale_speed)
        cpap_control_dat.low_speed = cpap_control_dat.exhale_speed;

    // need reset low and high record
    cpap_control_dat.inhale_press_low = CPAP_PRESS_DEFAULT_SET_VALUE;
    cpap_control_dat.exhale_press_high = CPAP_PRESS_DEFAULT_SET_VALUE;
#endif	
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
 * [detect_cpap_active_breath description]调用之前需要配置触发环境！
 * @method detect_cpap_active_breath
 */
void detect_cpap_active_breath(void)
{
	trigger_force_confirm_type(EM_TRIGGER_TYPE_E_INS);
    trigger_set_current_status();
    // 要判断呼气流速状态，以决定是否开启吸气检测
    switch(is_patient_actived_breath())
    {
    case 1: // 触发了吸气相
        cpap_control_dat.cycles = 0;
        display_set_last_work_status(EM_PATIENT_BREATH_DETECT);

        //cpap_set_speed_inhale();
        stop_trigger();

		// 模式控制-- PID
	    set_PID_blower_adjust_mode(ENUM_PID_NO_ADJUST);

        set_patient_status(EM_PATIENT_INHALE_START);
        break;

    case 2: // 触发了呼气相
        cpap_control_dat.cycles = 0;
        display_set_last_work_status(EM_PATIENT_BREATH_DETECT);

        stop_trigger();

		// 模式控制-- PID
	    set_PID_blower_adjust_mode(ENUM_PID_NO_ADJUST);

        set_patient_status(EM_PATIENT_EXHALE_START);
        break;

    default: // 没有检测到触发
        break;
    }
}


/**
 * [detect_cpap_inhale_finished description]
 * @method detect_cpap_inhale_finished
 * @return                             [description]
 */
int8_t detect_cpap_inhale_finished(void)
{
	trigger_force_confirm_type(EM_TRIGGER_TYPE_CPAP_INS_FINISHED);
    trigger_set_current_status();
    // 要判断吸气流速状态，以决定是否开启呼气检测
    if(is_patient_actived_breath())
    {
		stop_trigger();
		// 模式控制-- PID
	    set_PID_blower_adjust_mode(ENUM_PID_NO_ADJUST);

        //cpap_set_speed_inhale();
        set_patient_status(EM_PATIENT_EXHALE_DETECT);
        start_trigger(EM_TRIGGER_TYPE_E_EXP);
        return EM_TRUE;
    }
    return EM_FALSE;
}

/**
 * [detect_cpap_exhale_finished description]
 * @method detect_cpap_exhale_finished
 * @return                             [description]
 */
int8_t detect_cpap_exhale_finished(void)
{
	trigger_force_confirm_type(EM_TRIGGER_TYPE_CPAP_EXP_FINISHED);
    // 记录当前状态，用于触发检测，这里检测呼气是否结束，
    trigger_set_current_status();

    // 要判断呼气流速状态，以决定是否开启吸气检测
    if(is_patient_actived_breath())
    {
        //cpap_set_speed_normal();

        // 先停止触发检测
        stop_trigger();

        // 呼气结束了，计算呼气潮气量，忽略之后的潮气量计算（主动放弃不必要的数据计算）
        breath_count_Te();

        // 暂时在这里调整PEEP阀
        correct_peep_for_leak();

        // 告诉系统，现在开始计算PEEP
        set_peep_start_flag();

		// 模式控制-- PID
	    cpap_start_epap_pid(); // 在这里启动压力控制的PID

		disable_volume_count();
        // change work status
        set_patient_status(EM_PATIENT_INHALE_DETECT);
		start_trigger(EM_TRIGGER_TYPE_E_INS);
        return EM_TRUE;
    }
    // 如果呼气时间超过3秒，则强制进入吸气检测
    //if(is_cpap_force_change_to_inhale_detect())
    if(is_over_breath_time(3000))
    {
        //cpap_set_speed_normal();

        // 先停止触发检测
        stop_trigger();

        // 计算呼气潮气量
        breath_count_Te();

        // start peep count
        set_peep_start_flag();

        // 暂时在这里调整PEEP阀
        correct_peep_for_leak();

		// 模式控制-- PID
	    cpap_start_epap_pid(); // 在这里启动压力控制的PID

		disable_volume_count();
        // change work status
        set_patient_status(EM_PATIENT_INHALE_DETECT);
		start_trigger(EM_TRIGGER_TYPE_E_INS);
        return EM_TRUE;
    }
    return EM_FALSE;
}

/**
 * [detect_cpap_breath_over_time description]
 * @method detect_cpap_breath_over_time
 * @return                              [description]
 */
int8_t detect_cpap_breath_over_time(void)
{
    // 如果超过10秒则进入EM_PATIENT_BREATH_DETECT
    //if(is_cpap_bad_breath())
    if(is_over_breath_time(10000))
    {
        stop_trigger();
        start_trigger(EM_TRIGGER_TYPE_E_EXP_INS);

		// 模式控制-- PID
	    cpap_start_epap_pid(); // 在这里启动压力控制的PID

		disable_volume_count();
        set_patient_status(EM_PATIENT_BREATH_DETECT);
        return EM_TRUE;
    }
    return EM_FALSE;
}

/**
 * [detect_cpap_active_inhale description]
 * @method detect_cpap_active_inhale
 * @return                           [description]
 */
int8_t detect_cpap_active_inhale(void)
{
	trigger_force_confirm_type(EM_TRIGGER_TYPE_E_INS);
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
            //if(is_cpap_long_time_breath())
            if(is_over_breath_time(6000))
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
        stop_trigger();
		// 模式控制-- PID
	    set_PID_blower_adjust_mode(ENUM_PID_NO_ADJUST);

        //cpap_set_speed_inhale();
        set_patient_status(EM_PATIENT_INHALE_START);
        return EM_TRUE;
    }
    return EM_FALSE;
}

/**
 * [cpap_adjust_with_Pmean_and_leak description]
 * @method cpap_adjust_with_Pmean_and_leak
 */
void cpap_adjust_with_Pmean_and_leak(void)
{
	if(cpap_control_dat.cycles ++ <= 1) return; // 头几个呼吸周期不做调整
    // 从第二个呼吸周期算起，每一个呼吸周期开始之前都应该调用此函数
    // 但是如果长期不呼吸时，呼吸周期要清零，以避免使用过早的已经失效的数据
	// 如果上一周期的Pmean没有达到要求，应该调整Low_speed

	// 如果泄漏量发生变化，则调整low_speed
	// 如果泄漏量发生变化，则调整PEEP, 确保PEEP阀的流量超过15L/min
	
	// 吸气
    cpap_control_dat.inhale_speed = cpap_control_dat.low_speed + cpap_control_dat.inhale_speed_adjust;
    // 呼气
    cpap_control_dat.exhale_speed = cpap_control_dat.low_speed - cpap_control_dat.exhale_speed_adjust;

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
        // 状态结束
        break;

    case EM_PATIENT_PIPE_OUT:       // 管路处于脱落状态
        // 状态结束
        break;

    /* *******************************************************************/
    // 正常工作状态
    /* *******************************************************************/
    case EM_PATIENT_BREATH_DETECT:  // 当吸气时间或呼气时间超过10秒，进入这个状态可能会好一些
		// 每300次调整一下
		if(cpap_control_dat.entry_cycles ++ > 300)
		{
			cpap_control_dat.entry_cycles = 0;
			set_leak_flow_force();
		}
		// 需要检测模式或参数变化
        // 检查UI设置是否有变化
        if(check_mode_change_flag())
            return;
		// 检测参数的变化
		if(check_breath_CPAP_parameter_change())
			return;
		// 检测患者呼吸动作
        detect_cpap_active_breath();

        // 状态结束
        break;

    case EM_PATIENT_INHALE_DETECT:
		// 检测吸气
        detect_cpap_active_inhale();

        // 超时检测，强行进入吸气检测
        detect_cpap_breath_over_time();

        // 状态结束
        break;

    case EM_PATIENT_INHALE_START:
        // 检查UI设置是否有变化
        if(check_mode_change_flag())
            return;

        if(!check_breath_CPAP_parameter_change())
        {
        	// 如果没有参数变化，则使用上次的Pmean 和 泄漏 进行相关调整
        	// To Be Determined (TBD)
        	cpap_adjust_with_Pmean_and_leak();
			cpap_start_epap_pid();
			//cpap_set_speed_inhale();
        }
		//else
		//	return; // 有参数变化
        reset_cpap_inhale_time_gate();
        set_patient_status(EM_PATIENT_INHALE_PERIOD);

        // 状态结束
        break;

    case EM_PATIENT_INHALE_PERIOD:
        if(is_cpap_protect_time_active())
        {
            if(!(is_cpap_inhale_protect_time_over()))
                break;
            else
            {
                //cpap_set_speed_inhale();
                cpap_start_epap_pid();
				
                reset_cpap_protect_active_flag();
                // start trigger
                start_trigger(EM_TRIGGER_TYPE_CPAP_INS_FINISHED);
            }
        }

        // for peep valve adjust
        refresh_cpap_inhale_press_low(courent_counted_press, courent_counted_flow);

        detect_cpap_inhale_finished();

        // 状态结束
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
        // 超时检测，强行进入吸气检测
        detect_cpap_breath_over_time();

        // 状态结束
        break;

    case EM_PATIENT_EXHALE_START:		
        cpap_set_speed_c_flex();
        reset_cpap_exhale_time_gate();

        set_patient_status(EM_PATIENT_EXHALE_PERIOD);

        // 状态结束
        break;

    case EM_PATIENT_EXHALE_PERIOD:

        // 如果是 C-Flex 保护期
        if(is_cpap_protect_time_active())
        {
            if(is_cpap_exhale_protect_time_over())
            {
                // 在保护期结束时，设置涡轮风机恢复到呼气状态（结束C-Flex）
                //cpap_set_speed_exhale();
				cpap_start_epap_pid();

                // 关闭保护期
                reset_cpap_protect_active_flag();

                // （如果叩击允许）启动（呼气相）叩击start trigger
                start_trigger(EM_TRIGGER_TYPE_CPAP_EXP_FINISHED);
                break;
            }
            else
                break; // 不做任何处理，直接返回了
        }

        // for blower speed adjust
        refresh_cpap_exhale_press_high(courent_counted_press, courent_counted_flow);

        // 呼气结束特征检测，以便进入吸气检测
        detect_cpap_exhale_finished();

        // 状态结束
        break;

    ////////////////////////////////////////////////////
    // 状态结束标记
    case EM_PATIENT_STATUS_ENDS:
        prepare_cpap_control_dat();// 错误状态，强制恢复到初始态

        // 状态结束
        break;

    default:
        prepare_cpap_control_dat();// 错误状态，强制恢复到初始态

        // 状态结束
        break;
    }
}

/**
 * [start_cpap_test description]
 * @method start_cpap_test
 * @param  val             [description]
 */
void start_cpap_test(uint32_t val)
{
	ui_set_data.epap = val;
	to_breath_cpap_mode();
}

// file end
