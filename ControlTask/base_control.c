// file base_control.c
#include "base_control.h"
#include "../lib/insert_data.h"
#include "../global.h"
#include "../ModeControl/breath_rap.h"
#include "../lib/leak_flow_count.h"

/// function or variable declarations
MAIN_BASE_CONTROL_DATA_DEFINES PID_blower_control_data;
int16_t  get_current_press(void);
void     set_blower_speed(uint16_t speed);
uint16_t get_blower_current_set_speed(void);

#define get_current_press() display_count_data.Press
/* ***************************************************/
// 未完成，先使用简单的处理函数！！！
__weak void flow_set(void)
{
}
__weak void flow_adjust(void)
{
}
__weak void epap_set(void)
{
}
__weak void epap_adjust(void)
{
}

//涡轮控制归零
// 插值算法（控制值）的初始化
// 初始化流量及压力控制
__weak void reset_flow_control_data(void)
{
}

// 呼气阀完全打开 = 0
// 相关控制数据的初始化
// 初始化呼气阀
__weak void reset_epap_control_data(void)
{
}
/* ***************************************************/
typedef enum __PID_BLOWER_CONTROL_DATA_TYPES
{
    ENUM_PID_BLOWER_CONTROL_BASE_DATA,
    ENUM_PID_BLOWER_CONTROL_IPAP_ADJUST_DATA,
    ENUM_PID_BLOWER_CONTROL_EPAP_ADJUST_DATA,
    ENUM_PID_BLOWER_CONTROL_PSUPPORT_ADJUST_DATA,
    ENUM_PID_BLOWER_CONTROL_CFLEX_ADJUST_DATA
} PID_BLOWER_CONTROL_DATA_TYPES;


// 在患者主动吸气/呼气时，会造成压力波动，这个参数是用于控制补偿的，
// 具体来说：呼气中就是C-Flex的补偿过程， 吸气中 是P-Support过程 不是‘SP’(Spontaneous)那个过程
// 目前还没由进行到这一步
static struct _MIAN_BASE_CONTROL_FLAGS
{
    uint16_t i_completed_flag : 1; // CPAP/BiPAP中 用于补偿患者在主动吸气时造成的压力降低的情况
    uint16_t e_completed_flag : 1; // CPAP/BiPAP中 用于补偿患者在主动呼气时造成的压力降低的情况
} main_base_control_flags;

extern uint32_t PEEP_VALVE_DATA[];
extern uint32_t PEEP_VALVE_IDX[];
extern uint32_t PEEP_BLOWER_DATA[];
extern ST_INSERT_DATA peep_valve_control_data;
extern ST_INSERT_DATA blower_press_control_data;

PWM_CONTROL_DATA_DEFINES pwm_control_data;

/**
 * [init_peep_valve_control_data description]
 * @method init_peep_valve_control_data
 */
void init_peep_valve_control_data(void)
{
    init_insert_data(&peep_valve_control_data, 25, PEEP_VALVE_IDX, PEEP_VALVE_DATA, ENUM_DATA_IS_DECREASE);
    init_insert_data(&blower_press_control_data, 20, PEEP_VALVE_IDX, PEEP_BLOWER_DATA, ENUM_DATA_IS_INCREASE);
}

/**
 * [set_main_base_control_flag description]
 * @method set_main_base_control_flag
 * @param  type                       [description]
 */
void set_main_base_control_flag(uint8_t type)
{
    switch(type)
    {
    case EM_I_COMPLETED_FLAGS:
        main_base_control_flags.i_completed_flag = 1;
        break;
    case EM_E_COMPLETED_FLAGS:
        main_base_control_flags.e_completed_flag = 1;
        break;
    }
}

/**
 * [reset_main_base_control_flag description]
 * @method reset_main_base_control_flag
 * @param  type                         [description]
 */
void reset_main_base_control_flag(uint8_t type)
{
    switch(type)
    {
    case EM_I_COMPLETED_FLAGS:
        main_base_control_flags.i_completed_flag = 0;
        break;
    case EM_E_COMPLETED_FLAGS:
        main_base_control_flags.e_completed_flag = 0;
        break;
    }
}

/**
 * [is_main_base_control_flag_set description]
 * @method is_main_base_control_flag_set
 * @param  type                          [description]
 * @return                               [description]
 */
uint8_t is_main_base_control_flag_set(uint8_t type)
{
    switch(type)
    {
    case EM_I_COMPLETED_FLAGS:
        return main_base_control_flags.i_completed_flag;
    case EM_E_COMPLETED_FLAGS:
        return main_base_control_flags.e_completed_flag;
    }
    return 0;
}

/**
 * [set_blower_pwm_comtrol_data description]
 * @method set_blower_pwm_comtrol_data
 * @param  val                         [description]
 */
#if 1  // 用于记录和查询PWM实际设置的数值（或者直接查看硬件的CCR）
void set_blower_pwm_comtrol_data(uint16_t val)
{
    pwm_control_data.pwm_blower_val = val;
}

/**
 * [get_blower_pwm_comtrol_data description]
 * @method get_blower_pwm_comtrol_data
 * @return                             [description]
 */
uint16_t get_blower_pwm_comtrol_data(void)
{
    return pwm_control_data.pwm_blower_val;
}

/**
 * [set_peep_pwm_comtrol_data description]
 * @method set_peep_pwm_comtrol_data
 * @param  val                       [description]
 */
void set_peep_pwm_comtrol_data(uint16_t val)
{
    pwm_control_data.pwm_peep_val = val;
}

/**
 * [get_peep_pwm_comtrol_data description]
 * @method get_peep_pwm_comtrol_data
 * @return                           [description]
 */
uint16_t get_peep_pwm_comtrol_data(void)
{
    return pwm_control_data.pwm_peep_val;
}

/**
 * [set_oxygen_pwm_comtrol_data description]
 * @method set_oxygen_pwm_comtrol_data
 * @param  val                         [description]
 */
void set_oxygen_pwm_comtrol_data(uint16_t val)
{
    pwm_control_data.pwm_oxygen_val = val;
}

/**
 * [get_oxygen_pwm_comtrol_data description]
 * @method get_oxygen_pwm_comtrol_data
 * @return                             [description]
 */
uint16_t get_oxygen_pwm_comtrol_data(void)
{
    return pwm_control_data.pwm_oxygen_val;
}

/**
 * [set_flowback_pwm_comtrol_data description]
 * @method set_flowback_pwm_comtrol_data
 * @param  val                           [description]
 */
void set_flowback_pwm_comtrol_data(uint16_t val)
{
    pwm_control_data.pwm_flowback_val = val;
}

/**
 * [get_flowback_pwm_comtrol_data description]
 * @method get_flowback_pwm_comtrol_data
 * @return                               [description]
 */
uint16_t get_flowback_pwm_comtrol_data(void)
{
    return pwm_control_data.pwm_flowback_val;
}

/**
 * [set_vacuum_blower_pwm_comtrol_data description]
 * @method set_vacuum_blower_pwm_comtrol_data
 * @param  val                                [description]
 */
void set_vacuum_blower_pwm_comtrol_data(uint16_t val)
{
    pwm_control_data.pwm_vacuum_blower_val = val;
}

/**
 * [get_vacuum_blower_pwm_comtrol_data description]
 * @method get_vacuum_blower_pwm_comtrol_data
 * @return                                    [description]
 */
uint16_t get_vacuum_blower_pwm_comtrol_data(void)
{
    return pwm_control_data.pwm_vacuum_blower_val;
}
#endif

