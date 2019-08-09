// file: breath_control.c
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

/******************************************************************/
int16_t get_current_press(void);
void    oxygen_start(void);
void    oxygen_stop(void);
void    breath_change_nebulizer(uint8_t status);
uint8_t get_blower_current_set_speed(void);
void    set_one_cycle_complete_flag(void);
void    reset_oxygen_mixer_control_data(void);
uint8_t check_CPAP_breath_inspire_start(void);

/******************************************************************/

/******************************************************************/
// 暂时借用st_ST_mode_control_data的数据结构
extern st_ST_mode_control_data_DEFINES st_PCV_mode_control_data;
#define st_PCV_mode_control_data st_ST_mode_control_data

#define get_current_press() display_count_data.Press
/******************************************************************/

//这里定义变量
/******************************************************************/
static uint8_t global_patient_breath_mode;
/******************************************************************/
#if PEEP_VALVE_CHANGE_ENABLE
static uint8_t peep_delay_actived_flag = 0;
#endif
static uint8_t rap_run_flag = 0;  // 在一个呼吸阶段内，只能激活一次叩击动作
#if USE_BREATH_CYCLES_ADJUST
static uint8_t breath_adjust_flag = 0;  // 在一个呼吸阶段内，只能激活一次
#endif

/******************************************************************/
// 代码开始
/**
 * [复制UI设置参数到缓冲区]
 * @method copy_control_data
 * @param  src               [数据源]
 * @param  tar               [目标]
 */
__weak void copy_control_data(ST_MAIN_CONTROL_DATA *src, ST_MAIN_CONTROL_DATA *tar)
{
    tar->change_mode_flag          = src->change_mode_flag;
    tar->change_parameter_flag     = src->change_parameter_flag;
    tar->cough_finish_types        = src->cough_finish_types;
    tar->cough_finish_val          = src->cough_finish_val;
    tar->cough_hold_time           = src->cough_hold_time;
    tar->cough_prepare_press       = src->cough_prepare_press;
    tar->cough_press_increase_time = src->cough_press_increase_time;
    tar->cough_start_press         = src->cough_start_press;
    tar->cough_start_vt            = src->cough_start_vt;
    tar->c_flex_level              = src->c_flex_level;
    tar->epap                      = src->epap;
    tar->flow                      = src->flow;
    tar->gate_flow_limited_e       = src->gate_flow_limited_e;
    tar->gate_flow_limited_i       = src->gate_flow_limited_i;
    tar->gate_mode_e               = src->gate_mode_e;
    tar->gate_mode_i               = src->gate_mode_i;
    tar->gate_press_limited_e      = src->gate_press_limited_e;
    tar->gate_press_limited_i      = src->gate_press_limited_i;
    tar->ipap                      = src->ipap;
    tar->status                    = src->status;
    tar->gate_mode_e               = src->gate_mode_e;
    tar->gate_mode_i               = src->gate_mode_i;
    tar->trigger_flow_level_e      = src->trigger_flow_level_e;
    tar->trigger_flow_level_i      = src->trigger_flow_level_i;
    tar->trigger_mode              = src->trigger_mode;
    tar->trigger_press_level_e     = src->trigger_press_level_e;
    tar->T_freq                    = src->T_freq;
    tar->T_inspire_time            = src->T_inspire_time;
    tar->T_press                   = src->T_press;
    tar->T_rising_time             = src->T_rising_time;
    tar->work_mode                 = src->work_mode;
}

/**
 * [设置UI变更参数请求]
 * @method set_ui_change_parameters
 */
__weak void set_ui_change_parameters(void)
{
    ui_set_data.change_parameter_flag = 1;
}

/**
 * [判断是否有UI变更参数的请求]
 * @method is_ui_change_parameters
 * @return                         [有参数变换=1]
 */
__weak uint8_t is_ui_change_parameters(void)
{
    return ui_set_data.change_parameter_flag;
}

// 不进行合法性检测，需要调用者自己确认
/**
 * [设置系统工作模式]
 * @method set_patient_breath_mode
 * @param  mode                    [要设置的模式]
 */
__weak void set_patient_breath_mode(uint8_t mode)
{
    global_patient_breath_mode = mode;
}

/**
 * [读取当前工作模式]
 * @method get_patient_breath_mode
 * @return                         [当前的模式]
 */
__weak uint8_t get_patient_breath_mode(void)
{
    return global_patient_breath_mode;
}

/**
 * [设置涡轮PID控制状态]
 * @method set_PID_blower_control_status
 * @param  status                        [要设置的状态]
 */
__weak void set_PID_blower_control_status(uint8_t status)
{
    main_control_data.status = status;
}

/**
 * [读取当前工作状态]--指通气状态
 * @method get_main_control_status
 * @return                         [状态值]
 */
__weak uint8_t get_main_control_status(void)
{
    return main_control_data.status;
}

/**
 * [获得下一压力值]
 * @method get_press_increase
 * @param  press_inc_data_ptr [description]
 * @param  steps              [description]
 * @return                    [description]
 */
__weak int32_t get_press_increase(ST_INCREASE_PRESS_STEP_DATA_DEFINES *press_inc_data_ptr, int32_t steps)
{
    int32_t val;
    val  = (press_inc_data_ptr->d_press) * steps;
    val /= press_inc_data_ptr->total_steps;
    return val;
}

/**
 * [设置epap控制参数]
 * @method set_main_control_epap_data
 * @param  epap    []
 */
__weak void set_main_control_epap_data(int16_t epap)
{
    main_control_data.epap = epap;
}

// 如果当前气道压力比epap-delta_p 小
/**
 * [如果当前气道压力比epap-delta_p 小，则返回 True]
 * @method is_epap_gate_ready
 * @param  delta_p            [要减去的值]
 * @return                    [description]
 */
__weak uint8_t is_epap_gate_ready(int16_t delta_p)
{
    return main_control_data.epap - delta_p > get_current_press();
}

/**
 * [初始化控制参数]
 * @method reset_main_control_data
 */
__weak void reset_main_control_data(void)
{
    // 默认为CPAP模式，所有的压力都是10cmH2O,流量为10 L/min
    // base parameters(UI set)
    main_control_data.epap                      =   100;
    main_control_data.ipap                      =   100;
    main_control_data.flow                      = 10000;
    main_control_data.T_press                   =   100;
    main_control_data.T_freq                    =    10;
    main_control_data.T_inspire_time            =  2000;
    main_control_data.T_rising_time             =  1000;
    main_control_data.c_flex_level              =     0;// 呼气时压力释放强度

    // parameters change flages(affected by UI)
    main_control_data.change_mode_flag          =     0;
    main_control_data.change_parameter_flag     =     0;
    main_control_data.work_mode                 = EM_VENTLATOR_STANDBY_MODE;

    // Special parameters
    // cough
    main_control_data.cough_finish_types        =     2;
    main_control_data.cough_finish_val          =     5;
    main_control_data.cough_hold_time           =  1000;
    main_control_data.cough_prepare_press       =   200;
    main_control_data.cough_press_increase_time =  2000;
    main_control_data.cough_start_press         =   220;
    main_control_data.cough_start_vt            =   400;

    // gate
    main_control_data.gate_flow_limited_e       =     0;
    main_control_data.gate_flow_limited_i       =     0;
    main_control_data.gate_mode_e               =     0;
    main_control_data.gate_mode_i               =     0;
    main_control_data.gate_press_limited_e      =     0;
    main_control_data.gate_press_limited_i      =     0;

    // trigger
    main_control_data.trigger_flow_level_e      =     0;
    main_control_data.trigger_flow_level_i      =     0;
    main_control_data.trigger_mode              =     0;
    main_control_data.trigger_press_level_e     =     0;

    // others
    copy_control_data((ST_MAIN_CONTROL_DATA *)&main_control_data, (ST_MAIN_CONTROL_DATA *)&ui_set_data);
    set_PID_blower_control_status(EM_BREATH_STANDBY);// 调整数据存储位置
    set_PID_blower_adjust_mode(ENUM_PID_NO_ADJUST);
}

/**
 * [初始化UI传输参数]
 * @method reset_ui_set_data
 */
__weak void reset_ui_set_data(void)
{
    // 默认为CPAP模式，所有的压力都是10cmH2O,流量为10 L/min
    ui_set_data.c_flex_level          =     0;// 呼气时压力释放强度
    ui_set_data.change_mode_flag      =     0;
    ui_set_data.change_parameter_flag =     0;
    ui_set_data.epap                  =   100;
    ui_set_data.ipap                  =   100;
    ui_set_data.flow                  = 10000;
    ui_set_data.T_press               =   100;
    ui_set_data.T_freq                =    10;
    ui_set_data.T_inspire_time        =  2000;
    ui_set_data.T_rising_time         =  1000;
    ui_set_data.status                = EM_BREATH_STANDBY;
    ui_set_data.work_mode             = EM_VENTLATOR_STANDBY_MODE;
}

/**
 * [变更工作模式]
 * @method set_changemode_actived_mode
 * @param  mode                        [description]
 */
__weak void set_changemode_actived_mode(uint8_t mode)
{
    ui_set_data.work_mode        = mode;
    ui_set_data.change_mode_flag = 1;
}

/**
 * [强制停止咳痰工作]
 * @method set_cough_stop_mode
 */
__weak void set_cough_stop_mode(void)
{
    // force to stop cough blower
    stop_cough_blower_work();

    if(global_patient_breath_mode == EM_VENTLATOR_COUGH_MODE)
    {
        cough_back_to_prev_work();
    }
    else
    {
        ui_set_data.change_mode_flag = 1;
        // 恢复原来的工作状态
        set_patient_breath_mode(cough_control_data.prev_work_mode);
    }
}

// breath cycles adjustment
#if USE_BREATH_CYCLES_ADJUST
/**
 * [设置调整数据]
 * @method breath_cycle_adjust_set_ipap
 * @return                              [description]
 */
__weak void breath_cycle_adjust_set_ipap(void)
{
    breath_cycles_save_peep_data(EM_BREATH_ADJUST_SAVE_I_CONTROL, get_peep_pwm_comtrol_data(), get_expire_flow());
    breath_cycles_save_blower_speed_data(EM_BREATH_ADJUST_SAVE_I_CONTROL, get_blower_current_set_speed(), display_count_data.Press);
}

/**
 * [breath_cycle_adjust_set_epap description]
 * @method breath_cycle_adjust_set_epap
 */
__weak void breath_cycle_adjust_set_epap(void)
{
    breath_cycles_save_peep_data(EM_BREATH_ADJUST_SAVE_E_CONTROL, get_peep_pwm_comtrol_data(), get_expire_flow());
    breath_cycles_save_blower_speed_data(EM_BREATH_ADJUST_SAVE_E_CONTROL, get_blower_current_set_speed(), display_count_data.Press);
}

/**
 * [breath_cycle_adjust_set_T_press description]
 * @method breath_cycle_adjust_set_T_press
 */
__weak void breath_cycle_adjust_set_T_press(void)
{
    breath_cycles_save_peep_data(EM_BREATH_ADJUST_SAVE_T_CONTROL, get_peep_pwm_comtrol_data(), get_expire_flow());
    breath_cycles_save_blower_speed_data(EM_BREATH_ADJUST_SAVE_T_CONTROL, get_blower_current_set_speed(), display_count_data.Press);
}

/**
 * [breath_cycles_adjust_work description]
 * @method breath_cycles_adjust_work
 */
__weak void breath_cycles_adjust_work(void)
{
    if(!is_breath_cycles_adjust_data_ready())
        return;

    // EPAP
    PID_blower_control_data.Low_peep     = breath_cycles_adjust_peep_data(EM_BREATH_ADJUST_SAVE_E_CONTROL, main_control_data.flow);
    //PID_blower_control_data.Low_speed    = breath_cycles_adjust_speed_data(EM_BREATH_ADJUST_SAVE_E_CONTROL, main_control_data.epap);
    if(display_count_data.P_peep > main_control_data.epap + 5)
    {
        PID_blower_control_data.Low_speed -= 100;
    }
    if(display_count_data.P_peep < main_control_data.epap - 5)
    {
        PID_blower_control_data.Low_speed += 60;
    }
    // IPAP
    PID_blower_control_data.High_peep    = breath_cycles_adjust_peep_data(EM_BREATH_ADJUST_SAVE_I_CONTROL, main_control_data.flow);
    PID_blower_control_data.High_speed   = breath_cycles_adjust_speed_data(EM_BREATH_ADJUST_SAVE_I_CONTROL, main_control_data.ipap);
    PID_blower_control_data.T_High_speed = PID_blower_control_data.High_speed;

    if(PID_blower_control_data.Low_speed > PID_blower_control_data.High_speed - 200)
    {
        PID_blower_control_data.Low_speed = PID_blower_control_data.High_speed - 200;
    }
    PID_blower_control_data.C_Flex_speed = PID_blower_control_data.Low_speed - main_control_data.c_flex_level * 50;
#if 0
    // correct error data
    if(PID_blower_control_data.Low_peep > PID_blower_control_data.High_peep)
        PID_blower_control_data.Low_peep = PID_blower_control_data.High_peep;

    if(PID_blower_control_data.Low_speed > PID_blower_control_data.High_speed)
    {
        PID_blower_control_data.High_speed = PID_blower_control_data.Low_speed + 200;
        PID_blower_control_data.T_High_speed = PID_blower_control_data.High_speed;
    }
#endif
}
#endif


/* ***************Self test **************
 */
/**
 * [breath_selftest_mode description]
 * @method breath_selftest_mode
 */
__weak void breath_selftest_mode(void)
{
}

/**
 * [to_breath_selftest_mode description]
 * @method to_breath_selftest_mode
 */
__weak void to_breath_selftest_mode(void)
{
}

/* ***************STANDBY **************
 */
#if TEST_LEAK_FLOW
__weak void do_leak_test_actions(void);
#endif

#if TEST_PEEP_VAL
__weak void do_peep_test_actions(void);
#endif

/**
 * [breath_standby_mode description]
 * @method breath_standby_mode
 */
__weak void breath_standby_mode(void)
{
    check_mode_change_flag();
    // 如果有检测要求执行检测流程
#if TEST_LEAK_FLOW
    do_leak_test_actions();
#endif
#if TEST_PEEP_VAL
    do_peep_test_actions();
#endif
}

/**
 * [to_breath_standby_mode description]
 * @method to_breath_standby_mode
 */
__weak void to_breath_standby_mode(void)
{
    // 清除PEEP调整数据
    init_breath_cycles_adjust_data();

    // 停止PID
    set_PID_blower_adjust_mode(ENUM_PID_NO_ADJUST);
    //osDelay(20);

    // 关闭rap
    set_breath_rap_disable();

    // 关闭雾化器
    breath_change_nebulizer(EM_OFF);

    // 关闭混氧
    oxygen_stop();
    //reset_oxygen_mixer_control_data();

    // 涡轮风机最小压力模式
    set_blower_speed(5000);

    // 回流阀开到最大
    // set_overflow_valve(0);

    // 关负压风机
    stop_cough_blower_work();

    // 关咳痰阀
    COUGH_VALVE_CLOSE();

    // 开吸气阀
    INSPIRE_VALVE_OPEN();

    // 强制开PEEP阀到最小
    set_peep_press(0);

    // 清除所有显示数据
    reset_display_data();

    // 给UI发送显示数据
    set_one_cycle_complete_flag();

    // 禁止报警
    // 发送STANDBY MODE状态给UI

    // 设置工作模式及工作状态
    global_patient_breath_mode = EM_VENTLATOR_STANDBY_MODE;
    set_patient_status(EM_PATIENT_NOT_WORK);
}

/*
 * ***************CPAP **************
 */
/**
 * [copy_CPAP_parameter_to_main_control_data description]
 * @method copy_CPAP_parameter_to_main_control_data
 */
__weak void copy_CPAP_parameter_to_main_control_data(void)
{
    main_control_data.epap         = ui_set_data.epap;
    main_control_data.ipap         = ui_set_data.epap;
    main_control_data.flow         = ui_set_data.flow;
#if CFLEX_ENABLE_FOR_CPAP
    main_control_data.c_flex_level = ui_set_data.c_flex_level;
#endif
}


