// file: breath_rap.h
#ifndef __BREATH_RAP_H
#define __BREATH_RAP_H

typedef enum
{
    RAP_DMA_SQUARE_WAVE       = 0x0,
    RAP_DMA_SIN_WAVE          = 0x01,
    RAP_DMA_TRIANGLE_WAVE 	  = 0x02,
    RAP_DMA_TRAPEZOID_WAVE    = 0x03
} EM_RAP_DMA_WAVE_TYPES;

typedef enum
{
    EM_RAP_EXPIRE_METHOD  = 0,
    EM_RAP_INSPIRE_METHOD = 1,
    EM_RAP_CONTINUES
} ENUM_RAP_WORK_METHOD_DEFINES;

typedef struct __ST_BREATH_RAP_CONTROL_DATA
{
    // 用于DMA控制及设置
    // 叩击强度及频率相关
    uint32_t time_stamp;
    uint16_t press;
    uint8_t  freq;
    uint8_t  wave_type;
    // 吸气时叩击时间
    uint32_t inhale_work_time;
    // 呼气时叩击时间
    uint32_t exhale_work_time;

} ST_BREATH_RAP_CONTROL_DATA;
extern ST_BREATH_RAP_CONTROL_DATA breath_rap_control_data;

void    set_breath_rap_enable(void);
void    set_breath_rap_disable(void);
void    set_breath_rap_start(void);
void    set_breath_rap_stop(void);
void    set_breath_rap_time_stamp(void);
uint8_t is_rap_time_over(uint8_t type);

uint8_t is_rap_actived(void);// 返回值： = 0 叩击停止  = 1 叩击


void    init_rap_contorl_data(void);// 初始化硬件和数据缓冲区
void    refresh_rap_control_data(uint16_t press, uint8_t freq, uint8_t wave_type);// 重新设置叩击参数 press = 控制压力， freq = 频率
void    do_breath_rap_actions(void);// 直接执行叩击动作的函数（如果是DMA操作，需要定义为空函数）

#endif

// file end