/**
 * [S_T_set_inc_speed description]
 * @method S_T_set_inc_speed
 * @param  rising_time_count     [description]
 * @param  blower_increase_speed [description]
 */
void S_T_set_inc_speed(uint16_t rising_time_count, uint16_t blower_increase_speed)
{
    // first load this program
    if(rising_time_count == 0)
    {
        PID_blower_control_data.base_speed = PID_blower_control_data.Low_speed + blower_increase_speed;
        set_PID_blower_control_status(EM_BREATH_PRESS_INCREASE);
        // 关闭 blower pid调整模式
        set_PID_blower_adjust_mode(ENUM_PID_FAST_FOR_PRESS);
    }
    else
    {
        // 逐步增加转速以达到目标压力
        PID_blower_control_data.base_speed += blower_increase_speed;
    }
    // 调整涡轮
    set_blower_speed(PID_blower_control_data.base_speed + PID_blower_control_data.blower_p_support_adjust_val);
    return;
}

/**
 * [PCV_set_inc_speed description]
 * @method PCV_set_inc_speed
 * @param  rising_time_count     [description]
 * @param  blower_increase_speed [description]
 */
void PCV_set_inc_speed(uint16_t rising_time_count, uint16_t blower_increase_speed)
{
    // first load this program
    if(rising_time_count == 0)
    {
        PID_blower_control_data.base_speed = PID_blower_control_data.Low_speed + blower_increase_speed;
        set_PID_blower_control_status(EM_BREATH_PCV_PRESS_INC);
        // 关闭 blower pid调整模式
        set_PID_blower_adjust_mode(ENUM_PID_FAST_FOR_PRESS);
    }
    else
    {
        // 逐步增加转速以达到目标压力
        PID_blower_control_data.base_speed += blower_increase_speed;
    }
    // 调整涡轮
    set_blower_speed(PID_blower_control_data.base_speed + PID_blower_control_data.blower_p_support_adjust_val);
    return;
}

/**
 * [cough_enable_press_adjust description]
 * @method cough_enable_press_adjust
 * @param  press                     [description]
 */
void cough_enable_press_adjust(int16_t press)
{
    PID_blower_control_data.base_speed = get_blower_speed_for_press(press);
    set_blower_speed(PID_blower_control_data.base_speed);
    set_PID_blower_control_status(EM_BREATH_COUGH_SUPPORT);
    // 关闭 blower pid调整模式
    set_PID_blower_adjust_mode(ENUM_PID_FAST_FOR_PRESS);
}

/**
 * [Cough_increase_pid_base_speed description]
 * @method Cough_increase_pid_base_speed
 * @param  rising_time_count             [description]
 * @param  blower_increase_speed         [description]
 */
void Cough_increase_pid_base_speed(uint16_t rising_time_count, uint16_t blower_increase_speed)
{
    // first load this program
    if(rising_time_count == 0)
    {
        // PID_blower_control_data.base_speed = get_blower_speed_for_press(main_control_data.epap);
        set_PID_blower_control_status(EM_BREATH_COUGH_SUPPORT);
        // 关闭 blower pid调整模式
        set_PID_blower_adjust_mode(ENUM_PID_FAST_FOR_PRESS);
    }
    else
    {}
    // 逐步增加转速以达到目标压力
    PID_blower_control_data.base_speed += blower_increase_speed;
    // 调整涡轮
    set_blower_speed(PID_blower_control_data.base_speed);
    return;
}

/**
 * [Base_set_speed_T_press description]
 * @method Base_set_speed_T_press
 */
void Base_set_speed_T_press(void)
{
    set_PID_blower_control_status(EM_BREATH_PCV_PRESS_HOLD);
    set_PID_blower_adjust_mode(ENUM_PID_FAST_FOR_PRESS);
    PID_blower_control_data.T_High_speed = get_blower_current_set_speed();
    PID_blower_control_data.blower_p_support_adjust_val = 0;

    // 调整涡轮
    set_blower_speed(PID_blower_control_data.T_High_speed);
}

// 设置当前工作的状态标记
/**
 * [Base_set_speed_for_epap description]
 * @method Base_set_speed_for_epap
 */
void Base_set_speed_for_epap(void)
{
    set_PID_blower_control_status(EM_BREATH_PRESS_HOLD_LOW);
    set_PID_blower_adjust_mode(ENUM_PID_FAST_FOR_PRESS);
    PID_blower_control_data.blower_e_pap_adjust_val = 0;

    // 调整涡轮
    set_blower_speed(PID_blower_control_data.Low_speed);
}

/**
 * [Base_set_speed_for_ipap description]
 * @method Base_set_speed_for_ipap
 */
void Base_set_speed_for_ipap(void)
{
    set_PID_blower_control_status(EM_BREATH_PRESS_HOLD_HIGH);
    set_PID_blower_adjust_mode(ENUM_PID_FAST_FOR_PRESS);


    PID_blower_control_data.High_speed = get_blower_current_set_speed();
    PID_blower_control_data.blower_i_pap_adjust_val = 0;
    // 调整涡轮
    set_blower_speed(PID_blower_control_data.High_speed);
}

/**
 * [S_T_set_speed_cflex description]
 * @method S_T_set_speed_cflex
 */
void S_T_set_speed_cflex(void)
{
    set_PID_blower_control_status(EM_BREATH_EXHALE_SUPPORT);
    set_PID_blower_adjust_mode(ENUM_PID_NO_ADJUST);//(ENUM_PID_FAST_FOR_PRESS);

    // 调整涡轮
    set_blower_speed(PID_blower_control_data.C_Flex_speed + PID_blower_control_data.blower_cflex_adjust_val);
}

/**
 * [clear_pid_adjust_val description]
 * @method clear_pid_adjust_val
 * @param  type                 [description]
 */
void clear_pid_adjust_val(uint8_t type)
{
    switch(type)
    {
    case EM_BASE_SPEED_EPAP:
        PID_blower_control_data.blower_e_pap_adjust_val = 0;
        break;
    case EM_BASE_SPEED_IPAP:
        PID_blower_control_data.blower_i_pap_adjust_val = 0;
        break;
    case EM_BASE_SPEED_C_FLEX:
        PID_blower_control_data.blower_cflex_adjust_val = 0;
        break;
    case EM_BASE_SPEED_BASE:
        PID_blower_control_data.blower_p_support_adjust_val = 0;
        break;
    default:
        break;
    }
}
/* ***************************************************/
// 简单的处理函数
/* ***************************************************/
/**
 * [reset_PID_blower_control_data description]
 * @method reset_PID_blower_control_data
 */
void reset_PID_blower_control_data(void)
{
    PID_blower_control_data.blower_cflex_adjust_val       = 0;
    PID_blower_control_data.blower_e_pap_adjust_val       = 0;
    PID_blower_control_data.blower_i_pap_adjust_val       = 0;
    PID_blower_control_data.blower_p_support_adjust_val   = 0;

#if BASE_CONTROL_OVERFLOW_ENABLE
    PID_blower_control_data.base_over_flow                = 0;
    PID_blower_control_data.overflow_cflex_adjust_val     = 0;
    PID_blower_control_data.overflow_e_pap_adjust_val     = 0;
    PID_blower_control_data.overflow_i_pap_adjust_val     = 0;
    PID_blower_control_data.overflow_p_support_adjust_val = 0;
#endif

#if BASE_CONTROL_PEEP_ENABLE
    PID_blower_control_data.Low_peep                      = 0;
    PID_blower_control_data.High_peep                     = 0;
    PID_blower_control_data.peep_cflex_adjust_val         = 0;
    PID_blower_control_data.peep_e_pap_adjust_val         = 0;
    PID_blower_control_data.peep_i_pap_adjust_val         = 0;
    PID_blower_control_data.peep_p_support_adjust_val     = 0;
#endif

    PID_blower_control_data.base_press                    = 0;
    PID_blower_control_data.base_speed                    = 0;
    PID_blower_control_data.base_peep_valve_control_val   = 0;

    PID_blower_control_data.C_Flex_speed                  = 0;
    PID_blower_control_data.High_speed                    = 0;
    PID_blower_control_data.Low_speed                     = 0;
    PID_blower_control_data.adjust_mode                   = ENUM_PID_NO_ADJUST;
}

