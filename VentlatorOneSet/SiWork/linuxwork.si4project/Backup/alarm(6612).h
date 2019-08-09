// file: alarm.h

#include "stm32f4xx_hal.h"

#ifndef __ALARM_H
#define __ALARM_H
typedef struct __ST_base_board_status
{
    uint32_t pipeout_status     : 1; // 管路连接状态 = 0 正常                     = 1 管路脱落
    uint32_t cough_system_ready : 1; // 咳痰装置状态 = 0 咳痰装置未准备好 = 1 咳痰装置准备好
    uint32_t one_pipe_mode      : 1; // 单管模式         = 0 正常               = 1 单管模式
    uint32_t no_breath_checked  : 1; // 长时间未呼吸 = 0 正常                     = 1 没有呼吸
    uint32_t mode               : 4; // 当前工作模式 = 0 Standby                = 1 CPAP          = 2 ST       =3 PCV      * = 4 AIC
    uint32_t aic_mode_actived   : 1; // AIC 工作状态 = 0 AIC 停止工作     = 1 AIC 正在工作
    uint32_t nebulizer_actived  : 1; // 雾化器状态        = 0 雾化器停止工作          = 1 雾化器正在工作
    uint32_t rap_actived        : 1; // 叩击工作状态 = 0 rap停止工作                = 1 rap正在工作
    uint32_t unused             : 21;
} ST_base_board_status;

typedef union
{
    ST_base_board_status status;
    uint8_t bytes[4];
} UNION_BASE_BOARD_STATUS;
extern UNION_BASE_BOARD_STATUS base_board_status;
typedef struct __ST_ALARM_INFO
{
    // sensor
    uint32_t sensor_oxygen     : 1; // 0 --> OK  1--> fault
    uint32_t flow_insp_sensor  : 1; // 0 --> OK  1--> fault
    uint32_t flow_exp_sensor   : 1; // 0 --> OK  1--> fault
    uint32_t press_insp_sensor : 1; // 0 --> OK  1--> fault
    uint32_t press_exp_sensor  : 1; // 0 --> OK  1--> fault
    uint32_t press_cough_sensor: 1; // 0 --> OK  1--> fault

    // valve


    // other
    uint32_t oxygen_supply     : 1; // 0 --> OK  1--> fault
    uint32_t apnea             : 2; // 0 --> OK  1--> Low actived breath 2--> apnea 3--> TBD

    // TBD
    uint32_t unused  : 22;
} ST_ALARM_INFO;

void    alarm_clear_all(void);
void    get_new_base_board_status(void);
int     is_breath_nebulizer_turned_on(void);
uint8_t is_rap_enable(void); // 返回值： = 0 禁止叩击  = 1 允许叩击
uint8_t is_vacuum_blower_work(void);
void    reset_pipe_out_status(void);
void    set_aic_work_status(uint8_t status);
void    set_pipe_out_status(void);
void    set_nebulizer_work_status(uint8_t status);
void    set_rap_work_status(uint8_t status);
void    set_vacuum_blower_work_status(uint8_t status);
void alarm_detect_inhale_period(void);
void alarm_detect_exhale_period(void);

#endif
// End of file:alarm.h
