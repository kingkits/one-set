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
#include "rap_control.h"
#include "pcv_control.h"
#include "st_control.h"
#include "trigger_breath.h"
#include "../alarm/alarm.h"

ST_STV_CONTROL_DAT stv_control_dat;
#define STV_PRESS_DEFAULT_SET_VALUE 10000

void cpap_caculate_c_flex_blower_speed(uint16_t set_speed);
uint16_t get_cpap_c_flex_speed(void);

void oxygen_start(void);

/* ***************BiPAP-ST ******************
   ·    ST 模式：
   ·    1 有压力上升时间的控制（ST）
   ·    2 有压力释放（C-Flex）（ST）看情况而定，目前暂时不做
   ·    3 当长时间没有呼吸时，进入（T）模式
   ·    4 有呼吸时，是S模式（i-PAP+e_PAP）（e_PAP + ΔP {5cmH2O<=ΔP<=i_PAP}）
   ·    5 适当时刻启动叩击功能（RAP）
   ·    6 允许咳痰功能
   ·    7 适当的时机介入雾化功能
 * ******************************************
 */
/**
 * [is_S_mode_inhale_press_over description]
 * @method is_S_mode_inhale_press_over
 * @return [description]
 */
#define is_S_mode_inhale_press_over() (courent_counted_press > stv_control_dat.target_press + 10)

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

/**
 * [copy_stv_parameter_to_main_control_data description]
 * @method copy_stv_parameter_to_main_control_data
 */
void copy_stv_parameter_to_main_control_data(void)
{
    main_control_data.epap          = ui_set_data.epap;
    main_control_data.ipap          = ui_set_data.ipap;
    main_control_data.T_freq        = ui_set_data.T_freq;
    main_control_data.T_inhale_time = ui_set_data.T_inhale_time;
    main_control_data.T_rising_time = ui_set_data.T_rising_time;
}

/**
 * [stv_set_blower_speed description]
 * @method stv_set_blower_speed
 * @param  speed                [description]
 */
void stv_set_blower_speed(uint16_t speed)
{
    stv_control_dat.current_speed = speed;
    set_blower_speed(speed);
}

// 单独计算目标压力和Speed  // stv_control_dat.high_speed
// 如果PCV设置的IPAP与EPAP差值小于5cmH2O，则需要重新考虑计算目标压力
// 这里没有判断压力是否超出范围！！！！！需要小心使用
/**
 * [prepare_s_mode_target_press description]
 * @method prepare_s_mode_target_press
 * @param  epress                      [description]
 * @param  ipress                      [description]
 */
void prepare_s_mode_target_press(int16_t epress, int16_t ipress)
{
    if(ipress - epress >= 50)
    {
        stv_control_dat.target_press = ipress;
    }
    else
    {
        stv_control_dat.target_press = epress + 50;
    }
    // get speed
    stv_control_dat.high_speed = get_blower_speed_for_press_and_flow(main_control_data.ipap, CONST_ST_EXHALE_FLOW);
}

/**
 * [prepare_stv_control_data description]
 * @method prepare_stv_control_data
 */
void prepare_stv_control_data(void)
{
    ////////////////////////////////////////////////////////
    // T mode data (PCV -- Pressure Controlled Ventilation)
    ////////////////////////////////////////////////////////
    prepare_PCV_control_data();

    ////////////////////////////////////////////////////////
    // S mode data (PSV -- Pressure Spontaneous Ventilation)
    ////////////////////////////////////////////////////////
    prepare_s_mode_target_press(main_control_data.epap, main_control_data.ipap);

    stv_control_dat.low_speed = pcv_control_dat.low_speed;
    stv_control_dat.err_blower_speed = 0;
    cpap_caculate_c_flex_blower_speed(pcv_control_dat.low_speed);
    stv_control_dat.C_Flex_speed = get_cpap_c_flex_speed();
    stv_control_dat.inhale_peak_flow = 10000; //缺省，避免不必要的风险

    // 触发 重新设置触发点
    trigger_simple_set_base_inhale(1000, main_control_data.epap);
    start_trigger(EM_TRIGGER_TYPE_E_INS);

    // blower adjust
    stv_control_dat.inhale_press_low = STV_PRESS_DEFAULT_SET_VALUE;
    stv_control_dat.exhale_press_high = STV_PRESS_DEFAULT_SET_VALUE;

    // STV初始状态----第一次进入STV时的状态
    set_patient_status(EM_PATIENT_INHALE_DETECT);
}

