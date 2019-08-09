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
#include "pcv_control.h"
#include "trigger_breath.h"
#include "../alarm/alarm.h"
#include "rap_control.h"

extern float PCV_EXP_INCREASE_DATA[5][300];
extern int PCV_INCREASE_DATA_LEN[];
extern float PCV_INCREASE_DATA_END;

void oxygen_start(void);
void do_rap_actions(int type);

ST_PCV_CONTROL_DAT pcv_control_dat;

/**
 * [is_pcv_inhale_finished description]
 * @method is_pcv_inhale_finished
 * @return [description]
 */
#define is_pcv_inhale_finished() (system_direct_compare_tm1_be_tm2(ms_1_count, pcv_control_dat.exhale_start_gate))

/**
 * [is_pcv_one_breath_cycle_finished description]
 * @method is_pcv_one_breath_cycle_finished
 * @return [description]
 */
#define is_pcv_one_breath_cycle_finished() (system_direct_compare_tm1_be_tm2(ms_1_count, pcv_control_dat.breath_start_gate))

/**
 * [is_pcv_press_increase_finished description]
 * @method is_pcv_press_increase_finished
 * @return [description]
 */
#define is_pcv_press_increase_finished() (pcv_control_dat.inc_data_count >= pcv_control_dat.max_inc_data_num)
/**
 * [reset_rap_actions description]
 * @method reset_rap_actions
 */
//void reset_rap_actions(void)
//{
//	pcv_control_dat.rap_action_flag = EM_RAP_ACTION_IDLE;
//}


/**
 * [pcv_set_speed_high description]
 * @method pcv_set_speed_high
 */
//void pcv_set_speed_high(void)
//{
//    set_blower_speed(pcv_control_dat.high_speed);
//    // 这里可能还是需要PID
//}
#define pcv_set_speed_high() set_blower_speed(pcv_control_dat.high_speed)
/**
 * [pcv_set_speed_low description]
 * @method pcv_set_speed_low
 */
//void pcv_set_speed_low(void)
//{
//    // 这里准备触发的方案
//    set_blower_speed(pcv_control_dat.low_speed);
//}
#define pcv_set_speed_low() set_blower_speed(pcv_control_dat.low_speed)

void set_peep_valve_control_val(uint16_t val);


/**
 * [is_pcv_exhale_start_trigger description]
 * @method is_pcv_exhale_start_trigger
 * @return [description]
 */
uint8_t is_pcv_exhale_start_trigger(int32_t flow)
{
    // 如果有叩击，则等待叩击结束
    if(is_rap_enable())
    {
        if(pcv_control_dat.rap_action_flag != EM_RAP_ACTION_STOP)
            return EM_FALSE;
    }

    // 完成3/4呼气相
    if(system_direct_compare_tm1_be_tm2(ms_1_count, pcv_control_dat.T_exhale_3_4_gate))
        return EM_TRUE;

    // 等待300ms 最小呼气时间
    if(system_direct_compare_tm1_be_tm2(pcv_control_dat.exhale_start_gate + 300, ms_1_count))
        return EM_FALSE;

    // 满足呼气流量<2L/min
    if(flow > -2000)
        return EM_TRUE;

    return EM_FALSE;
}

//	float *inc_data_ptr;  //数据指针
//	int inc_data_count;   //计数（）
//	int max_inc_data_num; //最大计数
/**
 * [set_pcv_increase_data description] 准备上升数据的系数指针
 * @method set_pcv_increase_data
 */
void set_pcv_increase_data(void)
{
    if(main_control_data.T_rising_time > 4) main_control_data.T_rising_time = 4;
    pcv_control_dat.inc_data_ptr = (float *) PCV_EXP_INCREASE_DATA[main_control_data.T_rising_time];
    pcv_control_dat.max_inc_data_num = PCV_INCREASE_DATA_LEN[main_control_data.T_rising_time];
    pcv_control_dat.inc_data_count = 0; // reset
}

/**
 * [prepare_pcv_first_time description]
 * @method prepare_pcv_first_time
 */