uint16_t get_base_control_high_speed(void)
{
    return PID_blower_control_data.High_speed + PID_blower_control_data.blower_i_pap_adjust_val;
}


// 需要考虑基础流量！！！
/**
 * [set_PID_blower_control_base_speed description]
 * @method set_PID_blower_control_base_speed
 * @param  type                              [description]
 */
void set_PID_blower_control_base_speed(uint8_t type)
{
    switch(type)
    {
    case EM_BASE_SPEED_EPAP:
        PID_blower_control_data.Low_speed    = get_blower_speed_for_press_and_flow(main_control_data.epap,
                                               main_control_data.flow);
        break;
    case EM_BASE_SPEED_IPAP:
        PID_blower_control_data.High_speed   = get_blower_speed_for_press_and_flow(main_control_data.ipap,
                                               main_control_data.flow);
        break;
    case EM_BASE_SPEED_C_FLEX:
        PID_blower_control_data.C_Flex_speed = get_blower_speed_for_press_and_flow(main_control_data.c_flex_level,
                                               main_control_data.flow);
        break;
    case EM_BASE_SPEED_BASE: // 假设 T模式中 吸气压力和IPAP不同
        PID_blower_control_data.T_High_speed = get_blower_speed_for_press_and_flow(main_control_data.T_press,
                                               main_control_data.flow);
        break;
    }
}

// for read/write operation, we need pointer!!!!
/**
 * [get_speed_control_data description]
 * @method get_speed_control_data
 * @param  type                   [description]
 * @return                        [description]
 */
static int16_t *get_speed_control_data(PID_BLOWER_CONTROL_DATA_TYPES type)
{
    switch(type)
    {
    default:
        return &(PID_blower_control_data.temp);
    case ENUM_PID_BLOWER_CONTROL_BASE_DATA:
        return &(PID_blower_control_data.temp);
    //return (int16_t *)  &(PID_blower_control_data.base_speed); // 数据长度不对 尽量不要用！
    case ENUM_PID_BLOWER_CONTROL_IPAP_ADJUST_DATA:
        return &(PID_blower_control_data.blower_i_pap_adjust_val);
    case ENUM_PID_BLOWER_CONTROL_EPAP_ADJUST_DATA:
        return &(PID_blower_control_data.blower_e_pap_adjust_val);
    case ENUM_PID_BLOWER_CONTROL_PSUPPORT_ADJUST_DATA:
        return &(PID_blower_control_data.blower_p_support_adjust_val);
    case ENUM_PID_BLOWER_CONTROL_CFLEX_ADJUST_DATA:
        return &(PID_blower_control_data.blower_cflex_adjust_val);
    }
}


/**
 * [get_overflow_control_data description]
 * @method get_overflow_control_data
 * @param  type                      [description]
 * @return                           [description]
 */
// for read/write operation, we need pointer!!!!
static int16_t *get_overflow_control_data(PID_BLOWER_CONTROL_DATA_TYPES type)
{
#if BASE_CONTROL_OVERFLOW_ENABLE
    switch(type)
    {
    default:
        return &(PID_blower_control_data.temp);
    case ENUM_PID_BLOWER_CONTROL_BASE_DATA:
        return &(PID_blower_control_data.base_over_flow);
    case ENUM_PID_BLOWER_CONTROL_IPAP_ADJUST_DATA:
        return &(PID_blower_control_data.overflow_i_pap_adjust_val);
    case ENUM_PID_BLOWER_CONTROL_EPAP_ADJUST_DATA:
        return &(PID_blower_control_data.overflow_e_pap_adjust_val);
    case ENUM_PID_BLOWER_CONTROL_PSUPPORT_ADJUST_DATA:
        return &(PID_blower_control_data.overflow_p_support_adjust_val);
    case ENUM_PID_BLOWER_CONTROL_CFLEX_ADJUST_DATA:
        return &(PID_blower_control_data.overflow_cflex_adjust_val);
    }
#else
    return &(PID_blower_control_data.temp);
#endif
}

/**
 * [get_peep_control_data description]
 * @method get_peep_control_data
 * @param  type                  [description]
 * @return                       [description]
 */
static int16_t *get_peep_control_data(PID_BLOWER_CONTROL_DATA_TYPES type)
{
#if BASE_CONTROL_PEEP_ENABLE
    switch(type)
    {
    default:
        return &(PID_blower_control_data.temp);
    case ENUM_PID_BLOWER_CONTROL_BASE_DATA:
        return &(PID_blower_control_data.Low_peep);
    case ENUM_PID_BLOWER_CONTROL_IPAP_ADJUST_DATA:
        return &(PID_blower_control_data.peep_i_pap_adjust_val);
    case ENUM_PID_BLOWER_CONTROL_EPAP_ADJUST_DATA:
        return &(PID_blower_control_data.peep_e_pap_adjust_val);
    case ENUM_PID_BLOWER_CONTROL_PSUPPORT_ADJUST_DATA:
        return &(PID_blower_control_data.peep_p_support_adjust_val);
    case ENUM_PID_BLOWER_CONTROL_CFLEX_ADJUST_DATA:
        return &(PID_blower_control_data.peep_cflex_adjust_val);
    }
#else
    return &(PID_blower_control_data.temp);
#endif

}

/**
 * [get_speed_main_control_data description]
 * @method get_speed_main_control_data
 * @return                             [description]
 */
int16_t *get_speed_main_control_data(void)
{
    switch(get_main_control_status())
    {
    default:
        break;
    case EM_BREATH_STANDBY: // 未启动
        break;
    case EM_BREATH_PRESS_HOLD_HIGH: // IPAP BiLEVEL
        return get_speed_control_data(ENUM_PID_BLOWER_CONTROL_IPAP_ADJUST_DATA);
    case EM_BREATH_PRESS_HOLD_LOW: // EPAP BiLEVEL
        return get_speed_control_data(ENUM_PID_BLOWER_CONTROL_EPAP_ADJUST_DATA);
    case EM_BREATH_FLOW_SUPPORT: // 用于容量控制CMV/VCV
        break;
    case EM_BREATH_PRESS_INCREASE: // 用于压力上升控制
        return get_speed_control_data(ENUM_PID_BLOWER_CONTROL_PSUPPORT_ADJUST_DATA);
    case EM_BREATH_INHALE_SUPPORT: // 吸气时的补偿状态 Spontaneous Psupport
        return get_speed_control_data(ENUM_PID_BLOWER_CONTROL_PSUPPORT_ADJUST_DATA);
    case EM_BREATH_EXHALE_SUPPORT: // 呼气时的补偿状态 Spontaneous 辅助呼气
        return get_speed_control_data(ENUM_PID_BLOWER_CONTROL_CFLEX_ADJUST_DATA);
    }
    return &(PID_blower_control_data.temp);
}