/* ***************CPAP ******************
   ·    CPAP模式：
   ·    1 有压力释放（C-Flex）
   ·    2 添加叩击功能（RAP）
   ·    3 允许咳痰命令
   ·    4 允许雾化功能
 * CPAP的工作时，需要在明确检测到吸气时，
     才能给雾化器控制，时间为0.5秒，0.5秒后，
     才能打开呼气检测指令
 *******************************************
 */
/**
 * [breath_cpap_mode description]
 * @method breath_cpap_mode
 */
__weak void breath_cpap_mode(void)
{
    static uint8_t monitor_flag = 0;
    // CPAP 中暂时不做雾化器的自动控制
    //breath_change_nebulizer(is_breath_nebulizer_turned_on());
    switch(get_patient_status())
    {
    case EM_PATIENT_NOT_WORK:   // 未工作
        check_mode_change_flag();
        break;

    case EM_PATIENT_BREATH_DETECT: // 呼吸检测
        check_mode_change_flag();
        // 检测吸气
        if(check_CPAP_breath_inspire_start())
        {
            display_temp_data.last_work_status = EM_PATIENT_BREATH_DETECT;
            set_patient_status(EM_PATIENT_INSPIRE_START);
            breath_set_trigger_flag();
            break;
        }
        break;

    case EM_PATIENT_INSPIRE_DETECT: // 吸气检测
        // 如果有主动吸气的动作，则认为吸气开始
        // ****** 如果长时间（30s）检测不到吸气或呼气，则视为呼吸停止，需要强制到EM_PATIENT_BREATH_DETECT状态中
        if(check_CPAP_breath_inspire_start()) //	即患者出现了主动吸气的动作
        {
            // Mark this is a trigger breath（患者有自主呼吸的过程）
            breath_set_trigger_flag();
            display_temp_data.last_work_status = EM_PATIENT_INSPIRE_DETECT;
            set_patient_status(EM_PATIENT_INSPIRE_START);
            break;
        }
        break;

    case EM_PATIENT_INSPIRE_START: // 吸气开始
        if(check_mode_change_flag())
            break;
        rap_run_flag = 0;
        monitor_flag = 0;
        // 在此处打开雾化器，并定时
        // 为了不影响吸呼气检测，及雾化效果，当有雾化器打开时，吸气相不做叩击，
        // 只在呼气相 进行叩击，当雾化器不工作时，吸气相会启动叩击，
        // 并在叩击后0.5秒，启动呼气检测。
        // 进入快速流量补偿模式
        breath_force_operation_nebulizer(EM_ON);

        set_patient_status(EM_PATIENT_INSPIRE_PERIOD);
        break;

    case EM_PATIENT_INSPIRE_PERIOD: // 吸气过程
        if(is_breath_nebulizer_turned_on())
        {
            // 在此处处理叩击及关闭雾化器
            if(breath_get_time_interval() < 500)
                break;
            breath_force_operation_nebulizer(EM_OFF);
        }
        else
        {
            // 当系统没有打开雾化器时，考虑在此进行叩击，叩击时间依照系统设定
            if(is_rap_enable() && rap_run_flag == 0)
            {
                // 如果没启动则启动
                if(!is_rap_actived())
                {
                    set_breath_rap_start();
                    break;
                }
                if(!is_rap_time_over(EM_RAP_INSPIRE_METHOD))
                {
                    // 如果叩击时间不够，则维持
                    break;
                }
                // 否则停止叩击，等待进入下一模式
                set_breath_rap_stop();
                rap_run_flag = 1;
            }
        }
        // 当叩击及雾化器都关闭后，进入
        set_patient_status(EM_PATIENT_EXPIRE_DETECT);
        break;

    case EM_PATIENT_BREATH_HOLD: // 屏气保持
        set_patient_status(EM_PATIENT_EXPIRE_DETECT);
        break;

    case EM_PATIENT_EXPIRE_DETECT: // 呼气检测
        if(breath_get_time_interval() < 200)
            break;
        if(check_breath_expire_start())
        {
            set_patient_status(EM_PATIENT_EXPIRE_START);
            break;
        }

        break;

    case EM_PATIENT_EXPIRE_START: // 呼气开始
#if CFLEX_ENABLE_FOR_CPAP
        //S_T_set_speed_cflex();
#endif
#if PEEP_VALVE_CHANGE_ENABLE
#if CFLEX_ENABLE_FOR_CPAP
        //open_peep();
        peep_delay_actived_flag = 0;
#endif
#endif
#if BREATH_ENABLE_C_FLEX
        // 这里处理c-flex
#endif
        rap_run_flag = 0;
        monitor_flag = 0;
        // 吸气转呼气时，需要处理的一些事件
        set_patient_status(EM_PATIENT_EXPIRE_PERIOD);
        break;
    case EM_PATIENT_EXPIRE_PERIOD: // 呼气过程
        // 快速呼气补偿
#if PEEP_VALVE_CHANGE_ENABLE
#if CFLEX_ENABLE_FOR_CPAP
        if(!peep_delay_actived_flag)
        {
            peep_delay_actived_flag = breath_waiting_peep_e_ready();
        }
#endif
#endif
        // 这里处理C-flex 结束
#if BREATH_ENABLE_C_FLEX
        // C-flex 结束判定
        // 貌似可以和上面的呼气阀状态控制合并处理
        if((peep_delay_actived_flag == 1) && (breath_get_time_interval() > 500))
        {
            Base_set_speed_for_epap();
            peep_delay_actived_flag = 2;
        }
#endif
        if(is_rap_enable() && rap_run_flag == 0)
        {
            // 如果没启动则启动
            if(!is_rap_actived())
            {
                set_breath_rap_start();
                break;
            }
            if(!is_rap_time_over(EM_RAP_EXPIRE_METHOD))
            {
                // 如果叩击时间不够，则维持
                break;
            }
            // 否则停止叩击，等待进入下一模式
            set_breath_rap_stop();
            rap_run_flag = 1;

            // 计算呼气潮气量
            breath_count_Te();
        }
        // 首先要检测是不是出现了呼气结束信号---（非常重要）这时还没有吸气动作
        if(monitor_flag == 0)
        {
            if(monitor_expire_end())
            {
                // 告诉系统，现在开始计算PEEP
                set_peep_start_flag();
                breath_set_expire_trigger_flag(); // 这个主动呼气的标记暂时没有用

                monitor_flag = 1; // 避免重复执行上面的操作
            }
            break;
        }

        break;
    default:
        break;
    }
}

/**
 * [to_default_breath_status description]
 * @method to_default_breath_status
 */
__weak void to_default_breath_status(void)
{

    // 关闭叩击
    set_breath_rap_stop();

    // 是不是要关闭雾化器
    //breath_force_operation_nebulizer(EM_OFF);

    // 是不是要强制关闭咳痰？
    //stop_cough_blower_work();

    // 状态
    set_patient_status(EM_PATIENT_BREATH_DETECT);

    // 必要的初始化信息
    // 设为 呼气初始状态
    // 需要重新设置PID调整的数据
    Blower_start_epap_control(); // 使用EPAP调整模式
}

// 切换到CPAP模式
/**
 * [to_breath_cpap_mode description]
 * @method to_breath_cpap_mode
 */
__weak void to_breath_cpap_mode(void)
{
    oxygen_start();

    copy_CPAP_parameter_to_main_control_data();

    set_PID_blower_control_base_speed(EM_BASE_SPEED_EPAP); // Low_speed
    set_PID_blower_control_base_speed(EM_BASE_SPEED_C_FLEX);// C_Flex_speed
    PID_blower_control_data.Low_peep  = get_peep_value_with_press_and_flow(main_control_data.epap, main_control_data.flow);
    // 模式标记
    global_patient_breath_mode = EM_VENTLATOR_CPAP_MODE;
    to_default_breath_status();
    // ***************！！！！
}

/* ***************BiPAP-ST ******************
   ·    ST 模式：
   ·    1 有压力上升时间的控制（ST）
   ·    2 有压力释放（C-Flex）（ST）
   ·    3 当长时间没有呼吸时，进入（T）模式
   ·    4 有呼吸时，是S模式（i-PAP+e_PAP）
   ·    5 添加叩击功能（RAP）
   ·    6 允许咳痰功能
   ·    7 适当的时机介入雾化功能
 * ******************************************
 */
/*    Freq = 10	I:E rate = 1:1  */
/**
 * [reset_st_ST_mode_control_data description]
 * @method reset_st_ST_mode_control_data
 */
__weak void reset_st_ST_mode_control_data(void)
{
    st_ST_mode_control_data.blower_increase_speed    =  250;
    st_ST_mode_control_data.rising_time_count        =    0;
    st_ST_mode_control_data.rising_time_steps        =  100;
    st_ST_mode_control_data.T_one_breath_time        = 6000;
    st_ST_mode_control_data.T_target_press           =  150;
    st_ST_mode_control_data.flow_e_gate              = 2000;
    st_ST_mode_control_data.last_time_stemp          = ms_1_count;
    st_ST_mode_control_data.T_expire_start_gate      = ms_1_count + 3000;
#if BREATH_ENABLE_C_FLEX
    st_ST_mode_control_data.c_flex_completed_gate    = ms_1_count + 4500;
#endif
    st_ST_mode_control_data.T_expire_3_4_gate        = ms_1_count + 5250;// = 3000 + 3000 * 3 / 4
    st_ST_mode_control_data.T_next_breath_start_gate = ms_1_count + 6000;
}

// 启动T模式的吸气
//__weak void breath_ST_T_inspire_start(void)
//{
//    display_temp_data.last_work_status = EM_PATIENT_INSPIRE_DETECT;
//    set_patient_status(EM_PATIENT_T_INSPIRE_START);
//    Blower_start_press_control(main_control_data.T_press);
//}

// 检测备份通气时间是否到达
// =1 已经到了备份通气时间
// =0 还未到
/**
 * [is_ST_backup_breath_time description]
 * @method is_ST_backup_breath_time
 * @return                          [description]
 */
__weak int is_ST_backup_breath_time(void)
{
#if USE_SYSTEM_TIMER_COMPARE_FUNCTION
    return system_direct_compare_tm1_be_tm2(ms_1_count, st_ST_mode_control_data.T_next_breath_start_gate);
#else
    if(ms_1_count > st_ST_mode_control_data.T_next_breath_start_gate)
        return EM_TRUE;
    return EM_FALSE;
#endif
}

/**
 * [reset_ST_T_target_val description]
 * @method reset_ST_T_target_val
 */
__weak void reset_ST_T_target_val(void)
{
    uint32_t speed;

    st_ST_mode_control_data.T_target_press    = main_control_data.epap;
    st_ST_mode_control_data.rising_time_count = 0;

    // re-calculate speed
    if(PID_blower_control_data.T_High_speed < PID_blower_control_data.Low_speed)
    {
        //PID_blower_control_data.T_High_speed = PID_blower_control_data.Low_speed + 200;
        speed = 0;
    }
    else
    {
        speed = PID_blower_control_data.T_High_speed - PID_blower_control_data.Low_speed;
    }
    st_ST_mode_control_data.blower_increase_speed = speed / st_ST_mode_control_data.rising_time_steps;
}

/**
 * [force_ST_ipap_press description]
 * @method force_ST_ipap_press
 */
__weak void force_ST_ipap_press(void)
{
#if 0 // zzx 20190327 这部分代码应该没用
    st_ST_mode_control_data.T_target_press    = main_control_data.ipap;
    st_ST_mode_control_data.blower_increase_speed = 0;
    ST_increase_press_data.d_press = 0;
#endif

    PID_blower_control_data.base_speed = PID_blower_control_data.High_speed;
    PID_blower_control_data.base_press = main_control_data.ipap;
    set_PID_blower_adjust_mode(ENUM_PID_SUPPORT_FOR_PRESS);

    st_ST_mode_control_data.rising_time_count = 1;
}

/**
 * [get_ST_T_target_press description]
 * @method get_ST_T_target_press
 * @return                       [description]
 */
__weak int16_t get_ST_T_target_press(void)
{
    return st_ST_mode_control_data.T_target_press;
}

// 用于上升压力调整
/**
 * [inc_ST_T_target_press description]
 * @method inc_ST_T_target_press
 * @param  rising_time_count     [description]
 */
__weak void inc_ST_T_target_press(uint16_t rising_time_count)
{
    st_ST_mode_control_data.T_target_press = main_control_data.epap +
            (int16_t)get_press_increase(&ST_increase_press_data, (int32_t)rising_time_count);
}

/**
 * [reset_ST_T_time_stemp description]
 * @method reset_ST_T_time_stemp
 */
__weak void reset_ST_T_time_stemp(void)
{
    PID_blower_control_data.base_speed         = PID_blower_control_data.Low_speed;
    set_blower_speed(PID_blower_control_data.base_speed);
    st_ST_mode_control_data.T_next_breath_start_gate         = ms_1_count + st_ST_mode_control_data.T_one_breath_time;
    st_ST_mode_control_data.T_expire_start_gate = ms_1_count + main_control_data.T_inspire_time;

    if(st_ST_mode_control_data.T_one_breath_time > main_control_data.T_inspire_time + 500)
    {
        st_ST_mode_control_data.T_expire_3_4_gate = ((st_ST_mode_control_data.T_one_breath_time - main_control_data.T_inspire_time) * 3 / 4) +
                st_ST_mode_control_data.T_expire_start_gate;
    }
    else
    {
        st_ST_mode_control_data.T_expire_3_4_gate = st_ST_mode_control_data.T_next_breath_start_gate - 200;
    }
    //st_ST_mode_control_data.c_flex_completed_gate = st_ST_mode_control_data.T_expire_start_gate + 300;
}

/**
 * [breath_ST_start_T_mode description]
 * @method breath_ST_start_T_mode
 */
__weak void breath_ST_start_T_mode(void)
{
    breath_reset_trigger_flag(); //强调是机控通气
    set_PID_blower_control_status(EM_BREATH_PRESS_INCREASE);
    set_PID_blower_adjust_mode(ENUM_PID_FAST_FOR_PRESS);

    // general a inspire action of a breath
    // 这里有个强制变换工作状态的过程，可能要转换到T模式的某个状态下，
    // 目前还未做处理 --- 未完成！！！暂时当S模式处理
    display_temp_data.last_work_status = EM_PATIENT_INSPIRE_DETECT;
    set_patient_status(EM_PATIENT_T_INSPIRE_START);
}

/* ****************************************************
   字节1：EPAP（0，4-20）
   字节2：IPAP（4-20）
   字节3：FREQ频率（控制模式）
   字节4：Ti吸气时间（控制模式）
   字节5：压力上升时间（0.1-2.0S）（控制模式）
   字节6：压力释放强度C-Flex（0-4）
* ****************************************************/
/**
 * [copy_st_parameter_to_main_control_data description]
 * @method copy_st_parameter_to_main_control_data
 */
__weak void copy_st_parameter_to_main_control_data(void)
{
    main_control_data.epap           = ui_set_data.epap;
    main_control_data.ipap           = ui_set_data.ipap;
    main_control_data.T_freq         = ui_set_data.T_freq;
    main_control_data.T_inspire_time = ui_set_data.T_inspire_time;
    main_control_data.T_rising_time  = ui_set_data.T_rising_time;

    // 暂时认为T_press = ipap
    // main_control_data.T_press        = ui_set_data.ipap;

    // 调整一下C_Flex的设置值，目前暂时认为C-Flex为比epap低的厘米水数值
    if(ui_set_data.c_flex_level < ui_set_data.epap)
    {
        ui_set_data.c_flex_level     = ui_set_data.epap - ui_set_data.c_flex_level;
    }
    else
    {
        ui_set_data.c_flex_level     = 0;// 最小为 0
    }
    main_control_data.c_flex_level   = ui_set_data.c_flex_level;
}