void prepare_pcv_first_time(void)
{
    // 临时变量
    uint32_t t1;
    t1 = 60000 / main_control_data.T_freq; // 计算每一周期的时间，用于启动T模式
    pcv_control_dat.one_breath_time = t1;
    // 由呼吸频率决定，确定T触发时间
    // ！要由呼气相开始
    pcv_control_dat.breath_start_gate = ms_1_count + t1 - main_control_data.T_inhale_time;
    pcv_control_dat.exhale_start_gate = ms_1_count; // 不会再重复进入呼气相

    if(t1 > main_control_data.T_inhale_time + 500)
    {
        pcv_control_dat.T_exhale_3_4_gate = ((t1 - main_control_data.T_inhale_time) * 3 / 4) + ms_1_count;
    }
    else
    {
        pcv_control_dat.T_exhale_3_4_gate = pcv_control_dat.breath_start_gate - 200;
    }
}

/**
 * [prepare_pcv_time description]
 * @method prepare_pcv_time
 */
void prepare_pcv_time(void)
{
    // 临时变量
    uint32_t t1;
    t1 = 60000 / main_control_data.T_freq; // 计算每一周期的时间，用于启动T模式
    pcv_control_dat.one_breath_time        = t1;
    // 由呼吸频率决定，确定T触发时间
    // 这是正常的呼吸节奏，由吸气相开始！！！
    pcv_control_dat.breath_start_gate = ms_1_count + t1;
    pcv_control_dat.exhale_start_gate = ms_1_count + main_control_data.T_inhale_time;

    if(t1 > main_control_data.T_inhale_time + 500)
    {
        // 呼气相的3/4
        pcv_control_dat.T_exhale_3_4_gate = ((t1 - main_control_data.T_inhale_time) * 3 / 4) + pcv_control_dat.exhale_start_gate;
    }
    else
    {
        pcv_control_dat.T_exhale_3_4_gate = pcv_control_dat.breath_start_gate - 200; // 提前200ms
    }
}


/**
 * [pcv_stop_pid description]
 * @method pcv_stop_pid
 */
void pcv_stop_pid(void)
{
    set_PID_blower_adjust_mode(ENUM_PID_NO_ADJUST);
}

/**
 * [prepare_PCV_control_data description]
 * @method prepare_PCV_control_data
 */
void prepare_PCV_control_data(void)
{
    // 基础数据
    pcv_control_dat.cycles = 0;
    // 基础数据-- 显示数据
    if(!is_ui_change_parameters())
    {
        reset_display_data(); // 初始化显示数据
    }
    mark_breath_T_stmap();// 吸呼气 时间戳		强制
    // 基础数据-- 时间
    prepare_pcv_first_time();// 第一次进入PCV是由呼气相开始的

    // 基础数据-- 涡轮速度
    pcv_control_dat.low_speed = get_blower_speed_for_press_and_flow(main_control_data.epap, CONST_PCV_EXHALE_FLOW);
    pcv_control_dat.high_speed = get_blower_speed_for_press_and_flow(main_control_data.ipap, CONST_PCV_EXHALE_FLOW);
    pcv_control_dat.d_speed = (float) pcv_control_dat.high_speed - pcv_control_dat.low_speed;

    pcv_control_dat.err_blower_speed = 0; // reset with no error

    // first with epap
    pcv_set_speed_low();

    // 基础数据-- 呼气阀数据
    pcv_control_dat.peep_val = get_peep_value_with_press_and_flow(main_control_data.epap, CONST_PCV_EXHALE_FLOW);
    set_peep_valve_control_val(pcv_control_dat.peep_val); // 不用设置了。考虑到排痰，应该再复位一下

    // 模式控制-- 压力上升
    set_pcv_increase_data();

    // 模式控制-- 压力保持

    // 模式控制-- 压力下降（呼气）

    // 模式控制-- 触发
    trigger_simple_set_base_inhale(1000, main_control_data.epap);
    start_trigger(EM_TRIGGER_TYPE_E_INS);

    // 模式控制-- 叩击
    reset_rap_actions();

    // 模式控制-- PID
    pcv_stop_pid();
}

/**
 * [pcv_set_speed description]
 * @method pcv_set_speed
 * @return               [description]
 */