/**
 * [get_overflow_main_control_data description]
 * @method get_overflow_main_control_data
 * @return                                [description]
 */
int16_t *get_overflow_main_control_data(void)
{
    switch(get_main_control_status())
    {
    default:
        break;
    case EM_BREATH_STANDBY: // 未启动
        break;
    case EM_BREATH_PRESS_HOLD_HIGH: // IPAP BiLEVEL
        return get_overflow_control_data(ENUM_PID_BLOWER_CONTROL_IPAP_ADJUST_DATA);
    case EM_BREATH_PRESS_HOLD_LOW: // EPAP BiLEVEL
        return get_overflow_control_data(ENUM_PID_BLOWER_CONTROL_EPAP_ADJUST_DATA);
    case EM_BREATH_FLOW_SUPPORT: // 用于容量控制CMV/VCV
        break;
    case EM_BREATH_PRESS_INCREASE: // 用于压力上升控制
        return get_overflow_control_data(ENUM_PID_BLOWER_CONTROL_PSUPPORT_ADJUST_DATA);
    case EM_BREATH_INHALE_SUPPORT: // 吸气时的补偿状态 Spontaneous Psupport
        return get_overflow_control_data(ENUM_PID_BLOWER_CONTROL_PSUPPORT_ADJUST_DATA);
    case EM_BREATH_EXHALE_SUPPORT: // 呼气时的补偿状态 Spontaneous 辅助呼气
        return get_overflow_control_data(ENUM_PID_BLOWER_CONTROL_CFLEX_ADJUST_DATA);
    }
    return &(PID_blower_control_data.temp);
}

/**
 * [get_peep_main_control_data description]
 * @method get_peep_main_control_data
 * @return                            [description]
 */
int16_t *get_peep_main_control_data(void)
{
    switch(get_main_control_status())
    {
    default:
        break;
    case EM_BREATH_STANDBY: // 未启动
        break;
    case EM_BREATH_PRESS_HOLD_HIGH: // IPAP BiLEVEL
        return get_peep_control_data(ENUM_PID_BLOWER_CONTROL_IPAP_ADJUST_DATA);
    case EM_BREATH_PRESS_HOLD_LOW: // EPAP BiLEVEL
        return get_peep_control_data(ENUM_PID_BLOWER_CONTROL_EPAP_ADJUST_DATA);
    case EM_BREATH_FLOW_SUPPORT: // 用于容量控制CMV/VCV
        break;
    case EM_BREATH_PRESS_INCREASE: // 用于压力上升控制
        return get_peep_control_data(ENUM_PID_BLOWER_CONTROL_PSUPPORT_ADJUST_DATA);
    case EM_BREATH_INHALE_SUPPORT: // 吸气时的补偿状态 Spontaneous Psupport
        return get_peep_control_data(ENUM_PID_BLOWER_CONTROL_PSUPPORT_ADJUST_DATA);
    case EM_BREATH_EXHALE_SUPPORT: // 呼气时的补偿状态 Spontaneous 辅助呼气
        return get_peep_control_data(ENUM_PID_BLOWER_CONTROL_CFLEX_ADJUST_DATA);
    }
    return &(PID_blower_control_data.temp);
}


/**
 * [reset_P_support_adjust_val description]
 * @method reset_P_support_adjust_val
 */
void reset_P_support_adjust_val(void)
{
    PID_blower_control_data.blower_p_support_adjust_val = 0;
}

// 目前，在10升流量10厘米水的基础上，涡轮每增加1000转大约能增加1个厘米水的压力，以此为基础，进行简单调整
// 理论上，只在吸气时需要快速调整，呼气时不需要，这个要看情况后期再进行修正
// 目标： PID_blower_control_data.base_press
/**
 * [main_control_direct_blower_adjust description]
 * @method main_control_direct_blower_adjust
 */
void main_control_direct_blower_adjust(void)
{
    int32_t base;
    int32_t adj_val;
    int16_t *data_ptr;
    int16_t diff, abs_diff;

    base = PID_blower_control_data.base_speed;
    data_ptr = get_speed_main_control_data();

    // 计算新的调整量
    /// 计算压力差
    diff = PID_blower_control_data.base_press - get_current_press();
    if(diff > 0) abs_diff = diff;
    else abs_diff = 0 - diff;
    /// 依据压力差的大小进行相应调节
    if(abs_diff < 10) ///如果压力差小于1 cmH2O 进行微调
    {
        //10= 1厘米水=1000转左右的变化
        adj_val = (int32_t)diff * 105;
    }
    else ///否则加大调整力度
    {
        //10= 1厘米水=1000转左右的变化
        adj_val = (int32_t)diff * 120;
    }

    // 保存调整量
    *data_ptr = adj_val;
    adj_val += base;
    // 调整涡轮
    set_blower_speed((uint16_t)adj_val);
}



// 目前，在10升流量10厘米水的基础上，涡轮每增加1000转大约能增加1个厘米水的压力，以此为基础，进行简单调整
// 理论上，只在吸气时需要快速调整，呼气时不需要，这个要看情况后期再进行修正
// 这个调整是叠加的调整，所以调用时不能频繁
// 目标： PID_blower_control_data.base_press
/**
 * [main_control_Spontaneous_blower_adjust description]
 * @method main_control_Spontaneous_blower_adjust
 */
void main_control_Spontaneous_blower_adjust(void)
{
    int32_t adj_val;
    int16_t diff, abs_diff;

    // 计算新的调整量
    /// 计算压力差
    diff = PID_blower_control_data.base_press - get_current_press();
    if(diff > 0) abs_diff = diff;
    else abs_diff = 0 - diff;
    /// 依据压力差的大小进行相应调节
    if(abs_diff < 0) ///如果压力差小于1 cmH2O 进行微调
    {
        adj_val = (int32_t)diff * 1;
    }
    else ///否则加大调整力度
    {
        adj_val = (int32_t)diff * 3;
    }
    PID_blower_control_data.blower_p_support_adjust_val = (int16_t)adj_val;
    adj_val += PID_blower_control_data.base_speed;
    // 调整涡轮
    set_blower_speed((uint16_t)adj_val);
}

/**
 * [main_control_fast_for_press description]
 * @method main_control_fast_for_press
 */