/**
 * [adjust_stv_speed description]
 * @method adjust_stv_speed
 */
// Spontaneous模式的控制参数调整，未完成！！！
void adjust_stv_speed(void)
{
    if(stv_control_dat.inhale_press_low == STV_PRESS_DEFAULT_SET_VALUE) return;
    if(stv_control_dat.exhale_press_high == STV_PRESS_DEFAULT_SET_VALUE) return;

    ///////////////////////
    // blower S-high-speed
    ///////////////////////
    if(!stv_control_dat.err_blower_speed)
    {
        // 需要小心调整
    }
    else stv_control_dat.err_blower_speed = 0;

    //////////////////////
    // blower S-low-speed
    //////////////////////

    // 如果必要，设置C-Flex
    // blower S-C-flex-speed
}

/**
 * [refresh_stv_inhale_press_high description]
 * @method refresh_stv_inhale_press_high
 * @param  press                         [description]因为有主动的压力补偿，吸气需要检测最高压力
 * @param  flow                          [description]
 */
void refresh_stv_inhale_press_high(int16_t press, int32_t flow)
{
    if(flow < 2000) return; // do nothing,只检测吸气过程，在吸气流量小于2000（2升每分钟）以后，认为吸气结束，

    // 如果PID在控制压力，则返回----因为此时的检测值不正确
    if(is_pid_actived()) return;

    if(stv_control_dat.inhale_press_low == STV_PRESS_DEFAULT_SET_VALUE)
    {
        // 第一次设置时，直接设置压力值
        stv_control_dat.inhale_press_low = press;
        return;
    }

    // 只取压力最大值
    if(stv_control_dat.inhale_press_low < press)
        stv_control_dat.inhale_press_low = press;
}

/**
 * [refresh_stv_exhale_press_low description]
 * @method refresh_stv_exhale_press_low
 * @param  press                          [description]因为有主动的压力补偿，呼气需要检测最低压力
 * @param  flow                           [description]
 */
void refresh_stv_exhale_press_low(int16_t press, int32_t flow)
{
    if(flow > TRIGGER_EXHALE_FLOW_GATE) return; // do nothing在呼气流量小于2升每分钟，认为呼气结束，就不需要计算了（貌似此时已经进入PID控制）

    // 如果PID在控制压力，则返回----因为此时的检测值不正确
    if(is_pid_actived()) return;

    if(stv_control_dat.exhale_press_high == STV_PRESS_DEFAULT_SET_VALUE)
    {
        stv_control_dat.exhale_press_high = press;
        return;
    }

    if(stv_control_dat.exhale_press_high > press)
        stv_control_dat.exhale_press_high = press;
}

/**
 * [refresh_stv_control_data description]
 * @method refresh_stv_control_data
 */
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

/**
 * [stv_start_s_inhale description]
 * @method stv_start_s_inhale
 */
void stv_start_s_inhale(void)
{
    // 先判断有没有参数变化
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
    {
        // 没有参数变化，刷新下一呼吸周期的控制数据
        refresh_stv_control_data();
    }

    // 雾化器
    breath_force_operation_nebulizer(EM_ON);

    // S 模式的吸气相如何启动叩击？这是个亟待解决的问题

    // 吸气时间保护
    reset_stv_inhale_time_gate();

    // set speed high, must start inhale period
    stv_set_blower_speed(stv_control_dat.high_speed);

    // next work status
    set_patient_status(EM_PATIENT_INHALE_PERIOD);
}

/**
 * [stv_start_s_exhale description]
 * @method stv_start_s_exhale
 */
void stv_start_s_exhale()
{
    // 建立呼气时间保护
    reset_stv_exhale_time_gate();
    // 设置涡轮
    stv_set_blower_speed(stv_control_dat.low_speed); // 如果有C_flex则选择C-Flex
    // 叩击
    reset_rap_actions();

    set_patient_status(EM_PATIENT_EXHALE_PERIOD);
}

/**
 * [to_breath_stv_mode description]
 * @method to_breath_stv_mode
 */
void to_breath_stv_mode(void)
{
    // 进入STV模式时，需要做的操作
    ui_set_data.change_parameter_flag = 0;

    // 如果当前工作模式就是ST模式
    if(get_patient_breath_mode() != EM_VENTLATOR_BIPAP_ST_MODE)
    {
        // 从其他模式切换过来的
        set_patient_breath_mode(EM_VENTLATOR_BIPAP_ST_MODE);

        // 启动混氧
        oxygen_start();
    }

    // 把UI设置的参数复制过来
    copy_stv_parameter_to_main_control_data();

    // （第一次进入）初始化STV控制数据
    prepare_stv_control_data();
}