uint8_t pcv_set_speed(void)
{
    // count speed
    float cur_speed;
    uint16_t set_speed;

    if(is_pcv_press_increase_finished())
        return 1;

    cur_speed = pcv_control_dat.d_speed * (*(pcv_control_dat.inc_data_ptr));
    cur_speed += (float32_t) pcv_control_dat.low_speed;

    // currect ptr
    (pcv_control_dat.inc_data_count) ++;
    (pcv_control_dat.inc_data_ptr) ++;

    if(pcv_control_dat.inc_data_ptr >= (float *)&PCV_INCREASE_DATA_END)
    {
        // error 避免指针溢出
        // set press increase end

        pcv_control_dat.inc_data_count = pcv_control_dat.max_inc_data_num;
        // do not set blower with counted date,just use pcv_control_dat.high_speed
        //set_blower_speed(pcv_control_dat.high_speed); // never need
        return EM_TRUE; // tell system that this cycle is completed
    }
    // set blower
    set_speed = (uint16_t)cur_speed;

#if 0
    if(set_speed > MAX_BLOWER_SPEED)
    {
        set_speed = MAX_BLOWER_SPEED;
        // 这里是不是需要结束涡轮风机的加速了？
        pcv_control_dat.inc_data_count = pcv_control_dat.max_inc_data_num; // 设置成结束状态
    }
#else
    NO_MORE_THAN(set_speed, MAX_BLOWER_SPEED)
#endif
    set_blower_speed(set_speed);

    return EM_FALSE;
}

/**
 * [adjust_blower_speed_for_pcv description]
 * @method adjust_blower_speed_for_pcv
 */
void adjust_blower_speed_for_pcv()
{
    int32_t diff;
    int32_t abs_diff;
    /////////////////////////////
    // high speed for ipap
    /////////////////////////////


    // 这里可以考虑使用之前的PID数据
    // TBD
    if(!pcv_control_dat.err_blower_speed)
    {
        // 得到偏差
        diff = main_control_data.ipap - display_count_data.P_peak;
        if(diff > 0)
        {
            abs_diff = diff;
        }
        else
        {
            abs_diff = -diff;
        }


        if(abs_diff < 20)
        {
            ;
        }
        else if(abs_diff < 30)
        {
            pcv_control_dat.high_speed += diff * 3;
        }
        else if(abs_diff < 40)
        {
            pcv_control_dat.high_speed += diff * 10;
        }
        else if(diff < 0)
        {
            pcv_control_dat.high_speed -= 500;
        }
        else pcv_control_dat.high_speed += 500;
    }
    else
    {
        //已经调整完成了
    }

    // reset this flag
    pcv_control_dat.err_blower_speed = 0;

    // correct blower speed range
#if 0
    if(pcv_control_dat.high_speed <  MIN_BLOWER_SPEED)
    {
        pcv_control_dat.high_speed =  MIN_BLOWER_SPEED;
    }
    if(pcv_control_dat.high_speed > MAX_BLOWER_SPEED)
    {
        pcv_control_dat.high_speed = MAX_BLOWER_SPEED;
    }
#else
    NO_MORE_THAN(pcv_control_dat.high_speed, MAX_BLOWER_SPEED);
    NO_LESS_THAN(pcv_control_dat.high_speed, MIN_BLOWER_SPEED)
#endif

    /////////////////////////////
    // Low speed for epap/PEEP
    /////////////////////////////
    diff = main_control_data.epap - display_count_data.P_peep;
    if(diff > 0)
    {
        abs_diff = diff;
    }
    else
    {
        abs_diff = -diff;
    }


    if(abs_diff < 50)
    {
        ;
    }
    else if(abs_diff < 30)
    {
        pcv_control_dat.low_speed += diff * 3;
    }
    else if(abs_diff < 40)
    {
        pcv_control_dat.low_speed += diff * 5;
    }
    else if(diff < 0)
    {
        pcv_control_dat.low_speed -= 300;
    }
    else pcv_control_dat.low_speed += 300;
#if 0
    if(pcv_control_dat.low_speed <	MIN_BLOWER_SPEED)
    {
        pcv_control_dat.low_speed =  MIN_BLOWER_SPEED;
    }
    if(pcv_control_dat.low_speed > 30000)
    {
        pcv_control_dat.low_speed = 30000;
    }
#else
    NO_MORE_THAN(pcv_control_dat.low_speed, 30000);
    NO_LESS_THAN(pcv_control_dat.low_speed, MIN_BLOWER_SPEED)
#endif
    // correct press increase data
    pcv_control_dat.d_speed = (float) pcv_control_dat.high_speed - pcv_control_dat.low_speed;
}