/* ****************************************************
   字节1：EPAP（0，4-25）
   字节2：IPAP（4-40）
   字节3：FREQ频率（控制模式）
   字节4：Ti吸气时间（控制模式）
   字节5：压力上升时间（0.1-2.0S）（控制模式）
   字节6：压力释放强度C-Flex（0-,4）
* ****************************************************/
/**
 * [prepare_ST_control_data description]
 * @method prepare_ST_control_data
 */
__weak void prepare_ST_control_data(void)
{
    uint32_t t1;
    uint16_t speed;
    //int16_t press;
    if(!is_ui_change_parameters())
    {
        reset_display_data(); // 初始化显示数据
    }
    mark_breath_T_stmap();// 吸呼气 时间戳

    // 暂时认为T_press = ipap
    main_control_data.T_press = main_control_data.ipap;

    // Based 必须先计算这个控制速度，然后才能计算后面的
    // IPAP/EPAP for fast PID
    set_PID_blower_control_base_speed(EM_BASE_SPEED_EPAP); // Low_speed
    set_PID_blower_control_base_speed(EM_BASE_SPEED_IPAP); // High_speed
    set_PID_blower_control_base_speed(EM_BASE_SPEED_C_FLEX);// C_Flex_speed

    // 对应T_press压力的转速被保存在T_High_speed中，
    // 因为V60没有独立设置T模式的吸气压力，这里暂时不用,或许需要一个T_High_speed
    set_PID_blower_control_base_speed(EM_BASE_SPEED_BASE);// T_High_speed
#if PEEP_VALVE_CHANGE_ENABLE
    PID_blower_control_data.High_peep = get_peep_value_with_press_and_flow(main_control_data.ipap, main_control_data.flow);
    PID_blower_control_data.Low_peep  = get_peep_value_with_press_and_flow(main_control_data.epap, main_control_data.flow);
#endif
#if USE_BREATH_CYCLES_ADJUST
    breath_cycles_save_peep_data(EM_BREATH_ADJUST_SAVE_I_CONTROL, PID_blower_control_data.High_peep, main_control_data.flow);
    breath_cycles_save_peep_data(EM_BREATH_ADJUST_SAVE_T_CONTROL, PID_blower_control_data.High_peep, main_control_data.flow);
    breath_cycles_save_peep_data(EM_BREATH_ADJUST_SAVE_E_CONTROL, PID_blower_control_data.Low_peep,  main_control_data.flow);
    breath_cycles_save_blower_speed_data(EM_BREATH_ADJUST_SAVE_E_CONTROL, PID_blower_control_data.Low_speed, main_control_data.epap);
    breath_cycles_save_blower_speed_data(EM_BREATH_ADJUST_SAVE_I_CONTROL, PID_blower_control_data.High_speed, main_control_data.ipap);
    breath_cycles_save_blower_speed_data(EM_BREATH_ADJUST_SAVE_T_CONTROL, PID_blower_control_data.T_High_speed, main_control_data.T_press);
#endif
    // 压力上升相关的
    breath_monitor_set_time_t();
    // Press increse base + status
    // 计数，用于计算上升时间
    t1 = main_control_data.T_rising_time;// 1ms
    if(t1 < VAL_PRESS_INCREASE_STEP_TIME * 2)
    {
        t1 = VAL_PRESS_INCREASE_STEP_TIME * 2; // 防止除0，保障上升时间最小100ms
    }
    // 压力上升时的步进（每 VAL_PRESS_INCREASE_STEP_TIME 毫秒调整一次）
    st_ST_mode_control_data.rising_time_steps = t1 / VAL_PRESS_INCREASE_STEP_TIME;
    speed = PID_blower_control_data.T_High_speed - PID_blower_control_data.Low_speed;
    st_ST_mode_control_data.blower_increase_speed = speed / st_ST_mode_control_data.rising_time_steps;

#if BLOWER_ADJUST_PID_PRESS_INCREASE_ENABLE
    // 用于PID（blower）控制中的目标压力   -
    //!!! 这里有问题，关键是压力数值比较小，但调整的步数可能超过压力值，这会导致压力值不变
    ST_increase_press_data.d_press     = main_control_data.ipap - main_control_data.epap;
    ST_increase_press_data.total_steps = st_ST_mode_control_data.rising_time_steps;
#endif
    // reset;
    st_ST_mode_control_data.T_target_press = main_control_data.epap;

    // C-FLEX 控制相关
    // 时间控制相关的
    t1 = 60000 / main_control_data.T_freq; // 计算每一周期的时间，用于启动T模式
    st_ST_mode_control_data.T_one_breath_time = t1;
    // 由呼吸频率决定，确定T触发时间
    st_ST_mode_control_data.T_next_breath_start_gate         = ms_1_count + t1;
    st_ST_mode_control_data.T_expire_start_gate = ms_1_count + main_control_data.T_inspire_time;

    if(t1 > main_control_data.T_inspire_time + 500)
    {
        st_ST_mode_control_data.T_expire_3_4_gate = ((t1 - main_control_data.T_inspire_time) * 3 / 4) +
                st_ST_mode_control_data.T_expire_start_gate;
    }
    else
    {
        st_ST_mode_control_data.T_expire_3_4_gate = st_ST_mode_control_data.T_next_breath_start_gate - 200;
    }
#if BREATH_ENABLE_C_FLEX
    st_ST_mode_control_data.c_flex_completed_gate = st_ST_mode_control_data.T_expire_start_gate + (st_ST_mode_control_data.T_one_breath_time - main_control_data.T_inspire_time) / 2;
#endif
    PID_blower_control_data.base_speed           = PID_blower_control_data.Low_speed;
    set_blower_speed(PID_blower_control_data.base_speed);

    // 吸气相--相关的 触发 门限/阈值
}

/**
 * [is_ST_breath_expire_start_time description]
 * @method is_ST_breath_expire_start_time
 * @return                                [description]
 */
__weak uint8_t is_ST_breath_expire_start_time(void)
{
#if USE_SYSTEM_TIMER_COMPARE_FUNCTION
    return system_direct_compare_tm1_be_tm2(ms_1_count, st_ST_mode_control_data.T_expire_start_gate);
#else
    return st_ST_mode_control_data.T_expire_start_gate < ms_1_count;// TDB
#endif
}

#if 0
__weak int8_t is_ST_T_inspire_time_finished(void)
{
#if USE_SYSTEM_TIMER_COMPARE_FUNCTION
    return system_direct_compare_tm1_be_tm2(ms_1_count, st_ST_mode_control_data.T_expire_start_gate);
#else
    return (st_ST_mode_control_data.T_expire_start_gate <= ms_1_count);// TDB
#endif
}
#endif

/**
 * [to_breath_st_mode description]
 * @method to_breath_st_mode
 */
__weak void to_breath_st_mode(void)
{
    ui_set_data.change_parameter_flag = 0;

    // 如果当前工作模式就是ST模式
    if(global_patient_breath_mode == EM_VENTLATOR_BIPAP_ST_MODE)
    {
        // 把控制数据拷贝过来
        copy_st_parameter_to_main_control_data();

        // 计算下一呼吸周期的控制数据
        prepare_ST_control_data();

        // 必要的初始化信息
        to_default_breath_status();

        return;
    }

    oxygen_start(); // 启动氧浓度调节

    // 把控制数据拷贝过来
    copy_st_parameter_to_main_control_data();

    // 计算下一呼吸周期的控制数据
    prepare_ST_control_data();

    global_patient_breath_mode = EM_VENTLATOR_BIPAP_ST_MODE;

    // 必要的初始化信息
    to_default_breath_status();
}

/**
 * [check_breath_st_parameter_change description]
 * @method check_breath_st_parameter_change
 */
__weak void check_breath_st_parameter_change(void)
{
    if(ui_set_data.change_parameter_flag)
    {
        // 把控制数据拷贝过来
        copy_st_parameter_to_main_control_data();
        // 重新计算一呼吸周期的控制数据
        prepare_ST_control_data();
        ui_set_data.change_parameter_flag = 0;
        main_control_data.change_parameter_flag = 0;
    }
}

/**
 * [ST_get_expire_end_time description]
 * @method ST_get_expire_end_time
 * @return                        [description]
 */
__weak uint16_t ST_get_expire_end_time(void)
{
    return st_ST_mode_control_data.T_expire_3_4_gate;
}

/**
 * [ST_set_flow_e_gate description]
 * @method ST_set_flow_e_gate
 * @param  limited_flow       [description]
 */
__weak void ST_set_flow_e_gate(int limited_flow)
{
    // set flow_gate
    st_ST_mode_control_data.flow_e_gate = display_temp_data.inspire_peak_flow >> 2;
    if(st_ST_mode_control_data.flow_e_gate < limited_flow)
        st_ST_mode_control_data.flow_e_gate = limited_flow;
}

/**
 * [breath_ST_adjust_speed description]
 * @method breath_ST_adjust_speed
 */
__weak void breath_ST_adjust_speed(void)
{
    uint32_t speed;
    if(PID_blower_control_data.T_High_speed < PID_blower_control_data.Low_speed)
    {
        speed = 0;
    }
    else
    {
        speed = PID_blower_control_data.T_High_speed - PID_blower_control_data.Low_speed;
    }
    st_ST_mode_control_data.blower_increase_speed = speed / st_ST_mode_control_data.rising_time_steps;

}

/**
 * [breath_st_mode description]
 * @method breath_st_mode
 */
