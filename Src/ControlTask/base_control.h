// file base_control.h
#ifndef __BASE_CONTROL_H
#define __BASE_CONTROL_H
#include "stm32f4xx_hal.h"
//#define MAX_BLOWER_SPEED 40000

#define BASE_CONTROL_BLOWER_ENABLE   1
#define BASE_CONTROL_PEEP_ENABLE     1
#define BASE_CONTROL_OVERFLOW_ENABLE 0
#define is_pid_actived() get_pid_adjust_mode()
typedef enum _ENUM_MAIN_BASE_CONTROL_FLAG_TYPES
{
    EM_I_COMPLETED_FLAGS,
    EM_E_COMPLETED_FLAGS
} ENUM_MAIN_BASE_CONTROL_FLAG_TYPES;

typedef enum
{
    EM_OVERFLOW_VALVE_FULL_OPEN  = 0,
    EM_OVERFLOW_VALVE_FULL_CLOSE = 1,
    EM_OVERFLOW_VALVE_HALF_OPEN  = 2,
    EM_OVERFLOW_VALVE_ADJUST     = 3
} __ENUM_OVERFLOW_VALVE_WORK_STATUS;

typedef enum
{
    EM_BASE_SPEED_EPAP,
    EM_BASE_SPEED_IPAP,
    EM_BASE_SPEED_C_FLEX,
    EM_BASE_SPEED_BASE
} __ENUM__BASE_SPEED_TYPES_DEFINED;

typedef enum
{
    ENUM_PID_NO_ADJUST = 0,      // （=0 不调整）
    ENUM_PID_FAST_FOR_PRESS,     // （=1 快速压力调整）
    ENUM_PID_FAST_FOR_FLOW,      // （=2 快速流量调整【吸气口】）
    ENUM_PID_SUPPORT_FOR_PRESS   // （=3 Spontaneous压力支持模式）
    // 未完成的工作： 	ENUM_PID_SUPPORT_FOR_PRESS 需要调整
    // 先测试一下效果，效果令人满意，需要修改CPAP在吸气过程中和呼气过程中的调整，使得最终会有预期的控制效果
} __ENUM_PID_MAIN_CONTROL_ADJUST_MODE_DEFINES;
/* ********************************************************************/
#if 0 // Usage:
switch(PID_blower_control_data.adjust_mode)
{
default:
    break;
case ENUM_PID_NO_ADJUST:	  // （=0 不调整）
    break;
case ENUM_PID_FAST_FOR_PRESS: // （=1 快速压力调整）
    break;
case ENUM_PID_FAST_FOR_FLOW:  // （=2 快速流量调整【吸气口】）
    break;
case ENUM_PID_SUPPORT_FOR_PRESS:
    break;
}
#endif
/* ********************************************************************/

typedef struct __MAIN_BASE_CONTROL_DATA_DEFINES
{
    uint8_t adjust_mode;  // 用于控制涡轮及回气阀 （=0 不调整） （=1 快速压力调整） （=2 快速流量调整【吸气口】）

    // 基本数据 由控制函数来改变
    uint16_t base_speed;   //基本涡轮转速
    uint16_t High_speed;   //IPAP涡轮转速
    uint16_t Low_speed;    //EPAP涡轮转速
    uint16_t C_Flex_speed; //C_Flex涡轮转速

    // 因为V60没有独立设置T模式的吸气压力，这里暂时不用
    uint16_t T_High_speed;

    uint16_t base_peep_valve_control_val;    //基本PEEP阀控制值

    int16_t  base_press;   //基本目标压力 重要  （用于P-support 调整的目标压力）

    //blower涡轮转速补偿数据， 由PID函数来改变
    int16_t  blower_p_support_adjust_val;   // 补偿 Other Press adjust  (如Spontaneous)
    int16_t  blower_i_pap_adjust_val;       // PEEPH时的补偿 BiPAP-IPAP
    int16_t  blower_e_pap_adjust_val;       // PEEPL时的补偿 BiPAP-EPAP
    int16_t  blower_cflex_adjust_val;       // 呼气压力释放时的补偿 BiPAP-CFLEX

#if BASE_CONTROL_OVERFLOW_ENABLE
    // 溢流阀
    int16_t  base_over_flow; // 基本调整数据
    int16_t  overflow_p_support_adjust_val; // Spontaneous 吸气时的补偿
    int16_t  overflow_i_pap_adjust_val;     // 吸气时的补偿
    int16_t  overflow_e_pap_adjust_val;     // 呼气PEEP时的补偿
    int16_t  overflow_cflex_adjust_val;     // 呼气压力释放时的补偿
#endif

#if BASE_CONTROL_PEEP_ENABLE
    // 呼气阀
    int16_t  High_peep;    // IPAP控制
    int16_t  Low_peep;     // 初始PEEP控制(EPAP)
    int16_t  peep_p_support_adjust_val;     // Spontaneous 吸气时的补偿
    int16_t  peep_i_pap_adjust_val;         // 吸气时的补偿
    int16_t  peep_e_pap_adjust_val;         // 呼气PEEP时的补偿
    int16_t  peep_cflex_adjust_val;         // 呼气压力释放时的补偿
#endif
    int16_t temp; // for error write operation
} MAIN_BASE_CONTROL_DATA_DEFINES;
extern MAIN_BASE_CONTROL_DATA_DEFINES PID_blower_control_data;