/**
 * [copy_pcv_parameter_to_main_control_data description]
 * @method copy_pcv_parameter_to_main_control_data
 */
void copy_pcv_parameter_to_main_control_data(void)
{
    main_control_data.epap           = ui_set_data.epap;
    main_control_data.ipap           = ui_set_data.ipap;
    main_control_data.T_freq         = ui_set_data.T_freq;
    main_control_data.T_inhale_time  = ui_set_data.T_inhale_time;
    main_control_data.T_rising_time  = ui_set_data.T_rising_time;
}

/**
 * [check_breath_PCV_parameter_change description]
 * @method check_breath_PCV_parameter_change
 */
void check_breath_PCV_parameter_change(void)
{
    if(ui_set_data.change_parameter_flag)
    {
        // 把控制数据拷贝过来
        copy_pcv_parameter_to_main_control_data();

        // 重新计算一呼吸周期的控制数据
        prepare_PCV_control_data();
        // 重置标记
        ui_set_data.change_parameter_flag = 0;
        main_control_data.change_parameter_flag = 0;
    }
}

/**
 * [pcv_start_inhale_pid description]
 * @method pcv_start_inhale_pid
 */
void pcv_start_inhale_pid(void)
{
    Base_set_speed_for_ipap();
}

/**
 * [pcv_start_exhale_pid description]
 * @method pcv_start_exhale_pid
 */
void pcv_start_exhale_pid(void)
{
    PID_blower_control_data.Low_speed = pcv_control_dat.low_speed;
    PID_blower_control_data.blower_e_pap_adjust_val = 0;
    Base_set_speed_for_epap();
}

/**
 * [to_breath_pcv_mode description]
 * @method to_breath_pcv_mode
 */
void to_breath_pcv_mode(void)
{
    ui_set_data.change_parameter_flag = 0;

    // 如果当前工作模式不是PCV模式
    if(get_patient_breath_mode() != EM_VENTLATOR_BIPAP_PCV_MODE)
    {
        set_patient_breath_mode(EM_VENTLATOR_BIPAP_PCV_MODE);

        // 强制启动混氧
        oxygen_start();
    }

    // 把控制数据拷贝过来
    copy_pcv_parameter_to_main_control_data();

    // 计算下一呼吸周期的控制数据
    prepare_PCV_control_data();

    // 模式控制-- 进入pcv的状态
    set_patient_status(EM_PATIENT_T_INHALE_DETECT);
}

/**
 * [pcv_start_inhale description]
 * @method pcv_start_inhale
 */
void pcv_start_inhale(void)
{
    // 检查是否有参数变化
    check_breath_PCV_parameter_change();

    // 开始新的呼吸周期，需要重置时间和控制
    prepare_pcv_time();

    // 复位压力上升控制指针
    set_pcv_increase_data();

    // 如果有必要，调整一下风机速度(low_speed & high_speed)
    adjust_blower_speed_for_pcv();

    // 暂时在这里调整PEEP阀
    correct_peep_for_leak();

    // 停止PID调整
    pcv_stop_pid();

    // 设置起步状态
    // 涡轮
    pcv_set_speed();
    // 雾化
    breath_force_operation_nebulizer(EM_ON);

    // 进入下一状态
    set_patient_status(EM_PATIENT_T_INHALE_PERIOD);
}

/**
 * [adjust_pcv_high_speed_for_error description]
 * @method adjust_pcv_high_speed_for_error
 */