__weak void breath_st_mode(void)
{
    switch(get_patient_status())
    {
    case EM_PATIENT_NOT_WORK:    // standby
        break;
    case EM_PATIENT_BREATH_DETECT: // 初始呼吸检测
        set_breath_rap_stop();
        rap_run_flag = 0;

        // 检查UI设置是否有变化
        if(check_mode_change_flag())
            return;
        check_breath_st_parameter_change();
        // 检测吸气
        if(check_breath_inspire_start())
        {
            display_temp_data.last_work_status = EM_PATIENT_BREATH_DETECT;
            set_patient_status(EM_PATIENT_INSPIRE_START);
            breath_set_trigger_flag();
            break;
        }
        if(is_ST_backup_breath_time())
        {
            breath_ST_start_T_mode();
        }
        break;
    case EM_PATIENT_INSPIRE_DETECT: // 吸气检测
#if USE_BREATH_CYCLES_ADJUST
        //这里设置EPAP
        if(breath_adjust_flag == 0)
        {
            breath_cycle_adjust_set_epap();
            breath_adjust_flag = 1;
        }
#endif
        set_breath_rap_stop();
        rap_run_flag = 0;

        // 如果有主动吸气的动作，则认为吸气开始
        // ****** 如果长时间（30s）检测不到吸气或呼气，则视为呼吸停止，需要强制到EM_PATIENT_BREATH_DETECT状态中
        if(check_breath_inspire_start())//	即患者出现了主动吸气的动作
        {
            // Mark this is a trigger breath（患者有自主呼吸的过程）
            breath_set_trigger_flag();
            display_temp_data.last_work_status = EM_PATIENT_INSPIRE_DETECT;
            set_patient_status(EM_PATIENT_INSPIRE_START);
            break;
        }
        if(is_ST_backup_breath_time()) // 吸气时间到
        {
            breath_ST_start_T_mode();
        }
        break;

    case EM_PATIENT_BREATH_HOLD: // 屏气
        // 暂时不做此功能，直接跳出
        set_patient_status(EM_PATIENT_EXPIRE_DETECT);
        break;

    case EM_PATIENT_EXPIRE_DETECT: // 呼气检测
        set_breath_rap_stop();
        if(check_breath_expire_start())
        {
#if USE_BREATH_CYCLES_ADJUST
            // 这里设置IPAP
            if(breath_adjust_flag == 0)
            {
                breath_cycle_adjust_set_ipap();
                breath_adjust_flag = 1;
            }
#else
            //恢复到压力调整模式
            Base_set_speed_for_ipap();
            breath_ST_adjust_speed();
#endif
            // PEEP release mode （C-FLEX）
            Blower_start_flow_compensate_for_expire();
            set_patient_status(EM_PATIENT_EXPIRE_START);
            break;
        }
        // 如果时间到，则转到呼气相
        if(is_ST_breath_expire_start_time())
        {
#if USE_BREATH_CYCLES_ADJUST
            // 这里设置IPAP
            if(breath_adjust_flag == 0)
            {
                breath_cycle_adjust_set_ipap();
                breath_adjust_flag = 1;
            }
#else
            //恢复到压力调整模式
            Base_set_speed_for_ipap();
            breath_ST_adjust_speed();
#endif
            set_patient_status(EM_PATIENT_EXPIRE_START);
        }
        break;
    case EM_PATIENT_EXPIRE_START: // 启动呼气
        rap_run_flag = 0;
#if USE_BREATH_CYCLES_ADJUST
        breath_adjust_flag = 0;
#endif
        // 进入快速呼气补偿模式
        S_T_set_speed_cflex();
#if PEEP_VALVE_CHANGE_ENABLE
        // TBD：这里是否要重新设置一下PEEP阀？
        open_peep();
        peep_delay_actived_flag = 0;
#else
        breath_set_peep_e_direct();
#endif
#if BREATH_ENABLE_C_FLEX
        // C-flex 启动
        st_ST_mode_control_data.c_flex_completed_gate = ms_1_count + 300;
#endif

        breath_force_operation_nebulizer(EM_OFF);

        // 吸气转呼气时，需要处理的一些事件
        set_patient_status(EM_PATIENT_EXPIRE_PERIOD);

        break;
    case EM_PATIENT_EXPIRE_PERIOD: // 呼气的过程
#if PEEP_VALVE_CHANGE_ENABLE
        if(!peep_delay_actived_flag)
        {
            peep_delay_actived_flag = breath_waiting_peep_e_ready();
            break;
        }
#endif

#if BREATH_ENABLE_C_FLEX
        if(peep_delay_actived_flag == 1)
        {
#if USE_SYSTEM_TIMER_COMPARE_FUNCTION
            if(system_direct_compare_tm1_be_tm2(ms_1_count, st_ST_mode_control_data.c_flex_completed_gate))
#else
            if(ms_1_count >= st_ST_mode_control_data.c_flex_completed_gate)
#endif
            {
                Base_set_speed_for_epap();
                peep_delay_actived_flag = 2;
            }
            break;
        }
#endif

        // 达到吸气时间时，停止叩击及呼气
        if(is_ST_backup_breath_time())
        {
            set_breath_rap_stop();
            rap_run_flag = 0;

            breath_ST_start_T_mode();
            break;
        }

        if(is_rap_enable() && rap_run_flag == 0)
        {
            // 如果延时不到，则返回
            if(breath_get_time_interval() < 150)
                break;

            // 如果没启动则启动
            if(!is_rap_actived())
            {
                set_breath_rap_start();
                break;
            }
            if(!is_rap_time_over(EM_RAP_EXPIRE_METHOD))
            {
                // 如果叩击时间不够，则维持
                break;
            }
            // 否则停止叩击，等待进入下一模式
            set_breath_rap_stop();
            rap_run_flag = 1;
            // 计算呼气潮气量
            breath_count_Te();
        }

        // 要检测呼气状态， 在monitor里
        // 一旦满足可检查触发的条件，
        // 则恢复到EPAP控制模式
        break;

    case EM_PATIENT_INSPIRE_START: // 启动吸气
        rap_run_flag = 0;
#if USE_BREATH_CYCLES_ADJUST
        // 这里进行调整
        if(display_count_data.total_breath_cycles > 1)
        {
            breath_cycle_adjust_set_epap();
            if(display_count_data.total_breath_cycles > 10)
            {
                breath_cycles_set_direct(display_count_data.P_peep, display_count_data.P_peak);
            }
            breath_cycles_adjust_work();
        }
#endif
        breath_ST_adjust_speed();
        breath_adjust_flag = 0;
        // 采用与T模式相同的操作
        set_breath_rap_stop();
        if(check_mode_change_flag())
            return;
        check_breath_st_parameter_change();
        // 如果这里的Ptarget参数和IPAP不相同，这里需要重新计算控制参数，（或者使用独立的一套数据）
        breath_force_operation_nebulizer(EM_ON);

        // 重置各种时间标签
        reset_ST_T_time_stemp();

        // for spontaneous needs
        force_ST_ipap_press(); // 直接进入IPAP

        st_ST_mode_control_data.last_time_stemp = ms_1_count;

#if PEEP_VALVE_CHANGE_ENABLE
        //注意：这几行代码未经测试！！！！ 目前为防止PEEP的滞环效应，这里是先完全关闭PEEP，再重新设置， 在吸气时可能也需要先把PEEP设置为0
        close_peep();
        peep_delay_actived_flag = 0;
#else
        breath_set_peep_i_direct();
#endif
        set_patient_status(EM_PATIENT_INSPIRE_PERIOD);
        break; //case EM_PATIENT_INSPIRE_START

    // 机械通气过程
    case EM_PATIENT_T_INSPIRE_START: // 启动机控的吸气
        set_breath_rap_stop();

        // 检查UI设置是否有变化
        if(check_mode_change_flag())
            return;
        check_breath_st_parameter_change();

#if USE_BREATH_CYCLES_ADJUST
        // 这里进行调整
        if(display_count_data.total_breath_cycles > 1)
        {
            breath_cycle_adjust_set_epap();
            if(display_count_data.total_breath_cycles > 10)
            {
                breath_cycles_set_direct(display_count_data.P_peep, display_count_data.P_peak);
            }
            breath_cycles_adjust_work();
        }
#endif

        // reset speed increase steps
        breath_ST_adjust_speed();
        breath_adjust_flag = 0;

        // 重置各种时间标签
        reset_ST_T_time_stemp();
        rap_run_flag = 0;

        // open nebulizer at breath start
        breath_force_operation_nebulizer(EM_ON);

        // 这里的Ptarget参数和IPAP相同，暂时使用一样的
#if BLOWER_ADJUST_PID_PRESS_INCREASE_ENABLE
        reset_ST_T_target_val();
        inc_ST_T_target_press(st_ST_mode_control_data.rising_time_count);
#endif

#if BLOWER_ADJUST_DIRECT_PRESS_INCREASE_ENABLE
        S_T_set_inc_speed(st_ST_mode_control_data.rising_time_count, st_ST_mode_control_data.blower_increase_speed);
        clear_pid_adjust_val(EM_BASE_SPEED_BASE);
#endif
        (st_ST_mode_control_data.rising_time_count)++;
        st_ST_mode_control_data.last_time_stemp = ms_1_count;

        // 这里要重新设置一下PEEP阀
#if PEEP_VALVE_CHANGE_ENABLE
        //注意：这几行代码未经测试！！！！ 目前为防止PEEP的滞环效应，这里是先完全关闭PEEP，再重新设置， 在吸气时可能也需要先把PEEP设置为0
        close_peep();
        //将PEEP设置到IPAP位置
        peep_delay_actived_flag = 0;
#else
        breath_set_peep_i_direct();
#endif

        set_patient_status(EM_PATIENT_T_INSPIRE_PERIOD);
        break; // case EM_PATIENT_T_INSPIRE_START:

    case EM_PATIENT_INSPIRE_PERIOD: // 吸气过程
#if PEEP_VALVE_CHANGE_ENABLE
        // peep
        if(!peep_delay_actived_flag)
        {
            peep_delay_actived_flag = breath_waiting_peep_i_ready();
        }
#endif

#if 0
        if(st_ST_mode_control_data.rising_time_count == st_ST_mode_control_data.rising_time_steps)
        {
            (st_ST_mode_control_data.rising_time_count)++;
#if USE_BREATH_CYCLES_ADJUST

#else
            //恢复到压力调整模式
            Base_set_speed_for_ipap();
            breath_ST_adjust_speed();
#endif
            // 重新标记 计时
            st_ST_mode_control_data.last_time_stemp = ms_1_count;
        }
        // 如果压力上升调整次数超过计算值，则进入下一模式
        if(st_ST_mode_control_data.rising_time_count > st_ST_mode_control_data.rising_time_steps)
        {
#if USE_SYSTEM_TIMER_COMPARE_FUNCTION
            if(!system_direct_compare_tm1_be_tm2(ms_1_count, st_ST_mode_control_data.last_time_stemp + VAL_PRESS_INCREASE_STEP_TIME))
#else
            if(ms_1_count - st_ST_mode_control_data.last_time_stemp < VAL_PRESS_INCREASE_STEP_TIME)
#endif
            {
                break; // wait a moment
            }
            if(is_rap_enable() && (rap_run_flag == 0))
            {
                // 如果没启动则启动
                if(!is_rap_actived())
                {
                    set_breath_rap_start();
                    break;
                }

                if(!is_rap_time_over(EM_RAP_INSPIRE_METHOD))
                {
                    // 如果叩击时间不够，则维持
                    break;
                }

                // 否则进入下一模式
                set_breath_rap_stop();
                rap_run_flag = 1;
            }

            // force!! change work status
            set_patient_status(EM_PATIENT_EXPIRE_DETECT);
#if USE_BREATH_CYCLES_ADJUST
            // 这里进行IPAP设置
            breath_cycle_adjust_set_ipap();
#endif

            // 恢复压力上升计数器
            st_ST_mode_control_data.rising_time_count = 0;
            break;
        }

        // 采用与T模式相同的操作
        // 依照压力上升时间调整涡轮
        // 此为压力上升的过程  // TDB
#if USE_SYSTEM_TIMER_COMPARE_FUNCTION
        if(system_direct_compare_tm1_be_tm2(ms_1_count, st_ST_mode_control_data.last_time_stemp + VAL_PRESS_INCREASE_STEP_TIME))
#else
        if(ms_1_count - st_ST_mode_control_data.last_time_stemp > VAL_PRESS_INCREASE_STEP_TIME)
#endif
        {
            (st_ST_mode_control_data.rising_time_count)++;
            // 如果超过50ms 则进行下一次调整
#if BLOWER_ADJUST_PID_PRESS_INCREASE_ENABLE
            //inc_ST_T_target_press(st_ST_mode_control_data.rising_time_count);
#endif
#if BLOWER_ADJUST_DIRECT_PRESS_INCREASE_ENABLE
            S_T_set_inc_speed(st_ST_mode_control_data.rising_time_count, st_ST_mode_control_data.blower_increase_speed);
            clear_pid_adjust_val(EM_BASE_SPEED_BASE);
#endif

            // 重新标记 计时
            st_ST_mode_control_data.last_time_stemp = ms_1_count;
        }
#else
        // 如果满足吸气时间，则转到呼气状态
        // 为防止吸气时间过长，需要此处理
        if(is_ST_breath_expire_start_time())
        {
#if USE_BREATH_CYCLES_ADJUST
            // 这里进行T-press 设置，如果有叩击，暂不进行调整
            if(!is_rap_actived())
            {
                breath_cycle_adjust_set_ipap();
            }
#else
            //恢复到压力调整模式
            Base_set_speed_for_ipap();
            breath_ST_adjust_speed();
#endif
            set_patient_status(EM_PATIENT_EXPIRE_START);
            break;
        }
#if USE_SYSTEM_TIMER_COMPARE_FUNCTION
        if(system_direct_compare_tm1_be_tm2(ms_1_count, st_ST_mode_control_data.last_time_stemp + VAL_PRESS_INCREASE_STEP_TIME))
#else
        if(ms_1_count - st_ST_mode_control_data.last_time_stemp > VAL_PRESS_INCREASE_STEP_TIME)
#endif
        {
            (st_ST_mode_control_data.rising_time_count)++;
        }
        // 如果压力达到ipap 则进入 叩击/呼气检测(EM_PATIENT_EXPIRE_DETECT)
        if(get_current_press() > main_control_data.ipap - 30)
        {
            rap_run_flag = 1; // 允许叩击
        }

        if(rap_run_flag == 1)
        {
            if(is_rap_enable() && rap_run_flag)
            {
                // 有叩击则进行叩击....

                // 如果没启动则启动
                if(!is_rap_actived())
                {
                    set_breath_rap_start();
                    breath_count_Ti();
                    break;
                }

                if(!is_rap_time_over(EM_RAP_INSPIRE_METHOD))
                {
                    // 如果叩击时间不够，则维持
                    break;
                }
            }
        }

        if(st_ST_mode_control_data.rising_time_count >= st_ST_mode_control_data.rising_time_steps)
            // 如果 时间超过压力上升时间，且流量小于2升
        {
            // 关闭此标志
            rap_run_flag = 0;
            // 进入下一模式
            set_breath_rap_stop();
            // set flow_gate
            ST_set_flow_e_gate(2000);
            set_patient_status(EM_PATIENT_EXPIRE_DETECT);
            break;
        }

        // 如果压力已经满足要求，则进入下一状态
        if(get_current_press() >= main_control_data.ipap - 10)
            // 如果 时间超过压力上升时间，且流量小于2升
        {
            // 关闭此标志
            rap_run_flag = 0;
            // 进入下一模式
            set_breath_rap_stop();
            // set flow_gate
            ST_set_flow_e_gate(2000);
            set_patient_status(EM_PATIENT_EXPIRE_DETECT);
            break;
        }
#endif
        break; //case EM_PATIENT_INSPIRE_PERIOD:

    case EM_PATIENT_T_INSPIRE_PERIOD:// T模式吸气过程（机控吸气）
#if PEEP_VALVE_CHANGE_ENABLE
        // waiting PEEP valve adjust ready
        if(!peep_delay_actived_flag)
        {
            peep_delay_actived_flag = breath_waiting_peep_i_ready();
        }
#endif
        // 如果满足条件，则直接进入呼气状态
#if USE_SYSTEM_TIMER_COMPARE_FUNCTION
        if(system_direct_compare_tm1_be_tm2(ms_1_count, st_ST_mode_control_data.T_expire_start_gate))
#else
        if(ms_1_count >= st_ST_mode_control_data.T_expire_start_gate)
#endif
        {
#if USE_BREATH_CYCLES_ADJUST

#else
            //恢复到压力调整模式
            Base_set_speed_T_press();
            breath_ST_adjust_speed();
#endif
            // 停止rap
            set_breath_rap_stop();

            // 关闭雾化器
            breath_force_operation_nebulizer(EM_OFF);
            // 这里进行T-press 设置
            breath_cycle_adjust_set_T_press();

            // force!! change work status
            set_patient_status(EM_PATIENT_T_EXPIRE_START);

            // 恢复压力上升计数器
            st_ST_mode_control_data.rising_time_count = 0;
            break;
        }

        if(st_ST_mode_control_data.rising_time_count == st_ST_mode_control_data.rising_time_steps)
        {
            (st_ST_mode_control_data.rising_time_count)++;
            //恢复到压力调整模式
            // 重新标记 计时
            st_ST_mode_control_data.last_time_stemp = ms_1_count;
            st_ST_mode_control_data.T_target_press = main_control_data.T_press;
            break;
        }

        if(st_ST_mode_control_data.rising_time_count > st_ST_mode_control_data.rising_time_steps)
        {
#if USE_SYSTEM_TIMER_COMPARE_FUNCTION
            if(!system_direct_compare_tm1_be_tm2(ms_1_count, st_ST_mode_control_data.last_time_stemp + VAL_PRESS_INCREASE_STEP_TIME))
#else
            if(ms_1_count - st_ST_mode_control_data.last_time_stemp <= VAL_PRESS_INCREASE_STEP_TIME)
#endif
            {
                break;
            }
            // 如果压力达到ipap 则进入 叩击/呼气检测(EM_PATIENT_EXPIRE_DETECT)
            if(get_current_press() > main_control_data.ipap - 30)
            {
                rap_run_flag = 1; // 允许叩击
            }
            if(rap_run_flag == 1)
            {
                if(is_rap_enable() && rap_run_flag)
                {
                    // 有叩击则进行叩击....
                    // 如果没启动则启动
                    if(!is_rap_actived())
                    {
                        set_breath_rap_start();
                        breath_count_Ti();
                        break;
                    }
                    if(!is_rap_time_over(EM_RAP_INSPIRE_METHOD))
                    {
                        // 如果叩击时间不够，则维持
                        break;
                    }
                }
                set_breath_rap_stop();
                st_ST_mode_control_data.last_time_stemp = ms_1_count;
            }
#if 0
            if(is_rap_enable())
            {
                if(rap_run_flag == 0)
                {
                    // 每呼吸周期只能进入一次
                    rap_run_flag = 1;
                    // 如果没启动则启动
                    if(!is_rap_actived())
                    {
                        set_breath_rap_start();
                        break;
                    }
                }
                if(!is_rap_time_over(EM_RAP_INSPIRE_METHOD))
                {
                    // 如果叩击时间不够，则维持
                    break;
                }

                set_breath_rap_stop();
                st_ST_mode_control_data.last_time_stemp = ms_1_count;
                // 否则进入下一模式
            }
            // 为防止误触发，叩击完成后，这里额外需要做个延时
#endif
            // TBD：这里是否要重新设置一下PEEP阀？
#if USE_BREATH_CYCLES_ADJUST
            // 这里进行T-press 设置，如果有叩击，暂不进行调整
            //Base_set_speed_for_ipap();
            PID_blower_control_data.base_press = main_control_data.T_press;
            set_PID_blower_adjust_mode(ENUM_PID_SUPPORT_FOR_PRESS);
#else
            //恢复到压力调整模式
            Base_set_speed_for_ipap();
            breath_ST_adjust_speed();
#endif
            if(rap_run_flag == 1)
            {
#if USE_SYSTEM_TIMER_COMPARE_FUNCTION
                if(!system_direct_compare_tm1_be_tm2(ms_1_count, st_ST_mode_control_data.last_time_stemp + 150))
#else
                if(ms_1_count - st_ST_mode_control_data.last_time_stemp >= 150)
#endif
                    break;
            }
            else
            {
#if USE_SYSTEM_TIMER_COMPARE_FUNCTION
                if(!system_direct_compare_tm1_be_tm2(ms_1_count, st_ST_mode_control_data.last_time_stemp + 100))
#else
                if(ms_1_count - st_ST_mode_control_data.last_time_stemp >= 100)
#endif

                    break;
            }
            // force!! change work status
            set_patient_status(EM_PATIENT_T_EXPIRE_DETECT);

            // 恢复压力上升计数器
            st_ST_mode_control_data.rising_time_count = 0;
            break; //case EM_PATIENT_T_INSPIRE_PERIOD:
        }

        // 此为压力上升的过程  // TDB
#if USE_SYSTEM_TIMER_COMPARE_FUNCTION
        if(system_direct_compare_tm1_be_tm2(ms_1_count, st_ST_mode_control_data.last_time_stemp + VAL_PRESS_INCREASE_STEP_TIME))
#else
        if(ms_1_count - st_ST_mode_control_data.last_time_stemp > VAL_PRESS_INCREASE_STEP_TIME)
#endif
        {
            // 如果超过50ms 则进行下一次调整
#if BLOWER_ADJUST_PID_PRESS_INCREASE_ENABLE
            inc_ST_T_target_press(st_ST_mode_control_data.rising_time_count);
#endif
#if BLOWER_ADJUST_DIRECT_PRESS_INCREASE_ENABLE
            S_T_set_inc_speed(st_ST_mode_control_data.rising_time_count, st_ST_mode_control_data.blower_increase_speed);
            clear_pid_adjust_val(EM_BASE_SPEED_BASE);
#endif
            (st_ST_mode_control_data.rising_time_count)++;
            // 如果压力上升调整次数超过计算值，则进入下一模式

            // 重新标记 计时
            st_ST_mode_control_data.last_time_stemp = ms_1_count;
        }
        break; // case EM_PATIENT_T_INSPIRE_PERIOD:
    case EM_PATIENT_T_EXPIRE_PERIOD: // T模式呼气过程
#if PEEP_VALVE_CHANGE_ENABLE
        if(!peep_delay_actived_flag)
        {
            peep_delay_actived_flag = breath_waiting_peep_e_ready();

            // zzx:2019-3-8 不需要进行后面的工作
            break;
        }
#endif

#if BREATH_ENABLE_C_FLEX
        // C-flex 结束判定
        // 貌似可以和上面的呼气阀状态控制合并处理
        // 如果C-flex时间到，恢复到EPAP
        if(peep_delay_actived_flag == 1)
        {
#if USE_SYSTEM_TIMER_COMPARE_FUNCTION
            if(system_direct_compare_tm1_be_tm2(ms_1_count, st_ST_mode_control_data.c_flex_completed_gate))
#else
            if(ms_1_count >= st_ST_mode_control_data.c_flex_completed_gate)
#endif
            {
                Base_set_speed_for_epap();
                peep_delay_actived_flag = 2;
            }
            break;
        }
#endif

        if(is_rap_enable() && (rap_run_flag == 0))
        {

            // 如果延时不到，则返回
            if(breath_get_time_interval() < 150)
                break;

            // 如果 达到吸气开始时间，则进入下一呼吸，此处是限制叩击时间
            if(is_ST_backup_breath_time())
            {
#if USE_BREATH_CYCLES_ADJUST
                // 这里处理epap
                breath_cycle_adjust_set_epap();
                breath_adjust_flag = 0;
#endif
                breath_ST_start_T_mode();
                break;
            }


            // 如果没启动则启动
            if(!is_rap_actived())
            {
                set_breath_rap_start();
                break;
            }
            if(!is_rap_time_over(EM_RAP_EXPIRE_METHOD))
            {
                // 如果叩击时间不够，则维持
                break;
            }
            // 否则等待进入下一模式
            else
            {
                // 先关闭叩击
                set_breath_rap_stop();
                rap_run_flag = 1;
                // 计算呼气潮气量
                breath_count_Te();
                break;
            }
        }

        // 在呼气时，需要监测的内容
#if USE_SYSTEM_TIMER_COMPARE_FUNCTION
        if(!system_direct_compare_tm1_be_tm2(ms_1_count, ST_get_expire_end_time()))
#else
        if(ms_1_count < ST_get_expire_end_time())
#endif
        {
            // !!! 是不是错了？
            break;
        }
        else
        {
            display_reset_peep_data();
            //to_default_breath_status();
            breath_continue_with_epap();

            // 计算呼气潮气量
            breath_count_Te();

            set_patient_status(EM_PATIENT_INSPIRE_DETECT);
            breath_reset_inspire_trigger_flag();
            break;
        }

    case EM_PATIENT_T_EXPIRE_DETECT: // T呼气检测
        //
        set_breath_rap_stop();
        rap_run_flag = 0;

        // 如果满足吸气时间，则转到呼气状态
        if(is_ST_breath_expire_start_time())
        {
#if USE_BREATH_CYCLES_ADJUST

#endif
            set_patient_status(EM_PATIENT_T_EXPIRE_START);
        }
        break;
    case EM_PATIENT_T_EXPIRE_START:
        breath_adjust_flag = 0;
#if USE_BREATH_CYCLES_ADJUST
        breath_cycle_adjust_set_T_press();
        PID_blower_control_data.T_High_speed = PID_blower_control_data.High_speed;
#endif
        // 关闭雾化器
        breath_force_operation_nebulizer(EM_OFF);

        // 进入快速呼气补偿模式
        S_T_set_speed_cflex();
#if PEEP_VALVE_CHANGE_ENABLE
        // TBD：这里是否要重新设置一下PEEP阀？
        open_peep();
        //*********************************
        // zzx: maybe need a short delay
        //*********************************
        peep_delay_actived_flag = 0;
#else
        breath_set_peep_e_direct();
#endif
        rap_run_flag = 0;
#if BREATH_ENABLE_C_FLEX
        // C-flex 启动
        st_ST_mode_control_data.c_flex_completed_gate = ms_1_count + 300;
#endif
        set_patient_status(EM_PATIENT_T_EXPIRE_PERIOD);
        break;
    ////////////////////////////////////////////////////
    // 状态结束标记
    case EM_PATIENT_STATUS_ENDS:
        break;
    default:
        break;
    }
}