typedef struct __PWM_CONTROL_DATA_DEFINES
{
    uint16_t pwm_blower_val;
    uint16_t pwm_peep_val;
    uint16_t pwm_oxygen_val;
    uint16_t pwm_vacuum_blower_val;
    uint16_t pwm_flowback_val;
} PWM_CONTROL_DATA_DEFINES;
extern PWM_CONTROL_DATA_DEFINES pwm_control_data;

uint8_t get_pid_adjust_mode(void);

/////////////////////////////////////////////////////////////////////////////////////
void     init_peep_valve_control_data(void);
void     flow_set(void);
void     flow_adjust(void);
void     reset_flow_control_data(void);
void     epap_set(void);
void     epap_adjust(void);
void     reset_epap_control_data(void);
/* **************************************
 * 患者吸呼气过程的相关补偿函数
 * **************************************/
uint16_t get_blower_speed_for_press_and_flow(int16_t press, int32_t flow);
uint16_t get_peep_value_with_press_and_flow(int16_t press, int32_t flow);
void     set_peep_with_press_and_flow(int16_t press, int32_t flow);

void     init_blower_control_data_for_flow_0(void);
void     set_PID_blower_control_base_speed(uint8_t type);
uint16_t get_blower_speed_for_press(int16_t press);
// 启动 压力维持模式（很少有调节的模式）
void     Blower_start_press_control(uint16_t press);
void     Blower_start_ipap_control(void);
void     Blower_start_epap_control(void);
void     reset_PID_blower_control_data(void);

// 新的通气模式
uint16_t get_base_control_high_speed(void);

// （患者吸气过程）启动流量补偿的模式，依据流量变化而作相应补偿的过程
void     Blower_start_flow_compensate_for_inhale(void);

// （患者呼气过程）启动PEEP及减流量的补偿模式，依据流量变化而作相应补偿的过程
void     Blower_start_flow_compensate_for_exhale(void);

void     main_control_pid_blower_adjust(void);
void     set_PID_blower_adjust_mode(uint8_t mode);
void     main_control_direct_blower_adjust(void);


void     set_main_base_control_flag(uint8_t type);
void     reset_main_base_control_flag(uint8_t type);
uint8_t  is_main_base_control_flag_set(uint8_t type);
void     S_T_set_inc_speed(uint16_t rising_time_count, uint16_t blower_increase_speed);
void     Base_set_speed_for_epap(void);
void     Base_set_speed_for_ipap(void);
void     S_T_set_speed_cflex(void);
void     Base_set_speed_T_press(void);

void     reset_P_support_adjust_val(void);
void     set_blower_pid_stop_adjust(void);
void     set_base_speed(uint16_t speed);
void     set_blower_press(uint16_t press);
void     set_peep_press(uint16_t press);
uint8_t  breath_waiting_peep_i_ready(void);
uint8_t  breath_waiting_peep_e_ready(void);
void     set_default_peep(void);
void     open_peep(void);
void     close_peep(void);
void     set_overflow_valve(uint16_t data);

void     Blower_start_pcv_control(void);
void     clear_pid_adjust_val(uint8_t type);
void     PCV_set_inc_speed(uint16_t rising_time_count, uint16_t blower_increase_speed);
void     breath_set_peep_e_direct(void);
void     breath_set_peep_i_direct(void);
void     breath_continue_with_epap(void);

/* ***************************************************************************/
// 负压涡轮控制
/* ***************************************************************************/
void     Cough_increase_pid_base_speed(uint16_t rising_time_count, uint16_t blower_increase_speed);
void     vacuum_pressure_blower_start(int16_t press);
void     vacuum_pressure_blower_stop(void);

/* ***************************************************************************/
// 基本PWM数据控制
/* ***************************************************************************/
void     set_blower_pwm_comtrol_data(uint16_t val);
uint16_t get_blower_pwm_comtrol_data(void);
void     set_peep_pwm_comtrol_data(uint16_t val);
uint16_t get_peep_pwm_comtrol_data(void);
void     set_oxygen_pwm_comtrol_data(uint16_t val);
uint16_t get_oxygen_pwm_comtrol_data(void);
void     set_flowback_pwm_comtrol_data(uint16_t val);
uint16_t get_flowback_pwm_comtrol_data          (void);
void     set_vacuum_blower_pwm_comtrol_data(uint16_t val);
uint16_t get_vacuum_blower_pwm_comtrol_data(void);
#endif