// 说明涡轮控制数据算得不准，需要修正一下
void adjust_pcv_high_speed_for_error(void)
{
    int d_speed;
    // 每次之修正1/4
    d_speed = (int) pcv_control_dat.max_inc_data_num - pcv_control_dat.inc_data_count;
    d_speed >>= 2; // /4
    d_speed = (int) (pcv_control_dat.high_speed - pcv_control_dat.low_speed) * d_speed / pcv_control_dat.max_inc_data_num;

    pcv_control_dat.high_speed = (int)pcv_control_dat.high_speed - d_speed;

    // 压力上升结束
    pcv_control_dat.inc_data_count = pcv_control_dat.max_inc_data_num;

    // 禁止下一轮自动调整
    pcv_control_dat.err_blower_speed = 1; // error in speed count
}


/**
 * [pcv_increase_press_period description]
 * @method pcv_increase_press_period
 */
void pcv_increase_press_period(void)
{
    //如果有气道压力报警，需要处理一下
    alarm_detect_inhale_period();//TBD

    // 如果压力达到要求 则 返回
    if(is_T_mode_inhale_press_over())
    {
        // 说明涡轮控制数据算得不准，需要修正一下
        adjust_pcv_high_speed_for_error();
    }

    // 压力上升阶段，禁止叩击及其它有干扰的操作，安心控制涡轮风机，并判断是否结束。
    if(pcv_set_speed())
    {
        set_patient_status(EM_PATIENT_BREATH_HOLD);
        // 关闭雾化
        breath_force_operation_nebulizer(EM_OFF);
        // 启动叩击
        reset_rap_actions();

        // 启动PID
        pcv_start_inhale_pid();  // 会自动调整涡轮风机
        //pcv_set_speed_high(); // 如果启动PID，就不需要这行代码
    }
}

/**
 * [pcv_check_inhale_finished description]
 * @method pcv_check_inhale_finished
 * @return                           [description]
 */
uint8_t pcv_check_inhale_finished(void)
{
    // 判断是否结束
    // PCV只有一个判断条件---时间
    if(is_pcv_inhale_finished())
    {
        // 可以考虑在这里关闭雾化
        //breath_force_operation_nebulizer(EM_OFF);

        // 停止PID
        pcv_stop_pid();
        // 收集当前控制的速度
        pcv_control_dat.last_high_speed = get_base_control_high_speed();
        // 设置涡轮
        pcv_set_speed_low();
        set_patient_status(EM_PATIENT_T_EXHALE_START);
        return EM_TRUE;
    }
    return EM_FALSE;
}

/**
 * [pcv_detect_inhale_trigger description]
 * @method pcv_detect_inhale_trigger
 * @return                           [description]
 */
uint8_t pcv_detect_inhale_trigger(void)
{
    if(is_pcv_exhale_start_trigger(courent_counted_flow))
    {
        // PCV应该打开触发通道
        start_trigger(EM_TRIGGER_TYPE_E_INS);

        pcv_start_exhale_pid();

        // 切换到下一状态
        set_patient_status(EM_PATIENT_T_INHALE_DETECT);

        // 呼气结束了，计算呼气潮气量，忽略之后的潮气量计算（主动放弃不必要的数据计算）
        breath_count_Te();

        // 告诉系统，现在开始计算PEEP
        set_peep_start_flag();
        return EM_TRUE;
    }
    return EM_FALSE;
}

/**
 * [is_pcv_inhale_trigger description]
 * @method is_pcv_inhale_trigger
 * @return                       [description]
 */
uint8_t is_pcv_inhale_trigger(void)
{
    trigger_set_current_status();

    // 如果有触发则直接进入吸气相
    if(is_patient_actived_breath())
    {
        if(pcv_control_dat.cycles ++ == 0)
        {
            //display_temp_data.last_work_status = EM_PATIENT_INHALE_DETECT;
            display_set_last_work_status(EM_PATIENT_BREATH_DETECT);
        }
        else
            display_set_last_work_status(EM_PATIENT_INHALE_DETECT);

        stop_trigger();

        pcv_stop_pid();

        set_patient_status(EM_PATIENT_T_INHALE_START);
        return EM_TRUE;
    }
    return EM_FALSE;
}

/**
 * [is_pcv_next_breath_time description]
 * @method is_pcv_next_breath_time
 * @return                         [description]
 */