/**
 * [stv_resume_to_s_exhale description]
 * @method stv_resume_to_s_exhale
 */
// 由PCV回到Spontaneous
void stv_resume_to_s_exhale(void)
{
    // 需要准备S-mode 呼气开始的内容
    // 切换到下一状态
    //set_patient_status(EM_PATIENT_EXHALE_PERIOD);
    stv_start_s_exhale();
    // 是否需要记录本周期是PCV吸气？
}

/**
 * [is_stv_in_inhale_protect description]
 * @method is_stv_in_inhale_protect
 * @return                          [description]
 */
uint8_t is_stv_in_inhale_protect(void)
{
    if(is_stv_protect_time_active())
    {
        if(is_stv_inhale_protect_time_over())
        {
            reset_stv_protect_active_flag();
            // start trigger
            start_trigger(EM_TRIGGER_TYPE_STV_INS_FINISHED);
        }
        return EM_TRUE;
    }
    return EM_FALSE;
}

/**
 * [is_stv_detect_trigger_inhale description]
 * @method is_stv_detect_trigger_inhale
 * @return                              [description]
 */
uint8_t is_stv_detect_trigger_inhale(void)
{
    trigger_set_current_status();

    // 如果有触发则直接进入吸气相
    if(is_patient_actived_breath())
    {
        if(stv_control_dat.cycles ++ == 0)
        {
            //display_temp_data.last_work_status = EM_PATIENT_INHALE_DETECT;
            display_set_last_work_status(EM_PATIENT_BREATH_DETECT);
        }
        else
            display_set_last_work_status(EM_PATIENT_INHALE_DETECT);
        stop_trigger();
        set_patient_status(EM_PATIENT_INHALE_START); // S mode inhale start
        return EM_TRUE;
    }
    return EM_FALSE;
}

// 注意，每个呼吸周期只会调整一次
void adjust_stv_high_speed_for_error(void)
{
    stv_control_dat.high_speed -= 800;

    stv_set_blower_speed(stv_control_dat.low_speed);  // 强制切换到呼气相

    // 禁止下一轮自动调整
    stv_control_dat.err_blower_speed = 1; // error in speed count

    stop_trigger();
    set_patient_status(EM_PATIENT_EXHALE_START); // S mode inhale start
}

/**
 * [is_stv_detect_inhale_finished description]
 * @method is_stv_detect_inhale_finished
 * @return                               [description]
 */
uint8_t is_stv_detect_inhale_finished(void)
{
    trigger_set_current_status();

    // 如果满足吸气结束，则进入呼气相
    if(is_patient_actived_breath())
    {
        stop_trigger();
        set_patient_status(EM_PATIENT_EXHALE_START); // S mode inhale start
        return EM_TRUE;
    }

    // 如果压力过高则结束吸气
    if(is_S_mode_inhale_press_over())
    {
        // need to adjust high-speed
        adjust_stv_high_speed_for_error();
        return EM_TRUE;
    }
    return EM_FALSE;
}

/**
 * [is_stv_in_exhale_protect description]
 * @method is_stv_in_exhale_protect
 * @return                          [description]
 */
uint8_t is_stv_in_exhale_protect(void)
{
    if(is_stv_protect_time_active())
    {
        if(is_stv_exhale_protect_time_over())
        {
            reset_stv_protect_active_flag();
            // start trigger
            start_trigger(EM_TRIGGER_TYPE_STV_EXP_FINISHED);
        }

        return EM_TRUE;
    }
    return EM_FALSE;
}

/**
 * [is_stv_detect_exhale_finished description]
 * @method is_stv_detect_exhale_finished
 * @return                               [description]
 */
uint8_t is_stv_detect_exhale_finished(void)
{
    trigger_set_current_status();

    // 如果满足吸气结束，则进入呼气相
    if(is_patient_actived_breath())
    {
        stop_trigger();

        // 呼气结束了，计算呼气潮气量，忽略之后的潮气量计算（主动放弃不必要的数据计算）
        breath_count_Te();

        // 暂时在这里调整PEEP阀
        correct_peep_for_leak();

        // 告诉系统，现在开始计算PEEP
        set_peep_start_flag();

        //
        set_patient_status(EM_PATIENT_INHALE_DETECT); // S mode inhale start
        return EM_TRUE;
    }
    return EM_FALSE;
}