void main_control_fast_for_press(void)
{
    uint16_t *base;
    int32_t adj_val;
    int16_t  *data_ptr;
    int16_t diff, abs_diff;

    switch(main_control_data.status)
    {
    default:
        return; // 不调整
    case EM_BREATH_PCV_PRESS_INC: // 用于PCV压力上升过程				 main_control_data.epap -> -> main_control_data.T_press
#if BLOWER_ADJUST_PID_PRESS_INCREASE_ENABLE
        base = &PID_blower_control_data.base_speed;
        data_ptr = &PID_blower_control_data.blower_p_support_adjust_val;

        /// 计算压力差
        diff = get_PCV_T_target_press() - get_current_press() - 5;
        goto DIRECT_PRESS_LABEL;//break;
#else
        return;
#endif
    case EM_BREATH_PCV_PRESS_HOLD: // 用于PCV压力保持过程				 main_control_data.T_press
        base = &PID_blower_control_data.T_High_speed;
        data_ptr = &PID_blower_control_data.blower_p_support_adjust_val;

        /// 计算压力差
        // // 目标： main_control_data.T_press
        diff = main_control_data.T_press - get_current_press() - 5;
        break;
    case EM_BREATH_STANDBY: // 未启动
        return; // 不调整

    case EM_BREATH_PRESS_HOLD_HIGH: // IPAP BiLEVEL
        base = &PID_blower_control_data.High_speed;
        data_ptr = &PID_blower_control_data.blower_i_pap_adjust_val;

        /// 计算压力差
        // // 目标： main_control_data.ipap
        diff = main_control_data.ipap - get_current_press() - 5;
        break;

    case EM_BREATH_PRESS_HOLD_LOW: // EPAP BiLEVEL
        base = &PID_blower_control_data.Low_speed;
        data_ptr = &PID_blower_control_data.blower_e_pap_adjust_val;

        /// 计算压力差
        diff = main_control_data.epap - get_current_press() - 5;
        break;

    case EM_BREATH_FLOW_SUPPORT: // 用于容量控制CMV/VCV
        return; // 不调整

    case EM_BREATH_PRESS_INCREASE: // 用于压力上升控制
#if BLOWER_ADJUST_PID_PRESS_INCREASE_ENABLE
        base = &PID_blower_control_data.base_speed;
        data_ptr = &PID_blower_control_data.blower_p_support_adjust_val;

        /// 计算压力差
        diff = get_ST_T_target_press() - get_current_press() - 5;
        goto DIRECT_PRESS_LABEL;//break;
#else
        return;
#endif


    case EM_BREATH_COUGH_SUPPORT:

#if BLOWER_ADJUST_PID_PRESS_INCREASE_ENABLE
        base = &PID_blower_control_data.base_speed;
        data_ptr = &PID_blower_control_data.blower_p_support_adjust_val;

        /// 计算压力差
        // 为确保咳痰功能能实现，这里暂时增加了一点点压力差值
        diff = get_Cough_target_press() - get_current_press() + 5;
        goto DIRECT_PRESS_LABEL;//break;
#else
        return;
#endif

    case EM_BREATH_INHALE_SUPPORT: // 吸气时的补偿状态 Spontaneous Psupport
        return; // 不调整

    // 貌似C-FLEX工作由涡轮风机去调整会有些问题，应该由PEEP阀调整才对!!!!待研究
    case EM_BREATH_EXHALE_SUPPORT: // 呼气时的补偿状态的C-flex Spontaneous 辅助呼气
        base = &PID_blower_control_data.C_Flex_speed;
        data_ptr = &PID_blower_control_data.blower_cflex_adjust_val;

        /// 计算压力差
        diff = main_control_data.c_flex_level - get_current_press() - 5;
        break;
    }

    adj_val = (int32_t) * data_ptr;
    if(diff > 0)
        abs_diff = diff;
    else
        abs_diff = 0 - diff;
    /// 依据压力差的大小进行相应调节
    if(abs_diff < 10) ///如果压力差小于1 cmH2O 进行微调
    {
        //10= 1厘米水=1000转左右的变化
        // 初步50ms调整一次 1秒钟调整20次，寄希望于10次达到目标
        adj_val += (int32_t)diff >> 1;
    }
    else if(abs_diff < 25) ///如果压力差小于1 cmH2O 进行微调
    {
        //10= 1厘米水=1000转左右的变化
        // 初步50ms调整一次 1秒钟调整20次，寄希望于10次达到目标
        adj_val += (int32_t)diff * 3;
    }
    else if(abs_diff < 65) ///如果压力差小于1 cmH2O 进行微调
    {
        //10= 1厘米水=1000转左右的变化
        // 初步50ms调整一次 1秒钟调整20次，寄希望于10次达到目标
        adj_val += (int32_t)diff * 5;
    }
    else ///否则加大调整力度
    {
        //10= 1厘米水=1000转左右的变化
        // 初步50ms调整一次 1秒钟调整20次，寄希望于3次达到目标
        adj_val += (int32_t)diff * 6;
    }

    // 保存调整量
    *data_ptr = adj_val;
    adj_val += *base;
#if 0
    if(adj_val < MIN_BLOWER_SPEED)
        adj_val = MIN_BLOWER_SPEED;
#else
    NO_LESS_THAN(adj_val, MIN_BLOWER_SPEED);
#endif
    // 调整涡轮
    set_blower_speed((uint16_t)adj_val);
    return;

#if BLOWER_ADJUST_PID_PRESS_INCREASE_ENABLE

DIRECT_PRESS_LABEL:

    if(diff > 0)
        abs_diff = diff;
    else
        abs_diff = 0 - diff;
    /// 依据压力差的大小进行相应调节


    abs_diff /= 10;
    switch(abs_diff)
    {
    case 0:
        adj_val = (int32_t)diff * 1;
        break;
    case 1:
        adj_val = (int32_t)diff * 2;
        break;
    case 2:
    case 3:
        adj_val = (int32_t)diff * 3;
        break;
    case 4:
    case 5:
        adj_val = (int32_t)diff * 4;
        break;

    default:
        adj_val = (int32_t)diff * 5;
        break;
    }
    // 保存调整量
    *data_ptr = adj_val;
    adj_val += *base;
    //if(adj_val < MIN_BLOWER_SPEED) adj_val = MIN_BLOWER_SPEED;
    NO_LESS_THAN(adj_val, MIN_BLOWER_SPEED);
    // 调整涡轮
    set_blower_speed((uint16_t)adj_val);
    return;
#endif
}

/**
 * [main_control_pid_blower_adjust description]
 * @method main_control_pid_blower_adjust
 */
void main_control_pid_blower_adjust(void)
{
    if(is_rap_actived())
        return; // for rap control
    //变量定义
    switch(PID_blower_control_data.adjust_mode)
    {
    default:
        return; //（不调整）
    case ENUM_PID_NO_ADJUST: // （=0 不调整）
        return;
    case ENUM_PID_FAST_FOR_PRESS: // （=1 快速压力调整）
        //初始值
        main_control_fast_for_press();
        break;
    case ENUM_PID_FAST_FOR_FLOW: // （=2 快速流量调整【吸气口】）
        return; // 暂时不做 （容量模式专用）
    case ENUM_PID_SUPPORT_FOR_PRESS:
        main_control_Spontaneous_blower_adjust();
        break;
    }
}

/**
 * [set_PID_blower_adjust_mode description]
 * @method set_PID_blower_adjust_mode
 * @param  mode                       [description]
 */
void set_PID_blower_adjust_mode(uint8_t mode)
{
    PID_blower_control_data.adjust_mode = mode;
}


void set_base_speed(uint16_t speed)
{
    PID_blower_control_data.base_speed = speed;
}

#define MAX_BLOWER_PRESS 25
/**
 */
const uint16_t CONST_BLOWER_PRESS_DATA[] =
{
    7200,  8100,  9000,  9900, 10800,
    11900, 12950, 13900, 14850, 15700,
    16600, 17500, 18400, 19200, 20100,
    21200, 22500, 23800, 25700, 27200,
    28900, 30800, 32500, 35000, 37000,
    39600
};

/**
 * [get_blower_speed_for_press description]
 * @method get_blower_speed_for_press
 * @param  press                      [description]
 * @return                            [description]
 */
// 这是个简化版的函数，需要不断地改进
uint16_t get_blower_speed_for_press(int16_t press)
{
    int index;
    //	uint32_t speed;
    index = press / 10;
    if(index > MAX_BLOWER_PRESS)
        index = MAX_BLOWER_PRESS;
    else if(index < 0)
        index = 0;
    return (uint16_t) CONST_BLOWER_PRESS_DATA[index];
}

/**
 * [set_blower_press description]
 * @method set_blower_press
 * @param  press            [description]
 */