uint8_t is_pcv_next_breath_time(void)
{
    // 如果时间到，进入吸气相
    if(is_pcv_one_breath_cycle_finished())
    {
        if(pcv_control_dat.cycles ++ == 0)
        {
            //display_temp_data.last_work_status = EM_PATIENT_INHALE_DETECT;
            display_set_last_work_status(EM_PATIENT_BREATH_DETECT);
        }
        else
            display_set_last_work_status(EM_PATIENT_INHALE_DETECT);
        set_patient_status(EM_PATIENT_T_INHALE_START);
        return EM_TRUE;
    }
    return EM_FALSE;
}

// 检测是否有呼气结束信号
/**
 * [is_pcv_exhale_finished description]
 * @method is_pcv_exhale_finished
 * @return                        [description]
 */
uint8_t is_pcv_exhale_finished(void)
{
    // 这个有待进一步优化
    if(courent_counted_flow > -1000) return EM_TRUE;
    return EM_FALSE;
}

/**
 * [breath_pcv_mode description]
 * @method breath_pcv_mode
 */
void breath_pcv_mode(void)
{
    switch(get_patient_status())
    {
    /* *******************************************************************/
    // 非正常状态
    /* *******************************************************************/
    case EM_PATIENT_NOT_WORK:       // 待机 standby
        // PCV 禁止进入这个状态
        to_breath_pcv_mode();
        break;

    case EM_PATIENT_PIPE_OUT:       // 管路处于脱落状态
        break;

    /* *******************************************************************/
    // 正常工作状态
    /* *******************************************************************/
    case EM_PATIENT_BREATH_DETECT:  // 专门用于测试的状态
        // PCV 禁止进入这个状态
        to_breath_pcv_mode();

        break;

    /* *******************************************************************/
    // 机械通气过程 pcv 有且仅有机械通气
    /* *******************************************************************/
    case EM_PATIENT_T_INHALE_START:
        // 检查UI设置是否有变化
        if(check_mode_change_flag())
            return;

        pcv_start_inhale();

        // 状态结束
        break;

    case EM_PATIENT_T_INHALE_PERIOD:
        // PCV关键步骤
        pcv_increase_press_period();

        // 状态结束
        break;

    case EM_PATIENT_BREATH_HOLD:
        // 叩击过程
        do_rap_actions(EM_RAP_INHALE_METHOD);

        //
        pcv_check_inhale_finished();

        // 状态结束
        break;

    case EM_PATIENT_T_EXHALE_START:
        // 叩击
        reset_rap_actions();
        set_patient_status(EM_PATIENT_T_EXHALE_PERIOD);

        // 状态结束
        break;

    case EM_PATIENT_T_EXHALE_DETECT:
        //无需检测
        set_patient_status(EM_PATIENT_T_EXHALE_PERIOD);// 防止错误发生

        // 状态结束
        break;

    case EM_PATIENT_T_EXHALE_PERIOD:
        //如果有气道压力报警，需要处理一下
        alarm_detect_exhale_period();//TBD

        // 叩击过程
        do_rap_actions(EM_RAP_EXHALE_METHOD);

        pcv_detect_inhale_trigger();

        // 状态结束
        break;

    case EM_PATIENT_T_INHALE_DETECT:
        // 如果有触发则直接进入吸气相
        if(is_pcv_inhale_trigger())	break;

        // 如果满足要求，则计算潮气量
        if(is_pcv_exhale_finished())
        {
            // 呼气结束了，计算呼气潮气量，忽略之后的潮气量计算（主动放弃不必要的数据计算）
            breath_count_Te();
        }

        // 如果时间到，进入吸气相
        if(is_pcv_next_breath_time())
        {
            stop_trigger();
            break;
        }
        // 状态结束
        break;

    /* *******************************************************************/
    // 状态结束标记 不需要代码
    /* *******************************************************************/
    case EM_PATIENT_STATUS_ENDS:
        // PCV 禁止进入这个状态
        to_breath_pcv_mode();

        // 状态结束
        break;

    default:
        // PCV 禁止进入这个状态
        to_breath_pcv_mode();
        break;
    }
}

// file: pcv_control.c end