/* ********************************************************************
 ***************BIPAP-PCV **************
 ********************************************************************
 */
#if 0
/******************************************************************/
#define inc_PCV_T_target_press inc_ST_T_target_press
#define reset_PCV_T_target_press reset_ST_T_target_val
#define reset_PCV_T_time_stemp reset_ST_T_time_stemp
#define PCV_set_speed_epap Base_set_speed_for_epap
/******************************************************************/
#else
// 用于上升压力调整
/**
 * [inc_PCV_T_target_press description]
 * @method inc_PCV_T_target_press
 * @param  rising_time_count      [description]
 */
__weak void inc_PCV_T_target_press(uint16_t rising_time_count)
{
    st_PCV_mode_control_data.T_target_press = main_control_data.epap +
            (int16_t)get_press_increase(&PCV_increase_press_data, (int32_t)rising_time_count);
}

/**
 * [reset_PCV_T_time_stemp description]
 * @method reset_PCV_T_time_stemp
 */
__weak void reset_PCV_T_time_stemp(void)
{
    PID_blower_control_data.base_speed                 = PID_blower_control_data.Low_speed;
    set_blower_speed(PID_blower_control_data.base_speed);
    st_PCV_mode_control_data.T_next_breath_start_gate = ms_1_count + st_PCV_mode_control_data.T_one_breath_time;
    st_PCV_mode_control_data.T_expire_start_gate      = ms_1_count + main_control_data.T_inspire_time;

    if(st_PCV_mode_control_data.T_one_breath_time > main_control_data.T_inspire_time + 500)
    {
        st_PCV_mode_control_data.T_expire_3_4_gate    = ((st_PCV_mode_control_data.T_one_breath_time - main_control_data.T_inspire_time) * 3 / 4) +
                st_PCV_mode_control_data.T_expire_start_gate;
    }
    else
    {
        st_PCV_mode_control_data.T_expire_3_4_gate    = st_PCV_mode_control_data.T_next_breath_start_gate - 200;
    }
}

/**
 * [reset_PCV_T_target_press description]
 * @method reset_PCV_T_target_press
 */
__weak void reset_PCV_T_target_press(void)
{
    st_PCV_mode_control_data.T_target_press    = main_control_data.epap;
    st_PCV_mode_control_data.rising_time_count = 0;
}

/**
 * [PCV_set_speed_T_press description]
 * @method PCV_set_speed_T_press
 */
__weak void PCV_set_speed_T_press(void)
{
    set_PID_blower_control_status(EM_BREATH_PCV_PRESS_HOLD);
    set_PID_blower_adjust_mode(ENUM_PID_FAST_FOR_PRESS);
    PID_blower_control_data.blower_p_support_adjust_val = 0;

    // 调整涡轮
    set_blower_speed(PID_blower_control_data.T_High_speed);
}
// 检测备份通气时间是否到达
// =1 已经到了备份通气时间
// =0 还未到
/**
 * [is_PCV_backup_breath_time description]
 * @method is_PCV_backup_breath_time
 * @return                           [description]
 */
__weak int is_PCV_backup_breath_time(void)
{
#if USE_SYSTEM_TIMER_COMPARE_FUNCTION
    return system_direct_compare_tm1_be_tm2(ms_1_count, st_PCV_mode_control_data.T_next_breath_start_gate);
#else
    if(ms_1_count > st_PCV_mode_control_data.T_next_breath_start_gate)
        return EM_TRUE;
    else
        return EM_FALSE;
#endif
}

#endif
/**
 * [copy_pcv_parameter_to_main_control_data description]
 * @method copy_pcv_parameter_to_main_control_data
 */
__weak void copy_pcv_parameter_to_main_control_data(void)
{
    main_control_data.epap           = ui_set_data.epap;
    main_control_data.ipap           = ui_set_data.ipap;
    main_control_data.T_freq         = ui_set_data.T_freq;
    main_control_data.T_inspire_time = ui_set_data.T_inspire_time;
    main_control_data.T_rising_time  = ui_set_data.T_rising_time;
}

/**
 * [reset_PCV_T_time description]
 * @method reset_PCV_T_time
 */
__weak void reset_PCV_T_time(void)
{
    // 由呼吸频率决定，确定T触发时间
    st_PCV_mode_control_data.T_next_breath_start_gate = ms_1_count + st_PCV_mode_control_data.T_one_breath_time;
    st_PCV_mode_control_data.T_expire_start_gate      = ms_1_count + main_control_data.T_inspire_time;

    if(st_PCV_mode_control_data.T_one_breath_time > main_control_data.T_inspire_time + 500)
    {
        st_PCV_mode_control_data.T_expire_3_4_gate    = ((st_PCV_mode_control_data.T_one_breath_time - main_control_data.T_inspire_time) * 3 / 4) +
                st_PCV_mode_control_data.T_expire_start_gate;
    }
    else
    {
        st_PCV_mode_control_data.T_expire_3_4_gate    = st_PCV_mode_control_data.T_next_breath_start_gate - 200;
    }
}

/**
 * [pcv_get_inspire_end_time description]
 * @method pcv_get_inspire_end_time
 * @return                          [description]
 */
__weak uint16_t pcv_get_inspire_end_time(void)
{
    return (main_control_data.T_inspire_time + main_control_data.T_inspire_time + main_control_data.T_inspire_time) >> 2;
}

/**
 * [pcv_get_expire_end_time description]
 * @method pcv_get_expire_end_time
 * @return                         [description]
 */
__weak uint16_t pcv_get_expire_end_time(void)
{
    return st_PCV_mode_control_data.T_expire_3_4_gate;
}

/**
 * [get_PCV_T_target_press description]
 * @method get_PCV_T_target_press
 * @return                        [description]
 */
__weak int16_t get_PCV_T_target_press(void)
{
    return st_PCV_mode_control_data.T_target_press;
}

/**
 * [prepare_PCV_control_data description]
 * @method prepare_PCV_control_data
 */
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

/**
 * [breath_PCV_start_inspire description]
 * @method breath_PCV_start_inspire
 */
__weak void breath_PCV_start_inspire(void)
{
    // 需要关闭叩击
    set_breath_rap_stop();

    //reset_PCV_T_target_press(); // 重置目标压力

    set_PID_blower_control_status(EM_BREATH_PCV_PRESS_INC);
    set_PID_blower_adjust_mode(ENUM_PID_FAST_FOR_PRESS);

    // general a inspire action of a breath
    // 这里有个强制变换工作状态的过程，可能要转换到T模式的某个状态下，
    // 目前还未做处理 --- 未完成！！！暂时当S模式处理
    display_temp_data.last_work_status = EM_PATIENT_INSPIRE_DETECT;
    set_patient_status(EM_PATIENT_T_INSPIRE_START);
}

/**
 * [check_breath_PCV_parameter_change description]
 * @method check_breath_PCV_parameter_change
 */
__weak void check_breath_PCV_parameter_change(void)
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
 * [breath_pcv_adjust_speed description]
 * @method breath_pcv_adjust_speed
 */
__weak void breath_pcv_adjust_speed(void)
{
    uint32_t speed;
    if(PID_blower_control_data.T_High_speed < PID_blower_control_data.Low_speed)
    {
        speed = 0;
    }
    else
    {
        speed = PID_blower_control_data.T_High_speed - PID_blower_control_data.Low_speed;
    }
    st_PCV_mode_control_data.blower_increase_speed = speed / st_PCV_mode_control_data.rising_time_steps;
}

//__weak void PCV_set_flow_e_gate(int limited_flow)
//{
//	// set flow_gate
//	st_PCV_mode_control_data.flow_e_gate = display_temp_data.inspire_peak_flow >> 2;
//	if(st_PCV_mode_control_data.flow_e_gate < limited_flow) st_PCV_mode_control_data.flow_e_gate = limited_flow;
//}
/**
 * [breath_pcv_mode description]
 * @method breath_pcv_mode
 */