// STV 吸气时间超时
/**
 * [is_stv_inhale_overtiming description]
 * @method is_stv_inhale_overtiming
 * @return                          [description]
 */
uint8_t is_stv_inhale_overtiming(void)
{
    // 暂定超过PCV吸气时间
    return pcv_check_inhale_finished();
}

/**
 * [breath_stv_mode description] |  ^
 * @method breath_stv_mode       V  |
 */
void breath_stv_mode(void)
{
    switch(get_patient_status())
    {
    case EM_PATIENT_NOT_WORK://standby
        prepare_stv_control_data();

        // 状态结束
        break;
    case EM_PATIENT_BREATH_DETECT:
        prepare_stv_control_data();

        // 状态结束
        break;

    case EM_PATIENT_INHALE_DETECT:
        // 如果有触发，则进入S-INHALE
        if(is_stv_detect_trigger_inhale())
        {
            breath_set_trigger_flag();
            break;
        }

        // for blower speed adjust
        refresh_stv_exhale_press_low(courent_counted_press, courent_counted_flow);

        // 如果超时，将进入PCV- T_INHALE_START
        // 如果PCV吸气时间到，进入吸气相
        if(is_pcv_next_breath_time())
        {
            breath_reset_trigger_flag();
            break;
        }
        // 状态结束
        break;

    case EM_PATIENT_INHALE_START:
        // 检查UI设置是否有变化
        if(check_mode_change_flag())  return;

        stv_start_s_inhale();

        // 状态结束
        break;

    case EM_PATIENT_INHALE_PERIOD:
        //如果有气道压力报警，需要处理一下
        alarm_detect_inhale_period();//TBD

        // 吸气时间保护
        if(is_stv_in_inhale_protect()) break;


        // 强制进入下一状态
        set_patient_status(EM_PATIENT_EXHALE_DETECT);

        // 状态结束
        break;

    case EM_PATIENT_BREATH_HOLD:
        // 强制进入下一状态
        start_trigger(EM_TRIGGER_TYPE_STV_INS_FINISHED);// 防止没有设置触发数据
        set_patient_status(EM_PATIENT_EXHALE_DETECT);

        // 状态结束
        break;

    case EM_PATIENT_EXHALE_DETECT:	// 监测呼气触发条件
        // 检测吸气结束状态
        if(is_stv_detect_inhale_finished()) break;

        // for peep valve adjust
        refresh_stv_inhale_press_high(courent_counted_press, courent_counted_flow);

        // 超时的话，需要处理
        if(is_stv_inhale_overtiming())
        {
            stop_trigger();
            // 强制进入呼气
            set_patient_status(EM_PATIENT_EXHALE_START);
            break;
        }

        // 状态结束
        break;

    case EM_PATIENT_EXHALE_START:
        // 建立呼气时间保护
        reset_stv_exhale_time_gate();

        // 状态结束
        break;

    case EM_PATIENT_EXHALE_PERIOD:
        //如果有气道压力报警，需要处理一下
        alarm_detect_exhale_period();//TBD

        // 叩击过程
        do_rap_actions(EM_RAP_EXHALE_METHOD);

        // 如果在保护期则返回
        if(is_stv_in_exhale_protect()) break;


        // 如果超时，将进入PCV- T_INHALE_START
        // 如果PCV吸气时间到，进入吸气相
        if(is_pcv_next_breath_time())
        {
            breath_reset_trigger_flag();
            break;
        }
        // 如果叩击进行中，则结束
        if(is_rap_in_work()) break;

        // 检测呼气结束信号
        if(is_stv_detect_exhale_finished())
        {
            break;
        }
        // 状态结束
        break;

    // 机械通气过程
    case EM_PATIENT_T_INHALE_START:
        // to PCV
        breath_pcv_mode();

        // 状态结束
        break;

    case EM_PATIENT_T_INHALE_PERIOD:
        // to PCV
        breath_pcv_mode();

        // 状态结束
        break;

    case EM_PATIENT_T_EXHALE_DETECT:
        // to PCV
        breath_pcv_mode();

        // 状态结束
        break;

    case EM_PATIENT_T_EXHALE_START:

        // To S-exhale-start
        stv_resume_to_s_exhale();

        // 状态结束
        break;

    // 状态结束标记
    case EM_PATIENT_STATUS_ENDS:
        prepare_stv_control_data();

        // 状态结束
        break;
    default:
        prepare_stv_control_data();

        // 状态结束
        break;
    }
}

// ==================================
// file end =========================
// ==================================
