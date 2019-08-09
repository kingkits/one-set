//File global.h

#ifndef _GLOBAL_H
#define _GLOBAL_H
#include "stm32f4xx_hal.h"
#include "cmsis_os.h"

#include "stm32f4xx_hal_def.h"
#include "../core/inc/main.h"

#define ZZX_DEBUG 0

#include "ModeControl/breath_monitor.h"
#include "ModeControl/breath_control.h"
#include "ModeControl/Display_data_count.h"
#include "ModeControl/breath_cough_mode.h"
#include "alarm/alarm.h"
#include "GPIO/gpio.h"

// **************
// 可裁剪的功能
// **************
// test
#define USER_TEST_PPFFVV                  0           // Test
#define UART_TXT_COM_ENABLED              0           // Test
#define UART_TEST_VALVE                   0           // Test
#define TEST_LEAK_FLOW                    0           // Test
#define TEST_PEEP_VAL                     1           // Test

// options & set
#define CFLEX_ENABLE_FOR_CPAP             1           // Options: Ventilation-CPAP for parameter C-Flex
#define USE_DMA_CONTROL_RAP               1           // Should be 1
#define PEEP_VALVE_CHANGE_ENABLE          1           // Options: peep-valve hysteresis control
#define USE_SYSTEM_TIMER_COMPARE_FUNCTION 1           // Options: Timer overflow solution
#define USE_MA_TRANSLATOR                 1           // Options: for display-data(press & flow) filtering
#define USE_BREATH_CYCLES_ADJUST          1           // Options: for peep-valve and blower-control auto-adjust
#define BREATH_ENABLE_C_FLEX              1           // Options: for c-flex control
#define USE_STATIC_R_AND_C                1           // Options: =1 For static R&C    =0 For dynamic R&C

#define INVASIVE_VENTLATION_SUPPORT       1           // Options: =1 Invasive is enable  =0 Invasive is disable
#define PATIENT_TYPE_ENABLE               1           // Options: =1 Ventlation support Adult/Children/Infant diffrerent protect


// for press increase control
#define BLOWER_ADJUST_PID_PRESS_INCREASE_ENABLE	   1  // use pid
#define BLOWER_ADJUST_DIRECT_PRESS_INCREASE_ENABLE 1  // direct operation blower
#define OVERFLOW_ADJUST_PRESS_INCREASE_ENABLE      1  // use overflow valve


#define PEEP_VALVE_WAITING_TIME 50   // data-set: peep-valve hysteresis control
#define PEEP_VALVE_MAX_SET_VALUE 3500
#define PEEP_VALVE_MIN_SET_VALUE 460

#define NO_MORE_THAN(value,gate) {if(value>gate)value=gate;}
#define NO_LESS_THAN(value,gate) {if(value<gate)value=gate;}

typedef float float32_t;
typedef void (*VOID_FUN_DEF)(void);


//!!注意：这段RAM空间只能用于CPU操作，任何DMA及硬件都不可以访问
#define CCMRAM  __attribute__((section("ccmram")))

//!!注意：这段RAM空间只用于DMA (R/W)操作，不要定义其他无关的变量到这个RAM段
#define DMARAM  __attribute__((section("dma_ram")))

/* ******************************************************************************
 * ***张.志.新：指定地址定义函数或数据的方法
 * 函数放到 0x20000地址
 * intsqr(int n1) __attribute__((section(".ARM.__at_0x20000")));
 * intsqr(int n1){}
 * 数组放到0x4000地址
 * const uint8 tg[2] __attribute__(at(0X04000))= {	0x00 0x02};
 * ******************************************************************************
 */
typedef enum __ENUM_SYS_WORK_STATUS
{
    EM_OFF,
    EM_ON
} ENUM_SYS_WORK_STATUS;

#define EM_TRUE  1
#define EM_FALSE 0
// *********************************************************
// Valve defines
#define PWM_CHANEL_OXYGEN        TIM_CHANNEL_1
#define PWM_CHANEL_BLOWER        TIM_CHANNEL_2
#define PWM_CHANEL_OVERFLOW      TIM_CHANNEL_3
#define PWM_CHANEL_RAP           TIM_CHANNEL_4
#define PWM_CHANEL_VACUUM_BLOWER TIM_CHANNEL_3