void set_blower_press(uint16_t press)
{
    int index;
    uint16_t speed;
    index = press / 10;
    if(index > MAX_BLOWER_PRESS)
        index = MAX_BLOWER_PRESS;
    PID_blower_control_data.base_press = index * 10;
    PID_blower_control_data.base_speed = (uint16_t) CONST_BLOWER_PRESS_DATA[index];
    speed                             = PID_blower_control_data.base_speed;
    //if(speed > MAX_BLOWER_SPEED) speed = MAX_BLOWER_SPEED;
    NO_MORE_THAN(speed, MAX_BLOWER_SPEED);
    set_blower_speed((uint16_t)speed);
}

#define MAX_PEEP_PRESS 25
const uint16_t CONST_PEEP_PRESS_DATA[] =
{
    810,   834,  858,  882,  906,
    930,   994, 1028, 1058, 1130,
    1110, 1200, 1200, 1200, 1200,
    1200, 1200, 1230, 1262, 1282,
    1314, 1342, 1374, 1400, 1400,
    1400
};

void set_peep_valve_control_val(uint16_t val);
/**
 * [set_peep_press description]
 * @method set_peep_press
 * @param  press          [description]
 */
void set_peep_press(uint16_t press)
{
    int index;
    index = press / 10;
    if(index > MAX_PEEP_PRESS)
        index = MAX_PEEP_PRESS;
    set_peep_valve_control_val((uint16_t)CONST_PEEP_PRESS_DATA[index]);
}

/**
 * [close_peep description]
 * @method close_peep
 */
void close_peep(void)
{
    set_peep_valve_control_val(MAX_PWM_VAL);
}

/**
 * [open_peep description]
 * @method open_peep
 */
void open_peep(void)
{
    set_peep_valve_control_val(0);
}

// 张志新：用于调整PEEP阀，以保证输出流量满足要求
// 每次只调整大约总偏差的1/8
// 期望在1分钟内使得PEEP阀输出流量趋于理想状态
// 未完成，待验证
void adjust_peep_valve(void)
{
	// 如果管路脱落，或面罩摘下，则不调整！！！	
	
	// 还有啥时不调整？（叩击时不需要调整）

	// 得到PEEP阀的流量

	// 根据泄漏值（和可能的系统设置溢出流量），计算预期的流量

	// 计算偏差

	// 根据偏差计算需要调整的量

	// 只调整1/8

	// ！设置
}

/**
 * [breath_set_peep_e_direct description]
 * @method breath_set_peep_e_direct
 */
void breath_set_peep_e_direct(void)
{
#if BASE_CONTROL_PEEP_ENABLE
    // 恢复到 PEEP-e
    set_peep_valve_control_val(PID_blower_control_data.Low_peep);
#endif
    set_blower_speed(PID_blower_control_data.Low_speed + PID_blower_control_data.blower_e_pap_adjust_val);
    PID_blower_control_data.adjust_mode = ENUM_PID_FAST_FOR_PRESS;
    main_control_data.status           = EM_BREATH_PRESS_HOLD_LOW;
}

/**
 * [breath_set_peep_i_direct description]
 * @method breath_set_peep_i_direct
 */
void breath_set_peep_i_direct(void)
{
#if BASE_CONTROL_PEEP_ENABLE
    set_peep_valve_control_val(PID_blower_control_data.High_peep);
#endif
    PID_blower_control_data.blower_p_support_adjust_val = 0;
}

/**
 * [breath_waiting_peep_i_ready description]
 * @method breath_waiting_peep_i_ready
 * @return                             [description]
 */
uint8_t breath_waiting_peep_i_ready(void)
{
    // 如果延时不到，则返回
    if(breath_get_time_interval() < PEEP_VALVE_WAITING_TIME)
        return EM_FALSE;
#if BASE_CONTROL_PEEP_ENABLE
    // 恢复到 PEEP-i
    set_peep_valve_control_val(PID_blower_control_data.High_peep);
#endif
    PID_blower_control_data.blower_p_support_adjust_val = 0;
    return EM_TRUE;
}

void breath_continue_with_epap(void)
{
    set_blower_speed(PID_blower_control_data.Low_speed + PID_blower_control_data.blower_e_pap_adjust_val);
    PID_blower_control_data.adjust_mode = ENUM_PID_FAST_FOR_PRESS;
    main_control_data.status        = EM_BREATH_PRESS_HOLD_LOW;
}

/**
 * [breath_waiting_peep_e_ready description]
 * @method breath_waiting_peep_e_ready
 * @return                             [description]
 */
uint8_t breath_waiting_peep_e_ready(void)
{
    // 如果延时不到，则返回
    if(breath_get_time_interval() < PEEP_VALVE_WAITING_TIME)
        return EM_FALSE;

#if BASE_CONTROL_PEEP_ENABLE
    // 恢复到 PEEP-e
    set_peep_valve_control_val(PID_blower_control_data.Low_peep);
#endif
    return EM_TRUE;
}

// 每个厘米水需要调整大约25个点，
/**
 * [adjust_H_peep description]
 * @method adjust_H_peep
 * @param  current_press [description]
 */
void adjust_H_peep(uint16_t current_press) // ipap
{
#if BASE_CONTROL_PEEP_ENABLE
    int32_t adj_val;
    int16_t diff, abs_diff;
    adj_val = PID_blower_control_data.High_peep;
    diff    = main_control_data.ipap - current_press;
    if(diff < 0)
    {
        abs_diff = 0 - diff;
    }
    else
    {
        abs_diff = diff;
    }

    if(abs_diff > 50)
    {
        if(diff > 0)
        {
            PID_blower_control_data.peep_i_pap_adjust_val =  100;
        }
        else
        {
            PID_blower_control_data.peep_i_pap_adjust_val = -100;
        }
    }
    else
    {
        PID_blower_control_data.peep_i_pap_adjust_val = diff * 2;
    }
    adj_val += PID_blower_control_data.peep_i_pap_adjust_val;
    set_peep_valve_control_val((uint16_t)adj_val);
#endif
}

/**
 * [adjust_L_peep description]
 * @method adjust_L_peep
 * @param  current_press [description]
 */
void adjust_L_peep(uint16_t current_press) // epap
{
#if BASE_CONTROL_PEEP_ENABLE
    int32_t adj_val;
    int16_t diff, abs_diff;
    adj_val = PID_blower_control_data.Low_peep;
    diff = main_control_data.epap - current_press;
    if(diff < 0)
    {
        abs_diff = 0 - diff;
    }
    else
    {
        abs_diff = diff;
    }

    if(abs_diff > 50)
    {
        if(diff > 0)
        {
            PID_blower_control_data.peep_e_pap_adjust_val =  100;
        }
        else
        {
            PID_blower_control_data.peep_e_pap_adjust_val = -100;
        }
    }
    else
    {
        PID_blower_control_data.peep_e_pap_adjust_val = diff * 2;
    }
    adj_val += PID_blower_control_data.peep_e_pap_adjust_val;
    set_peep_valve_control_val((uint16_t)adj_val);
#endif
}

#define OVER_FLOW_PRESS_LOW_DEFAULT  1500
#define OVER_FLOW_PRESS_10_DEFAULT   1600
#define OVER_FLOW_PRESS_HIGH_DEFAULT 1700
/**
 * [set_overflow_work_mode description]
 * @method set_overflow_work_mode
 * @param  data                   [description]
 */