__weak void breath_pcv_mode(void)
{

    switch(get_patient_status())
    {
    case EM_PATIENT_PIPE_OUT:     // 管路脱落
        break;

    case EM_PATIENT_NOT_WORK:     // standby
        break;

    case EM_PATIENT_EXPIRE_START: // 呼气开始
#if USE_BREATH_CYCLES_ADJUST
        // 这里处理T-high
        breath_cycle_adjust_set_ipap(); // 没有用
        breath_cycle_adjust_set_T_press();
        breath_adjust_flag = 0;
#endif
        // 关闭雾化器
        breath_force_operation_nebulizer(EM_OFF);

        //恢复到压力调整模式
        Base_set_speed_for_epap();
#if PEEP_VALVE_CHANGE_ENABLE
        // TBD：这里是否要重新设置一下PEEP阀？
        open_peep();
        //*********************************
        // zzx: maybe need a short delay
        //      if()break;
        //*********************************
        peep_delay_actived_flag = 0;
#else
        breath_set_peep_e_direct();
#endif
        rap_run_flag = 0;

        set_patient_status(EM_PATIENT_T_EXPIRE_PERIOD);
        break;

    // 机械通气过程
    case EM_PATIENT_T_INSPIRE_START: // T吸气开始
        // 控制压力上升的过程时，不能有叩击
        set_breath_rap_stop();
        // 检查UI设置是否有变化
        if(check_mode_change_flag())
            return;
        check_breath_PCV_parameter_change();
#if USE_BREATH_CYCLES_ADJUST
        //这里进行调整
        if(display_count_data.total_breath_cycles > 1)
        {
            breath_cycle_adjust_set_epap();
            if(display_count_data.total_breath_cycles > 10)
            {
                breath_cycles_set_direct(display_count_data.P_peep, display_count_data.P_peak);
            }
            breath_cycles_adjust_work();
        }
        breath_pcv_adjust_speed();
#endif

        // 重置PCV控制数据
        reset_PCV_T_target_press();

        // 重置各种时间标签
        reset_PCV_T_time_stemp();

        breath_force_operation_nebulizer(EM_ON);

        // 这里的Ptarget参数和IPAP相同，暂时使用一样的
#if BLOWER_ADJUST_PID_PRESS_INCREASE_ENABLE
        inc_PCV_T_target_press(st_PCV_mode_control_data.rising_time_count);
#endif
#if BLOWER_ADJUST_DIRECT_PRESS_INCREASE_ENABLE
        PCV_set_inc_speed(st_PCV_mode_control_data.rising_time_count, st_PCV_mode_control_data.blower_increase_speed);
        clear_pid_adjust_val(EM_BASE_SPEED_BASE);
#endif
        (st_PCV_mode_control_data.rising_time_count)++;
        st_PCV_mode_control_data.last_time_stemp = ms_1_count;

        // 需要验证：这里要重新设置一下PEEP阀
#if PEEP_VALVE_CHANGE_ENABLE
        //注意：这几行代码未经测试！！！！ 目前为防止PEEP的滞环效应，这里是先完全关闭PEEP，再重新设置， 在吸气时可能也需要先把PEEP设置为0
        close_peep();
        peep_delay_actived_flag = 0;
#else
        breath_set_peep_i_direct();
#endif
        rap_run_flag = 0;

        set_PID_blower_control_status(EM_BREATH_PCV_PRESS_INC);

        set_patient_status(EM_PATIENT_T_INSPIRE_PERIOD);
        break; // case EM_PATIENT_T_INSPIRE_START:

    case EM_PATIENT_T_EXPIRE_PERIOD: // T呼气过程
#if PEEP_VALVE_CHANGE_ENABLE
        if(!peep_delay_actived_flag)
        {
            peep_delay_actived_flag = breath_waiting_peep_e_ready();

            // zzx:2019-3-8 不需要进行后面的工作
            break;
        }
#endif
        // 如果 达到吸气开始时间，则进入下一呼吸
        if(is_PCV_backup_breath_time())
        {
#if USE_BREATH_CYCLES_ADJUST
            // 这里处理epap
            breath_cycle_adjust_set_epap();
            breath_adjust_flag = 0;
#endif
            breath_PCV_start_inspire();
            break;
        }

        if(is_rap_enable() && (rap_run_flag == 0))
        {
            // 如果延时不到，则返回
            if(breath_get_time_interval() < 150)
                break;

            // 如果没启动则启动
            if(!is_rap_actived())
            {
                set_breath_rap_start();
                break;
            }
            if(!is_rap_time_over(EM_RAP_EXPIRE_METHOD))
            {
                // 如果叩击时间不够，则维持
                break;
            }
            // 否则等待进入下一模式
            else
            {
                // 先关闭叩击
                set_breath_rap_stop();
                rap_run_flag = 1;
                // 计算呼气潮气量
                breath_count_Te();
                break;
            }
        }

        // 在呼气时，需要监测的内容
#if USE_SYSTEM_TIMER_COMPARE_FUNCTION
        if(system_direct_compare_tm1_be_tm2(ms_1_count, pcv_get_expire_end_time()))
#else
        if(ms_1_count > pcv_get_expire_end_time())
#endif
        {
            break;
        }
        else
        {
            display_reset_peep_data();
            to_default_breath_status();
            set_patient_status(EM_PATIENT_T_INSPIRE_DETECT);

            // 计算呼气潮气量
            breath_count_Te();

            breath_reset_inspire_trigger_flag();
            break;
        }

    case EM_PATIENT_T_INSPIRE_PERIOD: // T模式吸气过程（机控吸气）
#if PEEP_VALVE_CHANGE_ENABLE

        if(!peep_delay_actived_flag)
        {
            if(basic_control_parameters.invasive_support)
            {
                peep_delay_actived_flag = 1;
                break;
            }
            peep_delay_actived_flag = breath_waiting_peep_i_ready();
            break; //这个过程暂时不用调整涡轮参数
        }
#endif
        // 如果满足条件，则直接进入呼气状态
#if USE_SYSTEM_TIMER_COMPARE_FUNCTION
        if(system_direct_compare_tm1_be_tm2(ms_1_count, st_PCV_mode_control_data.T_expire_start_gate))
#else
        if(ms_1_count >= st_PCV_mode_control_data.T_expire_start_gate)
#endif
        {
#if USE_BREATH_CYCLES_ADJUST
#else
            //恢复到压力调整模式
            Base_set_speed_T_press();
            breath_pcv_adjust_speed();
#endif
            // 停止rap
            set_breath_rap_stop();

            // 关闭雾化器
            breath_force_operation_nebulizer(EM_OFF);

            // force!! change work status
            set_patient_status(EM_PATIENT_EXPIRE_START);

            // 恢复压力上升计数器
            st_PCV_mode_control_data.rising_time_count = 0;
            break;
        }

        //恢复到压力调整模式
        if(st_PCV_mode_control_data.rising_time_count == st_PCV_mode_control_data.rising_time_steps)
        {
#if USE_SYSTEM_TIMER_COMPARE_FUNCTION
            if(system_direct_compare_tm1_be_tm2(ms_1_count, st_PCV_mode_control_data.last_time_stemp + VAL_PRESS_INCREASE_STEP_TIME))
#else
            if(ms_1_count - st_PCV_mode_control_data.last_time_stemp >= VAL_PRESS_INCREASE_STEP_TIME)
#endif
            {
                // 修正计数
                (st_PCV_mode_control_data.rising_time_count)++;
                st_PCV_mode_control_data.last_time_stemp = ms_1_count;
            }
            break;
        }
        // 如果上升过程结束
        if(st_PCV_mode_control_data.rising_time_count > st_PCV_mode_control_data.rising_time_steps)
        {
#if USE_SYSTEM_TIMER_COMPARE_FUNCTION
            if(!system_direct_compare_tm1_be_tm2(ms_1_count, st_PCV_mode_control_data.last_time_stemp + VAL_PRESS_INCREASE_STEP_TIME))
#else
            if(ms_1_count - st_PCV_mode_control_data.last_time_stemp <= VAL_PRESS_INCREASE_STEP_TIME)
#endif
            {
                break;
            }
            // 修正计数
            (st_PCV_mode_control_data.rising_time_count)++;
            st_PCV_mode_control_data.last_time_stemp = ms_1_count;
            // 这里开始处理叩击功能
            if(is_rap_enable() && (rap_run_flag == 0))
            {
                // 如果没启动则启动
                if(!is_rap_actived())
                {
                    set_breath_rap_start();
                    breath_count_Ti();
                    break;
                }
                if(!is_rap_time_over(EM_RAP_INSPIRE_METHOD))
                {
                    // 如果叩击时间不够，则维持
                    break;
                }
                // 否则停止叩击，等待进入下一模式
                //else {}
                set_breath_rap_stop();
                rap_run_flag = 1;
                break;
            }
#if USE_BREATH_CYCLES_ADJUST
            // 这里进行T-press 设置，如果有叩击，暂不进行调整
            //Base_set_speed_for_ipap();
            PID_blower_control_data.base_press = main_control_data.T_press;
            set_PID_blower_adjust_mode(ENUM_PID_SUPPORT_FOR_PRESS);
#else
            //恢复到压力调整模式
            Base_set_speed_for_ipap();
            breath_pcv_adjust_speed();
#endif
            // force!! change work status
            set_patient_status(EM_PATIENT_T_EXPIRE_DETECT);

            // 恢复压力上升计数器
            st_PCV_mode_control_data.rising_time_count = 0;

            //else {}
            break; //case EM_PATIENT_T_INSPIRE_PERIOD:
        }

        // ***********************************
        // 此为压力上升的过程	// TDB
        // ***********************************
#if USE_SYSTEM_TIMER_COMPARE_FUNCTION
        if(system_direct_compare_tm1_be_tm2(ms_1_count, st_PCV_mode_control_data.last_time_stemp + VAL_PRESS_INCREASE_STEP_TIME))
#else
        if(ms_1_count - st_PCV_mode_control_data.last_time_stemp > VAL_PRESS_INCREASE_STEP_TIME)
#endif
        {
            // 修正计数
            (st_PCV_mode_control_data.rising_time_count)++;
            // 如果超过50ms 则进行下一次调整
#if BLOWER_ADJUST_DIRECT_PRESS_INCREASE_ENABLE
            PCV_set_inc_speed(st_PCV_mode_control_data.rising_time_count, st_PCV_mode_control_data.blower_increase_speed);
            clear_pid_adjust_val(EM_BASE_SPEED_BASE);
#endif
#if BLOWER_ADJUST_PID_PRESS_INCREASE_ENABLE
            inc_PCV_T_target_press(st_PCV_mode_control_data.rising_time_count);
            clear_pid_adjust_val(EM_BASE_SPEED_BASE);
#endif
            // 如果压力上升调整次数超过计算值，则进入下一模式

            // 重新标记 计时
            st_PCV_mode_control_data.last_time_stemp = ms_1_count;
        }
        break; // case EM_PATIENT_T_INSPIRE_PERIOD:
    case EM_PATIENT_T_EXPIRE_DETECT: // T呼气检测
        // 如果满足吸气时间，则转到呼气状态
        if(is_ST_breath_expire_start_time())
        {
            set_patient_status(EM_PATIENT_EXPIRE_START);
            break;
        }
        // 如果检测到主动呼气的动作，也要触发吸气动作? 暂时不做
        break;


    case EM_PATIENT_T_INSPIRE_DETECT: // T吸气检测
        // 在这里处理
#if USE_BREATH_CYCLES_ADJUST
        // 这里处理EPAP
        if(!breath_adjust_flag)
        {
            breath_adjust_flag = 1;
            breath_cycle_adjust_set_epap();
        }
#endif
        // 关闭叩击
        if(is_rap_actived())
        {
            set_breath_rap_stop();
            break;
        }
        if(!basic_control_parameters.invasive_support)
        {
            if(check_breath_inspire_start()) //	即患者出现了主动吸气的动作
            {
#if USE_BREATH_CYCLES_ADJUST
                // 这里处理Epap
                breath_cycle_adjust_set_epap();
                breath_adjust_flag = 0;
#endif
                breath_PCV_start_inspire();

                // Mark this is a trigger breath（患者有自主呼吸的过程）
                breath_set_trigger_flag();
                break;
            }
        }
        // 如果 T时间到，则开始吸气
        if(is_ST_backup_breath_time())
        {
#if USE_BREATH_CYCLES_ADJUST
            // 这里处理Epap
            breath_cycle_adjust_set_epap();
            breath_adjust_flag = 0;
#endif
            breath_PCV_start_inspire();

            breath_reset_trigger_flag(); //强调是机控通气
        }

        // 如果检测到患者吸气的动作，则开始吸气（这个在monitor里 实现）
        break;
    default:
        break;
    }
}

/**
 * [to_breath_pcv_mode description]
 * @method to_breath_pcv_mode
 */
__weak void to_breath_pcv_mode(void)
{
    ui_set_data.change_parameter_flag = 0;

    // 如果当前工作模式就是PCV模式
    if(global_patient_breath_mode != EM_VENTLATOR_BIPAP_PCV_MODE)
    {
        global_patient_breath_mode = EM_VENTLATOR_BIPAP_PCV_MODE;
        oxygen_start();
    }

    // 把控制数据拷贝过来
    copy_pcv_parameter_to_main_control_data();

    // 计算下一呼吸周期的控制数据
    prepare_PCV_control_data();

    set_patient_status(EM_PATIENT_T_INSPIRE_START);
}

#if 1
/* ***************PAV **************
 */
/**
 * [breath_pav_mode description]
 * @method breath_pav_mode
 */
__weak void breath_pav_mode(void)
{

}

/**
 * [to_breath_pav_mode description]
 * @method to_breath_pav_mode
 */
__weak void to_breath_pav_mode(void)
{
    oxygen_start();
}

/* ***************BiLevel **************
 */
/**
 * [breath_bilevel_mode description]
 * @method breath_bilevel_mode
 */
__weak void breath_bilevel_mode(void)
{

}

/**
 * [to_breath_bilevel_mode description]
 * @method to_breath_bilevel_mode
 */
__weak void to_breath_bilevel_mode(void)
{
    oxygen_start();
}

/* ***************SPONTANEOUS_MODE **************
 */
/**
 * [breath_spontaneous_mode description]
 * @method breath_spontaneous_mode
 */
__weak void breath_spontaneous_mode(void)
{

}

/**
 * [to_breath_spontaneous_mode description]
 * @method to_breath_spontaneous_mode
 */
__weak void to_breath_spontaneous_mode(void)
{
    oxygen_start();
}

/* ***************AC-P **************
 */
/**
 * [breath_ac_p_mode description]
 * @method breath_ac_p_mode
 */
__weak void breath_ac_p_mode(void)
{

}

/**
 * [to_breath_ac_p_mode description]
 * @method to_breath_ac_p_mode
 */
__weak void to_breath_ac_p_mode(void)
{
    oxygen_start();
}

/* ***************AC-V **************
 */
/**
 * [breath_ac_v_mode description]
 * @method breath_ac_v_mode
 */
__weak void breath_ac_v_mode(void)
{

}

/**
 * [to_breath_ac_v_mode description]
 * @method to_breath_ac_v_mode
 */
__weak void to_breath_ac_v_mode(void)
{
    oxygen_start();
}

/* ***************SIMV-PCV **************
 */
/**
 * [breath_simv_pcv_mode description]
 * @method breath_simv_pcv_mode
 */
__weak void breath_simv_pcv_mode(void)
{

}

/**
 * [to_breath_simv_pcv_mode description]
 * @method to_breath_simv_pcv_mode
 */
__weak void to_breath_simv_pcv_mode(void)
{
    oxygen_start();
}

/* ***************SIMV-VCV **************
 */
/**
 * [breath_simv_vcv_mode description]
 * @method breath_simv_vcv_mode
 */
__weak void breath_simv_vcv_mode(void)
{

}

/**
 * [to_breath_simv_vcv_mode description]
 * @method to_breath_simv_vcv_mode
 */
__weak void to_breath_simv_vcv_mode(void)
{
    oxygen_start();
}
#endif
/* *************** cough **********************/

// 为咳痰控制设置一个带有滞后参数的时间标签，主要用于压力上升控制
/**
 * [set_cough_time_stemp description]
 * @method set_cough_time_stemp
 * @param  val                  [description]
 */
__weak void set_cough_time_stemp(uint32_t val)
{
    // 时间标记
    cough_control_data.last_time_stemp = ms_1_count + val;
}

/**
 * [get_Cough_target_press description]
 * @method get_Cough_target_press
 * @return                        [description]
 */
__weak int16_t get_Cough_target_press(void)
{
    return cough_control_data.cough_target_press;
}

// 咳痰数据初始化 // 应该给一个默认工作环境,防止意外进入咳痰模式时的错误操作
/**
 * [reset_cough_control_data description]
 * @method reset_cough_control_data
 */
__weak void reset_cough_control_data(void)
{
    cough_control_data.auto_cough_time_start_gate             = 0;
    cough_control_data.blower_increase_speed       = 0;
    cough_control_data.cough_finish_gate_flow_e    = 0;
    cough_control_data.cough_finish_gate_time_e    = 0;
    cough_control_data.cough_finish_gate_vt        = 0;
    cough_control_data.cough_finish_gate_vt_cent   = 0;
    cough_control_data.cough_target_press          = 0;
    cough_control_data.last_time_stemp             = ms_1_count;
    cough_control_data.prev_work_mode              = EM_VENTLATOR_STANDBY_MODE;
    cough_control_data.prev_work_status            = 0;
    cough_control_data.rising_time_count           = 0;
    cough_control_data.rising_time_steps           = 0;
    cough_control_data.cough_display_data_ready    = EM_INACTIVED;

    // 负压排痰操作状态 =1：表示正在进行负压排痰， = 0表示没有进行负压排痰
    cough_control_data.vacuum_pressure_work_status = 0;
}

// 继续执行原来的工作模式所对应的操作
/**
 * [cough_run_pre_work_mode description]
 * @method cough_run_pre_work_mode
 */
__weak void cough_run_pre_work_mode(void)
{
    // 恢复原来的工作状态
    set_patient_status(cough_control_data.prev_work_status);
    set_patient_breath_mode(cough_control_data.prev_work_mode);

    // 运行原来的工作
    // 数据计算
    calculate_breath_data();

    // 状态监测
    patient_status_monitor();

    // 紧急状态处理
    // 模式控制
    breath_mode_control();

    if(ui_set_data.change_mode_flag == 1)
    {
        ui_set_data.change_mode_flag = 0;

        // 不需要再恢复当前的状态，继续原来的状态就好了
        return;
    }


    // 在恢复前，需要保存之前的状态 (和工作模式)
    cough_control_data.prev_work_status = get_patient_status();
    cough_control_data.prev_work_mode   = get_patient_breath_mode();


    // 恢复到当前的模式
    set_patient_status(EM_PATIENT_COUGH_PREPARE);
    set_patient_breath_mode(EM_VENTLATOR_COUGH_MODE);
}

// 默认返回原来模式的 EM_PATIENT_BREATH_DETECT 状态
/**
 * [cough_back_to_prev_work description]
 * @method cough_back_to_prev_work
 */
__weak void cough_back_to_prev_work(void)
{
    // 恢复因咳痰而变化的阀
    // 雾化器
    if(is_breath_nebulizer_turned_on())
    {
        breath_force_operation_nebulizer(EM_ON);
    }
    // 关排痰阀
    COUGH_VALVE_CLOSE();
    // 打开吸气阀
    INSPIRE_VALVE_OPEN();

    // 恢复原来的工作状态
    set_patient_breath_mode(cough_control_data.prev_work_mode);
    // 打开呼气阀
    to_default_breath_status();
}

// 关闭所有跟咳痰相关的操作
// 这个程序完成之后，必须再次发送(START COUGH)指令才能触发咳痰; (REPEAT COUGH)指令无效
/**
 * [stop_cough_blower_work description]
 * @method stop_cough_blower_work
 */
__weak void stop_cough_blower_work(void)
{
    //vacuum_pressure_blower_stop();
    //set_vacuum_blower_work_status(EM_OFF);
}

// 把UI设置参数拷贝到主控参数表中
/**
 * [copy_COUGH_parameter_to_main_control_data description]
 * @method copy_COUGH_parameter_to_main_control_data
 */