#define MAX_PWM_VAL         4000
#define MAX_PWM_TIME_SLOT   4000

#define DEFAULT_NEBULIZER_FLOW          8500
#define DEFAULT_NEBULIZER_VALVE_CHANNEL 2

#define MIN_BLOWER_SPEED 5000
#define MAX_BLOWER_SPEED 40000

//#define get_1ms_count() ms_1_count
// Timer & counter
extern volatile uint32_t second_count;
extern volatile uint32_t ms_1_count;

// Test flag & value
extern unsigned char     test_flag;
extern uint32_t          test_val;


typedef enum __ENUM_AIR_TYPES
{
    EM_ATP,
    EM_BTPS
} ENUM_AIR_TYPES;
typedef enum __ENUM_PATIENT_TYPES
{
    EM_ADULT,
    EM_CHILDREN,
    EM_INFANT
} ENUM_PATIENT_TYPES;

typedef enum __ENUM_BODY_DISEASE
{
    EM_DISEASE_NORMAL = 0,
    EM_DISEASE_ARDS   = 1,
    EM_DISEASE_COPD   = 2
} ENUM_BODY_DISEASE;
/*
 插管状态：		=0 无创通气/ =1：有创通气
 气体标准：		=0：ATP/		=1：BTPS
 患者类型：		=0：Adult/ 	=1：Children/=2：Infant
 管路连接形式： =0：双管/ 	 =1：单管
 管路顺应性： ml/cmH2O
 症状 		=0: (正常)Normal/=1：（特殊）ARDS/=2：（特殊）COPD/etc.
 */
typedef struct __BASIC_CONTROL_PARAMETERS
{
    //               缺省设置
    uint8_t   invasive_support;        //     插管状态：		=0: 无创通气/ =1：有创通气
    uint8_t   air_type;                //     气体标准：		=0：ATP      / =1：BTPS
    uint8_t   patient_types;           //     患者类型：		=0：Adult    / =1：Children  /=2：Infant
    uint8_t   pipe_connect_type;       // 管路连接形式：=0：双管              / =1：单管
    uint8_t   body_disease;            //         症状：	=0: Normal  / =1：ARDS      /=2：COPD/etc.
    float32_t ambient_pressure;        //     环境压力：(1033.5)(0.1cmH2O)
    float32_t environment_temperiture; //     环境温度： 20℃

    //补偿因子
    float     btps_fact;               // 温湿度补偿因子
    float32_t pipe_fact;               // 管路顺应性：         (0.1)(ml/cmH2O)
} BASIC_CONTROL_PARAMETERS;
extern BASIC_CONTROL_PARAMETERS basic_control_parameters;

typedef struct __TEST_DATA
{
    uint16_t inhale_cycles;
    uint16_t exhale_cycles;
} TEST_DATA_DEFINES;
extern TEST_DATA_DEFINES test_breath_data;

typedef enum _ENUM_ACTIVED_TYPES
{
    EM_INACTIVED,
    EM_ACTIVED
} ENUM_ACTIVED_TYPES;

typedef enum _ENUM_ENABLE_TYPES
{
    EM_DISABLE,
    EM_ENABLE
} ENUM_ENBLE_TYPES;


// data translator types
typedef union _UN_LONG_TO_STRING
{
    int32_t data;
    uint8_t byte[4];
} UN_LONG_TO_STRING_DEFINES;
typedef union _UN_ULONG_TO_STRING
{
    uint32_t data;
    uint8_t  byte[4];
} UN_ULONG_TO_STRING_DEFINES;

typedef union __UN_TRANS_U32_2_BYTE
{
    uint32_t data;
    uint8_t  byte[4];
} UN_TRANS_U32_2_BYTE;

typedef union __UN_TRANS_U16_2_BYTE
{
    uint16_t data;
    uint8_t  byte[2];
} UN_TRANS_U16_2_BYTE;

// fubction in global.c
void     init_global_data(void);
void     inc_1ms_count(void);
uint32_t get_second_count(void);
uint16_t basic_VT_btps_and_pipe_compensation(uint16_t vt, int16_t dp);

void     PWM_change_val(uint32_t chanel, uint32_t val);
void     PWM_start(uint32_t Channel);
void     PWM_stop(uint32_t Channel);
#endif  // #ifndef _GLOBAL_H
// File end