void set_overflow_work_mode(uint8_t data)
{
#if BASE_CONTROL_OVERFLOW_ENABLE
    switch (data)
    {
    default:
    case EM_OVERFLOW_VALVE_HALF_OPEN:
        set_overflow_valve(OVER_FLOW_PRESS_10_DEFAULT);
        break;
    case EM_OVERFLOW_VALVE_FULL_CLOSE:
        set_overflow_valve(OVER_FLOW_PRESS_HIGH_DEFAULT);
        break;
    case EM_OVERFLOW_VALVE_FULL_OPEN:
        set_overflow_valve(OVER_FLOW_PRESS_LOW_DEFAULT);
        break;
    case EM_OVERFLOW_VALVE_ADJUST:
        // 应该做些操作
        break;
    }
#endif
}


// 管路脱落时需要特殊处理
// 恢复流量到10L/Min
// 设置成不做调整的模式
/**
 * [set_blower_pid_stop_adjust description]
 * @method set_blower_pid_stop_adjust
 */
void set_blower_pid_stop_adjust(void)
{
    PID_blower_control_data.adjust_mode = ENUM_PID_NO_ADJUST; // （=0 不调整）
    set_blower_speed(PID_blower_control_data.Low_speed); // !!!!暂时定为这个
}

uint8_t get_pid_adjust_mode(void)
{
    return PID_blower_control_data.adjust_mode;
}

/* **************************************
* 患者吸呼气过程的相关补偿函数
* **************************************/
// 启动 压力维持模式（很少有调节的模式）
/**
 * [Blower_start_press_control description]
 * @method Blower_start_press_control
 * @param  press                      [description]
 */
void Blower_start_press_control(uint16_t press)
{
    // 将 PEEP阀设置成需要的状态(依据press需要，初步确认为EPAP/IPAP)
    set_peep_press(press);

    // 将 回流阀设置到初始状态（控制的中间点）
    //set_overflow_work_mode(EM_OVERFLOW_VALVE_HALF_OPEN);

    // 设置涡轮
    set_blower_press(press);

    // 设置blower pid调整模式
    set_PID_blower_adjust_mode(ENUM_PID_FAST_FOR_PRESS);

    // 设置当前工作的状态标记
    //set_PID_blower_control_status(EM_BREATH_PRESS_INCREASE);
}

/**
 * [Blower_start_ipap_control description]
 * @method Blower_start_ipap_control
 */
void Blower_start_ipap_control(void)
{

    // 将 回流阀设置到初始状态（控制的中间点）
    //set_overflow_work_mode(EM_OVERFLOW_VALVE_HALF_OPEN);

    // 将 PEEP阀设置成需要的状态(依据press需要，初步确认为EPAP/IPAP)
    set_peep_press(main_control_data.ipap);

    // 计算PID调整数据
    PID_blower_control_data.High_speed = get_blower_speed_for_press(main_control_data.ipap);

    // 设置涡轮
    set_blower_speed((uint16_t)PID_blower_control_data.High_speed + PID_blower_control_data.blower_i_pap_adjust_val);

    // 设置blower pid调整模式
    set_PID_blower_adjust_mode(ENUM_PID_FAST_FOR_PRESS);

    // 设置当前工作的状态标记
    set_PID_blower_control_status(EM_BREATH_PRESS_HOLD_HIGH);
}

/**
 * [set_default_peep description]
 * @method set_default_peep
 */
void set_default_peep(void)
{
    // 将 PEEP阀设置成需要的状态(依据press需要，初步确认为EPAP/IPAP)
    set_peep_press(main_control_data.epap);
}

/**
 * [Blower_start_pcv_control description]
 * @method Blower_start_pcv_control
 */
void Blower_start_pcv_control(void)
{
    // 将 PEEP阀设置成需要的状态(EPAP)
    //set_peep_press(main_control_data.epap);
    set_peep_valve_control_val(PID_blower_control_data.Low_peep);

    // 计算PID调整数据
    //PID_blower_control_data.Low_speed = get_blower_speed_for_press(main_control_data.epap);

    // 设置blower pid调整模式
    set_PID_blower_adjust_mode(ENUM_PID_FAST_FOR_PRESS);
    // 设置当前工作的状态标记
    set_PID_blower_control_status(EM_BREATH_PRESS_HOLD_LOW);
}

/**
 * [Blower_start_epap_control description]
 * @method Blower_start_epap_control
 */
void Blower_start_epap_control(void)
{
    // 将 回流阀设置到初始状态（控制的中间点）
    //set_overflow_work_mode(EM_OVERFLOW_VALVE_HALF_OPEN);

    // 将 PEEP阀设置成需要的状态(EPAP)
    set_peep_valve_control_val(PID_blower_control_data.Low_peep);
    // 计算PID调整数据
    PID_blower_control_data.Low_speed = get_blower_speed_for_press(main_control_data.epap);

    // 设置涡轮
    set_blower_speed((uint16_t)PID_blower_control_data.Low_speed + PID_blower_control_data.blower_e_pap_adjust_val);

    // 设置blower pid调整模式
    set_PID_blower_adjust_mode(ENUM_PID_FAST_FOR_PRESS);

    // 设置当前工作的状态标记
    set_PID_blower_control_status(EM_BREATH_PRESS_HOLD_LOW);
}

// （患者吸气过程）启动流量补偿的模式，依据流量变化而作相应补偿的过程
// 这个操作需要延时一下，才能完成，我考虑加一个控制标志
/**
 * [Blower_start_flow_compensate_for_inhale description]
 * @method Blower_start_flow_compensate_for_inhale
 */
void Blower_start_flow_compensate_for_inhale(void)
{
    // 如果没有设置，需要一个小小的延时，一旦设置完成，就不需要设置了
    //if(!is_main_base_control_flag_set(EM_I_COMPLETED_FLAGS))
    {
        //if(breath_get_time_interval() < 200) return;

        // 将 PEEP阀设置成需要的状态 （初步建议：比iPAP调高2cmH2O）
        //set_peep_press(main_control_data.ipap+20);
        // 将 回流阀设置到吸气补偿状态（初步建议：控制的最大可控制点）
        //set_overflow_work_mode(EM_OVERFLOW_VALVE_FULL_CLOSE);

        // 设置涡轮
        //set_blower_press(main_control_data.ipap + 20);

        // 设置当前工作的状态标记
        //set_PID_blower_control_status(EM_BREATH_INHALE_SUPPORT);

        // 设置blower pid调整模式
        //set_PID_blower_adjust_mode(ENUM_PID_SUPPORT_FOR_PRESS);
        //set_main_base_control_flag(EM_I_COMPLETED_FLAGS);
    }
}

/**
 * [Blower_start_flow_compensate_for_exhale description]
 * @method Blower_start_flow_compensate_for_exhale
 */
// C-flex（患者呼气过程）启动PEEP及减流量的补偿模式，依据流量变化而作相应补偿的过程
void Blower_start_flow_compensate_for_exhale(void)
{
    // 将 回流阀设置到回流阀最大打开状态（控制的压力最小点）
    set_overflow_work_mode(EM_OVERFLOW_VALVE_FULL_OPEN);

    // 将 PEEP阀设置成需要的状态(依据press需要，初步确认为EPAP/IPAP)
    set_peep_press(main_control_data.c_flex_level);

    // 计算PID调整数据
    //PID_blower_control_data.C_Flex_speed = get_blower_speed_for_press(main_control_data.c_flex_level);

    // 设置涡轮
    set_blower_speed((uint16_t)PID_blower_control_data.C_Flex_speed + PID_blower_control_data.blower_cflex_adjust_val);

    // 设置blower pid调整模式
    set_PID_blower_adjust_mode(ENUM_PID_FAST_FOR_PRESS);

    // 设置当前工作的状态标记
    set_PID_blower_control_status(EM_BREATH_EXHALE_SUPPORT);
}


