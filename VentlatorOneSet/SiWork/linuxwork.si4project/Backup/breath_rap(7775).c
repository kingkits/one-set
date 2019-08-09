// file: breath_rap.c
#include "stm32f4xx_hal.h"
#include "../ControlTask/base_control.h"
#include "../global.h"
#include "../blower/blower.h"
#include "../adc/adc_manager.h"
#include "breath_rap.h"
#include "../VoiceCoilMotor/Voice_Coil_Motor.h"
#include "../lib/VentLIB.h"
VOICE_COIL_MOTOR_CONTROL rap_control_data;
ST_BREATH_RAP_CONTROL_DATA breath_rap_control_data;

void set_peep_valve_control_val(uint16_t val);

typedef struct __ST_BREATH_RAP_STATUS
{
    //uint8_t enable_flag;
    uint8_t actived_flag;
} ST_BREATH_RAP_STATUS;

ST_BREATH_RAP_STATUS breath_rap_dstatus;

__weak void rap_dma_start(void)
{

}

__weak void rap_dma_stop(void)
{

}

__weak void set_breath_rap_time_stamp(void)
{
    breath_rap_control_data.time_stamp = ms_1_count;
}

__weak uint8_t is_rap_time_over(uint8_t type)
{
    switch(type)
    {
    case EM_RAP_EXPIRE_METHOD:
#if USE_SYSTEM_TIMER_COMPARE_FUNCTION
        return(system_direct_compare_tm1_be_tm2(ms_1_count, breath_rap_control_data.time_stamp + breath_rap_control_data.expire_work_time));
#else
        if(ms_1_count - breath_rap_control_data.time_stamp > breath_rap_control_data.expire_work_time)// TDB
        {
            return EM_TRUE;
        }
        else
            return EM_FALSE;
#endif
    case EM_RAP_INSPIRE_METHOD:
#if USE_SYSTEM_TIMER_COMPARE_FUNCTION
        return(system_direct_compare_tm1_be_tm2(ms_1_count, breath_rap_control_data.time_stamp + breath_rap_control_data.inhale_work_time));
#else
        if(ms_1_count - breath_rap_control_data.time_stamp > breath_rap_control_data.inhale_work_time)// TDB
        {
            return EM_TRUE;
        }
        else
            return EM_FALSE;
#endif

    case EM_RAP_CONTINUES:
    default:
        break;
    }
    return EM_TRUE;
}

__weak void set_breath_rap_enable(void)
{
    set_rap_work_status(EM_ACTIVED);
}

__weak void set_breath_rap_disable(void)
{
    breath_rap_dstatus.actived_flag  = EM_INACTIVED;
    set_rap_work_status(EM_INACTIVED);
    set_peep_valve_control_val(get_peep_pwm_comtrol_data());
    rap_dma_stop();
}

__weak void set_breath_rap_start(void)
{
    if(is_rap_enable())
    {
        breath_rap_dstatus.actived_flag = EM_ACTIVED;
        rap_dma_start();
        set_breath_rap_time_stamp();
    }
}

__weak void set_breath_rap_stop(void)
{
    if(is_rap_enable())
    {
        breath_rap_dstatus.actived_flag = EM_INACTIVED;
        rap_dma_stop();
        // set PEEP valve to prev-val
        set_peep_valve_control_val(get_peep_pwm_comtrol_data());
    }
}

// 返回值： = 0 禁止叩击  = 1 叩击
__weak uint8_t is_rap_actived(void)
{
    if(is_rap_enable())
    {
        return breath_rap_dstatus.actived_flag;
    }
    return EM_INACTIVED;
}

// 初始化硬件和数据缓冲区
__weak void init_rap_contorl_data(void)
{
#if USE_DMA_CONTROL_RAP
#else
#endif
}

// 重新设置叩击参数 press = 控制压力， freq = 频率
__weak void refresh_rap_control_data(uint16_t press, uint8_t freq, uint8_t wave_type)
{
#if USE_DMA_CONTROL_RAP
#else
#endif
}

// 直接执行叩击动作的函数（如果是DMA操作，需要定义为空函数）
__weak void do_breath_rap_actions(void)
{
#if 0
    // 如果未激活叩击，则返回
    if(breath_rap_dstatus.enable_flag == EM_DISABLE)return;

    // 如果是不能进行叩击的模式 则返回
    switch(get_patient_breath_mode())
    {
    case EM_VENTLATOR_BIPAP_ST_MODE:
        break;
    case EM_VENTLATOR_CPAP_MODE:
        break;
    default:
        // 不能进行叩击控制
        return;
    }
    switch(get_patient_status())
    {
    case EM_PATIENT_NOT_WORK://standby
        // 不能进行叩击控制
        break;
    case EM_PATIENT_BREATH_DETECT:
        // 不能进行叩击控制
        break;
    case EM_PATIENT_INSPIRE_DETECT:
        // 不能进行叩击控制
        break;
    case EM_PATIENT_INSPIRE_START:
        // 不能进行叩击控制
        break;
    case EM_PATIENT_INSPIRE_PERIOD:
        // 不能进行叩击控制
        break;
    case EM_PATIENT_BREATH_HOLD:
        // 叩击
        break;
    case EM_PATIENT_EXPIRE_DETECT:	// 监测呼气触发条件
        // 不能进行叩击控制
        break;
    case EM_PATIENT_EXPIRE_START:
        // 不能进行叩击控制
        break;
    case EM_PATIENT_EXPIRE_PERIOD:
        // 检测叩击结束时间
        break;

    ////////////////////////////////////////////////////
    // 咳痰过程
    case EM_PATIENT_COUGH_PREPARE:
        // 不能进行叩击控制
        break;
    case EM_PATIENT_COUGH_PRESSURE_INCREASE:
        // 不能进行叩击控制
        break;
    case EM_PATIENT_COUGH_DETECTING_START:
        // 不能进行叩击控制
        break;
    case EM_PATIENT_COUGH_COMPLETE_AND_REPEAT:
        // 不能进行叩击控制
        ////////////////////////////////////////////////////
        break;
    // 机械通气过程
    case EM_PATIENT_T_INSPIRE_START:
        // 叩击
        set_breath_rap_start();
        break;
    case EM_PATIENT_T_INSPIRE_PERIOD:
        // 检测叩击结束时间
        break;
    case EM_PATIENT_T_EXPIRE_PERIOD:
        // 检测叩击结束时间
        break;
    ////////////////////////////////////////////////////
    // 状态结束标记
    case EM_PATIENT_STATUS_ENDS:
        break;
    default:
        break;
    }
#else
    if(!breath_rap_dstatus.actived_flag)
    {
        // 叩击未启动
        return;
    }
    // 执行叩击动作
#endif
#if USE_DMA_CONTROL_RAP
#else
#endif
}

void test_rap(void)
{
    breath_rap_control_data.wave_type         =    1;
    breath_rap_control_data.freq              =   10;
    breath_rap_control_data.press             =  100;
    breath_rap_control_data.inhale_work_time = 3000;
    breath_rap_control_data.expire_work_time  = 2000;
    refresh_rap_control_data(breath_rap_control_data.press, breath_rap_control_data.freq, breath_rap_control_data.wave_type);
    set_breath_rap_enable();
}

// file: breath_rap.c ends