__weak void copy_COUGH_parameter_to_main_control_data(void)
{
    main_control_data.cough_finish_val          = ui_set_data.cough_finish_val;
    main_control_data.cough_finish_types        = ui_set_data.cough_finish_types;
    main_control_data.cough_hold_time           = ui_set_data.cough_hold_time;
    main_control_data.cough_prepare_press       = ui_set_data.cough_prepare_press;
    main_control_data.cough_press_increase_time = ui_set_data.cough_press_increase_time;
    main_control_data.cough_start_press         = ui_set_data.cough_start_press;
    main_control_data.cough_start_vt            = ui_set_data.cough_start_vt;
}

// 检测是否超过了时间戳（带延时参数）
/**
 * [is_cough_out_of_time description]
 * @method is_cough_out_of_time
 * @param  target_time          [description]
 * @return                      [description]
 */
__weak uint8_t is_cough_out_of_time(uint32_t target_time)
{
#if USE_SYSTEM_TIMER_COMPARE_FUNCTION
    return system_direct_compare_tm1_be_tm2(ms_1_count, cough_control_data.last_time_stemp + target_time);
#else
    return ms_1_count - cough_control_data.last_time_stemp >= target_time;// TDB
#endif
}

// 咳痰起始时的准备工作，主要是解析压力上升相关的控制等
/**
 * [prepare_cough_control_data description]
 * @method prepare_cough_control_data
 */
__weak void prepare_cough_control_data(void)
{
    uint32_t t1;
    uint16_t speed;
    //int16_t press;
    //reset_display_data(); // 初始化显示数据
    // 显示数据处理函数
    reset_display_data_for_cough();

    // 压力上升相关的
    // Press increse base + status
    // 计数，用于计算上升时间
    t1 = main_control_data.cough_press_increase_time;// 1ms
    if(t1 < VAL_PRESS_INCREASE_STEP_TIME * 2)
        t1 = VAL_PRESS_INCREASE_STEP_TIME * 2; // 防止除0，保障最小上升时间  100ms

    // 压力上升时的步进（每VAL_PRESS_INCREASE_STEP_TIME（毫秒）调整一次）
    cough_control_data.rising_time_steps = t1 / VAL_PRESS_INCREASE_STEP_TIME;

    // 设置最大吸气截止时间
    cough_control_data.auto_cough_time_start_gate = ms_1_count + main_control_data.cough_hold_time
            + main_control_data.cough_press_increase_time;
    // 涡轮起始转速
    speed = get_blower_current_set_speed();
    set_base_speed(speed); // 设置初始速度
    //set_blower_speed(speed);// 调整涡轮速度
    speed = get_blower_speed_for_press_and_flow(main_control_data.cough_prepare_press, 0) - speed;
    cough_control_data.blower_increase_speed = speed / cough_control_data.rising_time_steps;


#if BLOWER_ADJUST_PID_PRESS_INCREASE_ENABLE  // 只有允许PID控制时才可以
    // 用于PID（blower）控制中的目标压力
    // reset;
    cough_control_data.cough_target_press = get_current_press();

    cough_increase_press_data.total_steps = cough_control_data.rising_time_steps;
    cough_increase_press_data.d_press = main_control_data.cough_prepare_press - main_control_data.epap; //cough_control_data.cough_target_press;
#endif
    // 目标潮气量
    if(main_control_data.cough_start_vt < 300)
        main_control_data.cough_start_vt = 300;

    // 时间标记
    set_cough_time_stemp(VAL_PRESS_INCREASE_STEP_TIME);

    // 设置peep阀
    close_peep();
}

// 当负压吸引工作准备好时
/**
 * [breath_cough_start_work description]
 * @method breath_cough_start_work
 */
__weak void breath_cough_start_work(void)
{
    // 设置涡轮当前工作的状态（PID调整），快速压力调整应该由回流阀解决!!!!2018-11-19
    set_PID_blower_control_status(EM_BREATH_COUGH_SUPPORT);// 调整数据存储位置
#if BLOWER_ADJUST_PID_PRESS_INCREASE_ENABLE
    // PID 压力调整模式
    set_PID_blower_adjust_mode(ENUM_PID_FAST_FOR_PRESS);
#else
    // 关闭 blower pid调整模式,
    set_PID_blower_adjust_mode(ENUM_PID_NO_ADJUST);//(ENUM_PID_FAST_FOR_PRESS);
#endif
    // 状态 // 设置涡轮初始转速
    prepare_cough_control_data();
}

// 当UI设置重复咳痰时，使用这个指令，不需要重新设置参数，关键是不需要等待负压风机准备负压
/**
 * [UI_reapeat_cough_com description]
 * @method UI_reapeat_cough_com
 */
__weak void UI_reapeat_cough_com(void)
{
    // 如果上次咳痰还没完成，禁止重复
    if(global_patient_breath_mode == EM_VENTLATOR_COUGH_MODE)
    {
        return;
    }

    // 如果负压风机没有打开，禁止重复
    if(!is_vacuum_blower_work())//if(!base_board_status.status.cough_system_ready)
    {
        return;
    }
    reset_cough_control_data();

    set_cough_time_stemp(0);

    // 备份相关数据
    cough_control_data.prev_work_status = get_patient_status();
    cough_control_data.prev_work_mode   = get_patient_breath_mode();

    // 模式标记
    global_patient_breath_mode = EM_VENTLATOR_COUGH_MODE;     // 变更通气模式
    // 初始状态
    set_patient_status(EM_PATIENT_COUGH_PREPARE);               // 咳痰预备状态
}

// 主要是检测负压风机的工作状况以及患者呼吸状态是否能允许启动咳痰
/**
 * [is_cough_system_ready description]
 * @method is_cough_system_ready
 * @return                       [description]
 */
__weak uint8_t is_cough_system_ready(void)
{
    // 如果真空泵没有准备好 返回 0 (至少要少于目标压力 2厘米水的差距)
    if(get_vacuum_pump_pressure() < main_control_data.cough_start_press - 20)
    {
        return EM_FALSE;
    }


    // 如果通气状态不正确（处于呼气状态） 返回 0
    switch(cough_control_data.prev_work_status)
    {
    case EM_PATIENT_NOT_WORK://standby
    case EM_PATIENT_BREATH_DETECT:
    case EM_PATIENT_INSPIRE_DETECT:
    case EM_PATIENT_INSPIRE_START:
    case EM_PATIENT_INSPIRE_PERIOD:
    case EM_PATIENT_T_INSPIRE_START:
    case EM_PATIENT_T_INSPIRE_PERIOD:
        return EM_TRUE;

    default:
        return EM_FALSE;
    }
}

//（由患者吸气潮气量及设置的剩余潮气量占吸气潮气量的百分比确定检测条件）
/**
 * [count_cough_gate_vt_cent description]
 * @method count_cough_gate_vt_cent
 * @return                          [description]
 */
__weak uint16_t count_cough_gate_vt_cent(void)
{
    uint32_t vt_val;
    vt_val = (uint32_t)(100 - main_control_data.cough_finish_val);
    vt_val *= (uint32_t)breath_get_remaining_vt();
    vt_val /= 100;
    return (uint16_t) vt_val;
}

// 本次咳痰用于检测结束条件的相关数据
// 由用户设置的参数及患者吸气数据，计算用于咳痰结束监测条件
/**
 * [cough_prepare_next_finished_check_data description]
 * @method cough_prepare_next_finished_check_data
 */
__weak void cough_prepare_next_finished_check_data(void)
{
    switch(main_control_data.cough_finish_types)
    {
    case EM_COUGH_FINISH_WITH_VT_VAL: // = 0	潮气量阈值(绝对值)			0-150		   x10ml
        cough_control_data.cough_finish_gate_vt = main_control_data.cough_finish_val * 10;
        if(cough_control_data.cough_finish_gate_vt < 150)
        {
            cough_control_data.cough_finish_gate_vt = 150;
        }
        break;
    case EM_COUGH_FINISH_WITH_VT_PERCENT: // = 1	潮气量百分比				50-100		   %
        cough_control_data.cough_finish_gate_vt_cent = count_cough_gate_vt_cent();
        if(cough_control_data.cough_finish_gate_vt_cent < 150)
        {
            cough_control_data.cough_finish_gate_vt_cent = 150;
        }
        break;
    case EM_COUGH_FINISH_WITH_FLOW_LIMITED:// = 2	呼气流量阈值				0-100		   L/min
        cough_control_data.cough_finish_gate_flow_e = (int32_t)main_control_data.cough_finish_val * 1000;
        break;
    case EM_COUGH_FINISH_WITH_TIME_LIMITED:// = 3	时间阈值				1-20		   0.1秒
        cough_control_data.cough_finish_gate_time_e = (uint32_t)main_control_data.cough_finish_val * 100 + ms_1_count;
        break;
    case EM_COUGH_FINISH_WITH_OTHERS:
        break;
    default:
        break;
    }
}

// 执行一次咳痰动作，主要是关吸气阀和打开呼气阀，为了准确计算潮气量，这里确定了咳痰开始时刻
/**
 * [Perform_a_cough_action description]
 * @method Perform_a_cough_action
 */
__weak void Perform_a_cough_action(void)
{
    // 要关闭叩击功能才可以
    if(is_rap_actived())
    {
        set_breath_rap_stop();
    }

    // 要关闭雾化装置
    if(is_breath_nebulizer_turned_on())
    {
        breath_force_operation_nebulizer(EM_OFF);
    }
    // 关闭吸气阀
    INSPIRE_VALVE_CLOSE();

    // 这里需要打开blower 的PID调整
    // 设置自动咳痰的时间间隔
    // 时间标记
    // 时间标记
    set_cough_time_stemp(0);

    // 显示数据
    display_start_cough_count();

    cough_prepare_next_finished_check_data();// 用于is_cough_finished()

    // 执行咳痰动作
    // 负压排痰操作状态 =1：表示正在进行负压排痰， = 0表示没有进行负压排痰
    cough_control_data.vacuum_pressure_work_status = 1;
    COUGH_VALVE_OPEN();// 咳痰的动作
    set_patient_status(EM_PATIENT_COUGH_PERIOD);// 进入咳嗽状态
}

// 用于PID 跟踪压力，每次调整当前时刻所需要的压力目标
/**
 * [Cough_inc_target_press description]
 * @method Cough_inc_target_press
 * @param  rising_time_count      [description]
 */
__weak void Cough_inc_target_press(uint16_t rising_time_count)
{

    cough_control_data.cough_target_press = main_control_data.epap +
                                            (int16_t)get_press_increase(&cough_increase_press_data, (int32_t)rising_time_count);
}

// 检测咳痰结束标志
/**
 * [is_cough_finished description]
 * @method is_cough_finished
 * @return                   [description]
 */
__weak uint8_t is_cough_finished(void)
{
    switch(main_control_data.cough_finish_types)
    {
    case EM_COUGH_FINISH_WITH_VT_VAL:   // = 0,  潮气量阈值(绝对值)			0-150		   x10ml
        if(breath_get_remaining_vt() < cough_control_data.cough_finish_gate_vt) return EM_TRUE;
        break;
    case EM_COUGH_FINISH_WITH_VT_PERCENT: // = 1,  潮气量百分比				50-100		   %
        if(breath_get_remaining_vt() < cough_control_data.cough_finish_gate_vt_cent) return EM_TRUE;
        break;
    case EM_COUGH_FINISH_WITH_FLOW_LIMITED: // = 2,  呼气流量阈值				0-100		   L/min
        // 应该是呼气流量小于门限时 返回1（此时排痰结束）
        if(get_expire_flow() < cough_control_data.cough_finish_gate_flow_e) return EM_TRUE;
        break;
    case EM_COUGH_FINISH_WITH_TIME_LIMITED: // = 3,  时间阈值				1-20		   0.1秒
#if USE_SYSTEM_TIMER_COMPARE_FUNCTION
        if(system_direct_compare_tm1_be_tm2(ms_1_count, cough_control_data.cough_finish_gate_time_e))
            return EM_TRUE;
#else
        if(ms_1_count > cough_control_data.cough_finish_gate_time_e)
            return EM_TRUE; // TDB
#endif
        break;
    case EM_COUGH_FINISH_WITH_OTHERS:
    default:
        break;
    }
    // 如果 呼气口流量小于2L/min
    if(get_expire_flow() < 2000)
        return EM_TRUE;
    if(display_count_data.volume < 100)
        return EM_TRUE;
    // 或者
    return EM_FALSE;
}

// 如果满足咳痰检测条件则准备咳痰
/**
 * [is_cough_detect_start_signal description]
 * @method is_cough_detect_start_signal
 * @return                              [description]
 */
__weak uint8_t is_cough_detect_start_signal(void)
{
    // !!!!(这里考虑患者的主动咳痰问题) 如果吸气流速小于阈值（0.5L/min），则考虑停止
    // 这时，其他条件都不重要了，因为即使不结束，时间增加也不会再有条件满足了
    if(get_ins_flow() - get_flow_leak() < 500)
    {
        // 注意： 这个过程会延长吸气时间--占用一部分屏气时间
        return EM_TRUE;
    }

    // 如果患者潮气量没有满足要求(300ml) 返回 0
    if(display_count_data.volume < main_control_data.cough_start_vt)
    {
        return EM_FALSE;
    }

    // 如果口端压力没有满足要求(< main_control_data.cough_prepare_press) 返回 0
    if(get_current_press() < main_control_data.cough_prepare_press)
    {
        return EM_FALSE;
    }

    // 条件都满足
    return EM_TRUE;
}

// 检测是否需要强制启动排痰
/**
 * [is_auto_cough_detect description]
 * @method is_auto_cough_detect
 * @return                      [description]
 */
__weak uint8_t is_auto_cough_detect(void)
{
    //（二选一）：A 患者有咳痰动作 (压力超过控制压力的10 cmH2O) && (                      口端流量小于0)
    if((get_current_press() > main_control_data.cough_prepare_press + 100)) // || (display_count_data.Flow < -1000)) // 待推敲
    {
        return EM_TRUE;
    }
    //B 到达自动咳痰时限
#if USE_SYSTEM_TIMER_COMPARE_FUNCTION
    return system_direct_compare_tm1_be_tm2(ms_1_count, cough_control_data.auto_cough_time_start_gate);
#else
    if(cough_control_data.auto_cough_time_start_gate < ms_1_count)// TDB

    {
        return EM_TRUE;
    }
    return EM_FALSE;
#endif
}

// 咳痰模式的入口函数
/**
 * [breath_cough_mode description]
 * @method breath_cough_mode
 */