/* ****************************************************************
   涡轮风机的调整需要考虑流量变化
 * ****************************************************************
 */

extern uint32_t blower_control_idx[];
extern uint32_t blower_control_press_flow_0[];

extern ST_INSERT_DATA blower_flow_0_control_data;

/**
 * [init_blower_control_data_for_flow_0 description]
 * @method init_blower_control_data_for_flow_0
 */
void init_blower_control_data_for_flow_0(void)
{
    init_insert_data(&blower_flow_0_control_data, 34, blower_control_press_flow_0, blower_control_idx, ENUM_DATA_IS_INCREASE);
}

/* ****************************************************************
 * PEEP控制需要重新处理
 * ****************************************************************
 */
/* 参数说明
   press 需要控制的压力
   flow 在满足当前控制压力的同时，满足一定精度的通气流量
   两个参数同时都要满足
   返回 PEEP 阀控制值
 */
/**
 * [get_peep_value_with_press_and_flow description]
 * @method get_peep_value_with_press_and_flow
 * @param  press                              [description]
 * @param  flow                               [description]
 * @return                                    [description]
 */
__weak uint16_t get_peep_value_with_press_and_flow(int16_t press, int32_t flow)
{
    int32_t peep_val;
    double dtmp = 0.0;
#if 0
    // 首先得到参考压力下的值
    peep_val = (int32_t)get_insert_dec_data(flow, (ST_INSERT_DATA *)&peep_valve_control_data);

    // 根据压力偏差再进行修正
    peep_val = peep_val + press * 4 - 550;//1000;//800;
#else
    if(flow < 40000)
    {
        dtmp = (double)(40000 - flow) * 0.0125;
    }
    else
    {
        dtmp = 0.0;
    }

    if(press < 40)
    {
        dtmp += 460;
    }
    else
    {
        dtmp += 460 + (double)(press - 40) * 4.4;
    }
    peep_val = (uint16_t) dtmp + 300;
#endif

    // 可能需要控制最大值
    if(peep_val < PEEP_VALVE_MIN_SET_VALUE)
        peep_val =  PEEP_VALVE_MIN_SET_VALUE;
    if(peep_val > PEEP_VALVE_MAX_SET_VALUE)
        peep_val  = PEEP_VALVE_MAX_SET_VALUE;
    return (uint16_t) peep_val;
}

/* 参数说明
   press 需要控制的压力
   flow 在满足当前控制压力的同时，满足一定精度的通气流量
   两个参数同时都要满足
   返回 Blower 转速
 */
/**
 * [get_blower_speed_for_press_and_flow description]
 * @method get_blower_speed_for_press_and_flow
 * @param  press                               [description]
 * @param  flow                                [description]
 * @return                                     [description]
 */
__weak uint16_t get_blower_speed_for_press_and_flow(int16_t press, int32_t flow)
{
    int32_t blower_val;

    if(flow == 0)
    {
        return (uint16_t) get_insert_inc_data((uint32_t) press, &blower_flow_0_control_data);
    }

    // 计算涡轮风机转速
    blower_val = (int32_t)get_insert_dec_data(flow, (ST_INSERT_DATA *)&blower_press_control_data);
    // 根据压力进行修正
    blower_val = blower_val + press * 65  - 23500;// 100 - 20000;

    // 检测是否溢出
    //    if(blower_val <  MIN_BLOWER_SPEED)
    //        blower_val =  MIN_BLOWER_SPEED;
    //    if(blower_val > MAX_BLOWER_SPEED)
    //        blower_val = MAX_BLOWER_SPEED;
    NO_LESS_THAN(blower_val,  MIN_BLOWER_SPEED);
    NO_MORE_THAN(blower_val,  MAX_BLOWER_SPEED);
    return (uint16_t) blower_val;
}

/* 参数说明
   press 需要控制的压力
   flow 在满足当前控制压力的同时，满足一定精度的通气流量
   两个参数同时都要满足
 */
__weak void set_peep_with_press_and_flow(int16_t press, int32_t flow)
{
    // 设置PEEP
    PID_blower_control_data.base_peep_valve_control_val = get_peep_value_with_press_and_flow(press, flow);
    set_peep_valve_control_val(PID_blower_control_data.base_peep_valve_control_val);
}

/* ********************************************************************************
   回流阀控制需要重新编程
   这个函数要求设置回流阀的控制中间点，在这一点满足一定压力和流量的回流阀设置函数，
   这个函数可能需要和上面的函数联动   才有可能同时满足流量和压力（待验证）
   这个中间点满足回流阀可以快速调整出气口端的（封口/或保障一定流量）压力，（上下调整）
   这个功能完成后，涡轮的设置参数也需要调整
 ********************************************************************************
 */
/* 参数说明
   press 需要控制的压力
   flow 在满足当前控制压力的同时，满足一定精度的通气流量

   两个参数同时都要满足
 */
__weak void set_overflow_valve_press_with_flow(int16_t press, int32_t flow)
{

}

/* ***************************************************************************/
/* ***************************************************************************/
/* ***************************************************************************/
/* ***************************************************************************/
// 负压涡轮控制
/* ***************************************************************************/
/* ***************************************************************************/
/* ***************************************************************************/
/* ***************************************************************************/
// 貌似还是需要之前的函数，把流量（泄漏的影响）添加进来
/**
 * [vacuum_pressure_blower_start description]
 * @method vacuum_pressure_blower_start
 * @param  press                        [description]
 */
__weak void vacuum_pressure_blower_start(int16_t press)
{
}

/**
 * [vacuum_pressure_blower_stop description]
 * @method vacuum_pressure_blower_stop
 */
__weak void vacuum_pressure_blower_stop(void)
{
}

/**
 * [Leak_test_start_epap_control description]
 * @method Leak_test_start_epap_control
 */
void Leak_test_start_epap_control(void)
{
    // 计算PID调整数据
    PID_blower_control_data.Low_speed = get_blower_speed_for_press(main_control_data.epap);

    // 设置涡轮
    set_blower_speed((uint16_t)PID_blower_control_data.Low_speed);

    // 设置blower pid调整模式
    set_PID_blower_adjust_mode(ENUM_PID_FAST_FOR_PRESS);
    // 设置当前工作的状态标记
    set_PID_blower_control_status(EM_BREATH_PRESS_HOLD_LOW);
}

/**
 * [Test_start_press_pid description]
 * @method Test_start_press_pid
 * @param  press                [description]
 */
void Test_start_press_pid(int16_t press)
{
    main_control_data.epap = press;
    // 计算PID调整数据
    PID_blower_control_data.Low_speed = get_blower_speed_for_press(main_control_data.epap);

    // 设置涡轮
    set_blower_speed((uint16_t)PID_blower_control_data.Low_speed);

    // 设置blower pid调整模式
    set_PID_blower_adjust_mode(ENUM_PID_FAST_FOR_PRESS);
    // 设置当前工作的状态标记
    set_PID_blower_control_status(EM_BREATH_PRESS_HOLD_LOW);
}

/**
 * [Test_stop_press_pid description]
 * @method Test_stop_press_pid
 */
void Test_stop_press_pid(void)
{
    set_blower_speed(MIN_BLOWER_SPEED);

    // 设置blower pid调整模式
    set_PID_blower_adjust_mode(ENUM_PID_NO_ADJUST);
    // 设置当前工作的状态标记
    set_PID_blower_control_status(EM_BREATH_STANDBY);
}

/// =======================File: End of base_control.c =============================