__weak void breath_cough_mode(void)
{
    switch(get_patient_status())
    {
#if 0
    default:

        break;
    case EM_PATIENT_NOT_WORK:   // 待机 standby
        break;
    case EM_PATIENT_BREATH_DETECT: // 专门用于测试的状态
        break;
    case EM_PATIENT_INSPIRE_DETECT: // 监测吸气触发条件
        break;
    case EM_PATIENT_INSPIRE_START: // 吸气开始的时刻
        break;
    case EM_PATIENT_INSPIRE_PERIOD: // 吸气的过程中
        break;
    case EM_PATIENT_EXPIRE_DETECT: // 监测呼气触发条件
        break;
    case EM_PATIENT_BREATH_HOLD: // 屏气过程（仅容控过程可能使用）
        break;
    case EM_PATIENT_EXPIRE_START: // 吸气转呼气的时刻
        break;
    case EM_PATIENT_EXPIRE_PERIOD: // 呼气过程
        break;

    // 状态结束标记
    case EM_PATIENT_STATUS_ENDS:
        break;
#else
    // 返回之前的模式
    default:
        //stop_cough_blower_work();
        //cough_back_to_prev_work();
        break;

#endif

    case EM_PATIENT_PIPE_OUT:         // 管路处于脱落状态
        // 返回之前的模式
        // 咳痰会导致管路脱落错误检测信号!!!!! 需要再次验证
        //stop_cough_blower_work();
        cough_back_to_prev_work();
        break;

    // 咳痰过程
    case EM_PATIENT_COUGH_PREPARE:        // 咳痰准备状态
        if(is_cough_system_ready())
        {
            // 启动排痰，转到下一工作状态
            breath_cough_start_work();
            set_patient_status(EM_PATIENT_COUGH_PRESSURE_INCREASE);
            break;
        }

        // 如果长时间(10s)不能达到要求，需要强制回到原来的模式
        if(is_cough_out_of_time(10000))
        {
            stop_cough_blower_work();
            cough_back_to_prev_work();
            // 貌似需要传送超时信息
            break;
        }
        // 执行之前的模式
        cough_run_pre_work_mode(); // 这里有迭代，应该避免重复调用！！！
        break;

    case EM_PATIENT_COUGH_PRESSURE_INCREASE: // 压力上升状态

        // 压力上升完成阶段的工作
        if(cough_control_data.rising_time_count >= cough_control_data.rising_time_steps)
        {
            // 如果满足咳痰检测条件则准备咳痰
            if(is_auto_cough_detect())
            {

                // 执行咳痰动作
                Perform_a_cough_action();
                break;
            }
            // 在潮气量达到要求，初始压力达到要求，
            if(is_cough_detect_start_signal())
            {
                // 设置工作状态
                set_patient_status(EM_PATIENT_COUGH_DETECTING_START);// 进入咳嗽检测阶段
                break;
            }
            else
            {
                // 要考虑持续无法进入下一状态的情况
                // 当时间到达阈值，如果条件满足最低咳痰要求，则强制咳痰
#if USE_SYSTEM_TIMER_COMPARE_FUNCTION
                if(system_direct_compare_tm1_be_tm2(ms_1_count, cough_control_data.auto_cough_time_start_gate))
#else
                if(ms_1_count >= cough_control_data.auto_cough_time_start_gate)// TDB
#endif
                {
                    // 执行咳痰动作
                    Perform_a_cough_action();
                    break;
                }
                // 否则维持当前状态
                else
                {
                    break;
                }
            }
        }
        else
        {
            // 此为压力上升的过程
#if USE_SYSTEM_TIMER_COMPARE_FUNCTION
            if(system_direct_compare_tm1_be_tm2(ms_1_count, cough_control_data.last_time_stemp))
#else
            if(ms_1_count > cough_control_data.last_time_stemp)// TDB
#endif
            {
                // 压力上升阶段
#if BLOWER_ADJUST_PID_PRESS_INCREASE_ENABLE
                Cough_inc_target_press(cough_control_data.rising_time_count);
#endif
                Cough_increase_pid_base_speed(cough_control_data.rising_time_count, cough_control_data.blower_increase_speed);
                cough_control_data.rising_time_count++;
                // 时间标记
                set_cough_time_stemp(50);// !!!!!如果这里直接加上50，上面就不用做减法了
            }
            break;
        }
    //break; // 或许不需要这一行代码
    case EM_PATIENT_COUGH_DETECTING_START: // 监测自主咳痰
        // 此时检测咳痰条件
        // 如果满足开始排痰的条件
        if(is_auto_cough_detect())
        {
            // 执行咳痰动作
            Perform_a_cough_action();
        }
        break;
    case EM_PATIENT_COUGH_PERIOD:         // 咳痰
        // 要有一个保护期，保证排痰动作完成 （约200ms）需要最后调整
        if(!is_cough_out_of_time(150))
            break;
        // 此时要检测咳痰结束标记
        // 如果满足呼气结束，则：处理必要的控制，进入下一状态

        // 状态1 ：需要检测满足负压吸引的条件
        if(cough_control_data.vacuum_pressure_work_status)
        {
            if(is_cough_finished())
            {
                display_reset_peep_data();// 开始计算PEEP
                // 负压排痰操作状态 =1：表示正在进行负压排痰， = 0表示没有进行负压排痰
                cough_control_data.vacuum_pressure_work_status = 0;
                cough_control_data.cough_display_data_ready    = EM_ACTIVED;
                calculate_cough_cycle_display_data();

                // 提前恢复因咳痰而变化的阀，这样会把系统因为延时造成的问题改正过来
                // 关排痰阀
                COUGH_VALVE_CLOSE();
                // 打开吸气阀
                INSPIRE_VALVE_OPEN();

                set_cough_time_stemp(200);

                set_patient_status(EM_PATIENT_COUGH_COMPLETE_AND_REPEAT);
            }
        }
        else // 真空泵未工作!!!!,停止排痰动作
        {
            stop_cough_blower_work();
            cough_back_to_prev_work();
        }
        // 状态2 呼气阀工作到位 延时 100ms
        // 关闭排痰阀

        // 状态3：需要检测呼气结束条件
        // 否则持续等待
        break;
    case EM_PATIENT_COUGH_COMPLETE_AND_REPEAT:// 结束
#if USE_SYSTEM_TIMER_COMPARE_FUNCTION  ///????
        if(system_direct_compare_tm1_be_tm2(ms_1_count, cough_control_data.last_time_stemp))
            break;
#else
        if(ms_1_count > cough_control_data.last_time_stemp)
            break; //// TDB
#endif
        // 回到之前的模式， 并等待下一个重复指令
        cough_back_to_prev_work();
        break;

    }
}

///////////////////////////////////////////
// 用户在启动咳痰时，首先要做环境准备
///////////////////////////////////////////
// 这里应该考虑如何快速启动真空泵
/**
 * [to_breath_cough_mode description]
 * @method to_breath_cough_mode
 */
__weak void to_breath_cough_mode(void)
{
    //禁止进入
    if(global_patient_breath_mode == EM_VENTLATOR_COUGH_MODE)
    {
        return;
    }

    // 首先要处理咳痰的参数
    copy_COUGH_parameter_to_main_control_data();
    reset_cough_control_data();

    set_cough_time_stemp(0);

    // 模式标记
    // 需要保存之前的状态 (和工作模式)
    cough_control_data.prev_work_status = get_patient_status();
    cough_control_data.prev_work_mode   = get_patient_breath_mode();

    global_patient_breath_mode = EM_VENTLATOR_COUGH_MODE;     // 变更通气模式
    // 初始状态
    set_patient_status(EM_PATIENT_COUGH_PREPARE);               // 咳痰预备状态
}

// 强制启动真空（负压）风机---只用于咳痰模式，
// 其压力由咳痰控制参数ui_set_data.cough_start_press 确定
/**
 * [force_start_vacuum_pump description]
 * @method force_start_vacuum_pump
 */
__weak void force_start_vacuum_pump(void)
{
    //if(cough_control_data.cough_system_actived_flag) return;
    if(is_vacuum_blower_work())
        return; //if(base_board_status.status.cough_system_ready) return;
    // 启动真空泵
    start_cough_blower_work();
}

void reset_sensor_data_active_flag(void);

/////////////////////////////////////////////////////////////////
// 检查是否有模式变化
/////////////////////////////////////////////////////////////////
/**
 * [check_mode_change_flag description]
 * @method check_mode_change_flag
 * @return                        [description]
 */
__weak uint8_t check_mode_change_flag(void)
{
    if(!ui_set_data.change_mode_flag)
        return 0;
    ui_set_data.change_mode_flag      = 0;


    reset_sensor_data_active_flag();
    // 貌似应该清除参数设置的标记
    ui_set_data.change_parameter_flag = 0;

    //set_patient_breath_mode(ui_set_data.work_mode);
    switch(ui_set_data.work_mode)
    {
    case EM_VENTLATOR_STANDBY_MODE:
        to_breath_standby_mode();
        break;

    //基本模式
    case EM_VENTLATOR_CPAP_MODE:
        to_breath_cpap_mode();
        break;

    // 高级模式
    case EM_VENTLATOR_PAV_MODE:
        break;
    case EM_VENTLATOR_COUGH_REPEAT_MODE:
        if(global_patient_breath_mode == EM_VENTLATOR_STANDBY_MODE)
        {
            return 0;
        }
        UI_reapeat_cough_com();
        break;
    case EM_VENTLATOR_COUGH_MODE:
        if(global_patient_breath_mode == EM_VENTLATOR_STANDBY_MODE)
        {
            return 0;
        }
        to_breath_cough_mode();
        break;

    //兼容 PHILIPS 伟康 V60
    case EM_VENTLATOR_BIPAP_ST_MODE:
        to_breath_st_mode();
        break;

    case EM_VENTLATOR_BIPAP_PCV_MODE:
        to_breath_pcv_mode();
        break;

    // 伟康高级模式，不可用

    //兼容五合一的模式 VOCSN
    case EM_VENTLATOR_BiLEVEL_MODE:
        break;
    case EM_VENTLATOR_SPONTANEOUS_MODE:
        break;
    case EM_VENTLATOR_AC_P_MODE:
        break;
    case EM_VENTLATOR_SIMV_PCV_MODE:
        break;
    case EM_VENTLATOR_AC_V_MODE:
        break;
    case EM_VENTLATOR_SIMV_VCV_MODE:
        break;
    default:
        break;
    }

    //if(get_patient_breath_mode() == ui_set_data.work_mode) return 0;
    //else  return 1;

    return 1;
}

/* *******************************************
*  总的控制入口 所有模式
* *******************************************/
/**
 * [breath_mode_control description]
 * @method breath_mode_control
 */
__weak void breath_mode_control(void)
{
    switch(global_patient_breath_mode)
    {
    case EM_VENTLATOR_STANDBY_MODE:
        breath_standby_mode();
        //to_breath_cpap_mode();
        break;

    //基本模式
    case EM_VENTLATOR_CPAP_MODE:
        //if(is_rap_actived())
        //    break;
        breath_cpap_mode();
        break;

    // 高级模式
    case EM_VENTLATOR_PAV_MODE:
        breath_pav_mode();
        break;
    case EM_VENTLATOR_COUGH_MODE:
        breath_cough_mode();
        break;

    //兼容 PHILIPS 伟康 V60
    case EM_VENTLATOR_BIPAP_ST_MODE:
        breath_st_mode();
        break;
    case EM_VENTLATOR_BIPAP_PCV_MODE:
        breath_pcv_mode();
        break;

    // 伟康高级模式，不可用

    //兼容五合一的模式 VOCSN
    case EM_VENTLATOR_BiLEVEL_MODE:
        breath_bilevel_mode();
        break;
    case EM_VENTLATOR_SPONTANEOUS_MODE:
        breath_spontaneous_mode();
        break;
    case EM_VENTLATOR_AC_P_MODE:
        breath_ac_p_mode();
        break;
    case EM_VENTLATOR_SIMV_PCV_MODE:
        breath_simv_pcv_mode();
        break;
    case EM_VENTLATOR_AC_V_MODE:
        breath_ac_v_mode();
        break;
    case EM_VENTLATOR_SIMV_VCV_MODE:
        breath_simv_vcv_mode();
        break;
    default:
        break;
    }
}

/* *******************************************
*  测试入口
* *******************************************/
/**
 * [test_start_st_mode description]
 * @method test_start_st_mode
 */
void test_start_st_mode(void)
{
    ui_set_data.epap           =  100;// 10 cmH2O
    ui_set_data.ipap           =  200;// 20 cmH2O
    ui_set_data.T_freq         =   10;
    ui_set_data.T_inspire_time = 2000; // 2s
    ui_set_data.T_rising_time  = 1000;
    ui_set_data.c_flex_level   =    0;
    set_changemode_actived_mode(EM_VENTLATOR_BIPAP_ST_MODE);
}

/**
 * [test_start_st_mode1 description]
 * @method test_start_st_mode1
 */
void test_start_st_mode1(void)
{
    ui_set_data.epap           =  50;// 10 cmH2O
    ui_set_data.ipap           =  150;// 20 cmH2O
    ui_set_data.T_freq         =    6;
    ui_set_data.T_inspire_time = 5000; // 2s
    ui_set_data.T_rising_time  = 1500;
    ui_set_data.c_flex_level   =   40;
    set_changemode_actived_mode(EM_VENTLATOR_BIPAP_ST_MODE);
}

/**
 * [test_start_pcv_mode description]
 * @method test_start_pcv_mode
 */
void test_start_pcv_mode(void)
{
    ui_set_data.epap           =  100;// 10 cmH2O
    ui_set_data.ipap           =  200;// 20 cmH2O
    ui_set_data.T_freq         =   10;
    ui_set_data.T_inspire_time = 2000; // 2s
    ui_set_data.T_rising_time  = 1000;

    set_changemode_actived_mode(EM_VENTLATOR_BIPAP_PCV_MODE);
}

/**
 * [test_start_cough_mode description]
 * @method test_start_cough_mode
 */
void test_start_cough_mode(void)
{
    /* ******************************************************
       字节1：Pcough_i 吸气的目标压力（15-60）cmH2O
       字节2：Pcough_e咳痰初始压力（0--->  -90）cmH2O
       字节3：Vtcough 初始潮气量 （30-150）x 10ml
       字节4：CoughTi 咳痰吸气时间（0.5-3.0）秒
       字节5：屏气时间（0.1-2.0S）
       字节6：stop_mode咳痰结束检测条件 （0-3）
       字节7：stop_value检测阈值
    *********************************************************
       检测条件及阈值定义：
       stop_mode          stop_value	单位(uint)
       = 0	潮气量阈值(绝对值)			   0-150    x10ml
       = 1	潮气量百分比				  50-100		%
       = 2	呼气流量阈值				   0-100    L/min
       = 3	时间阈值				   1- 20		0.1秒
    * ******************************************************/
    ui_set_data.cough_prepare_press     =  200;
    ui_set_data.cough_start_press     =  220;// -10cmH2O
    ui_set_data.cough_start_vt        =  400;
    ui_set_data.cough_press_increase_time = 2000;
    ui_set_data.cough_hold_time       = 1000;
    ui_set_data.cough_finish_types      =    2;
    ui_set_data.cough_finish_val      =    5;
    // 设置咳痰指令
    force_start_vacuum_pump();
    set_changemode_actived_mode(EM_VENTLATOR_COUGH_MODE);
}

/**
 * [test_stop_cough_mode description]
 * @method test_stop_cough_mode
 */
void test_stop_cough_mode(void)
{
    stop_cough_blower_work();
    cough_back_to_prev_work();
}

/**
 * [test_repeat_coughmode description]
 * @method test_repeat_coughmode
 */
void test_repeat_coughmode(void)
{
    set_changemode_actived_mode(EM_VENTLATOR_COUGH_REPEAT_MODE);
}

/////////////////////////////////////////////////////////////////
// 返回原来的模式---用于AI控制
/////////////////////////////////////////////////////////////////
/**
 * [breath_AI_back_to_prev_mode description]
 * @method breath_AI_back_to_prev_mode
 */
void breath_AI_back_to_prev_mode(void)
{

    switch(ui_set_data.work_mode)
    {

    //基本模式
    case EM_VENTLATOR_CPAP_MODE:
        // 模式标记
        global_patient_breath_mode = EM_VENTLATOR_CPAP_MODE;
        to_default_breath_status();
        break;
    case EM_VENTLATOR_STANDBY_MODE:
        break;
    case EM_VENTLATOR_BIPAP_ST_MODE:
        global_patient_breath_mode = EM_VENTLATOR_BIPAP_ST_MODE;
        prepare_ST_control_data();
        break;

#if 0
    case EM_VENTLATOR_PAV_MODE:
        break;
    case EM_VENTLATOR_BIPAP_PCV_MODE:
        break;
    case EM_VENTLATOR_BiLEVEL_MODE:
        break;
    case EM_VENTLATOR_SPONTANEOUS_MODE:
        break;
    case EM_VENTLATOR_AC_P_MODE:
        break;
    case EM_VENTLATOR_SIMV_PCV_MODE:
        break;
    case EM_VENTLATOR_AC_V_MODE:
        break;
    case EM_VENTLATOR_SIMV_VCV_MODE:
        break;
#endif
    default:
        break;
    }
}

// ***********************************************
// AI
// ***********************************************
__weak void init_breath_AI_data(void)
{
}

//UI设置参数命令
__weak void ui_set_breath_AI_data(void)
{
}

// 启动命令
__weak void start_breath_AI(uint8_t status)
{
}

// 停止命令
__weak void stop_breath_AI(void)
{
}

// for UI infomation
__weak uint8_t get_breath_AI_current_breath_num(void)
{
    return 0;
}

// for UI infomation
__weak uint8_t get_breath_AI_cycles(void)
{
    return 0;
}

// 执行单元
__weak void breath_AI_namager(void)
{
}

// File: breath_control.c end
