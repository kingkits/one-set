// file: alarm.c
#include "alarm.h"
#include "../global.h"

// 初始化报警相关数据
void init_alarm_data(void)
{

}

__weak void alarm_clear_all(void) {}
// 常规的报警检测
void alarm_detect(void)
{
}

__weak uint8_t is_vacuum_blower_work(void)
{
    return base_board_status.status.cough_system_ready;
}

__weak void set_vacuum_blower_work_status(uint8_t status)
{
    if(status)
        base_board_status.status.cough_system_ready = 1;
    else
        base_board_status.status.cough_system_ready = 0;
}

//返回值： = 0 禁止叩击  = 1 允许叩击
__weak uint8_t is_rap_enable(void)
{
    return base_board_status.status.rap_actived;
}

__weak void set_rap_work_status(uint8_t status)
{
    base_board_status.status.rap_actived = status;
}
void set_nebulizer_work_status(uint8_t status)
{
    if(status)
    {
        base_board_status.status.nebulizer_actived = 1;
    }
    else
    {
        base_board_status.status.nebulizer_actived = 0;
    }
}

int is_breath_nebulizer_turned_on(void)
{
    return base_board_status.status.nebulizer_actived;
}


void set_aic_work_status(uint8_t status)
{
    if(status)
    {
        base_board_status.status.aic_mode_actived = 1;
    }
    else
    {
        base_board_status.status.aic_mode_actived = 0;
    }
}

void set_pipe_out_status(void)
{
    base_board_status.status.pipeout_status = 1;
}
void reset_pipe_out_status(void)
{
    base_board_status.status.pipeout_status = 0;
}

// 低四位有效
void set_work_mode_to_status(uint8_t mode)
{
    base_board_status.status.mode = mode & 0xf;
}

uint8_t get_work_mode_type_for_status(uint8_t mode)
{
    switch(mode)
    {
    case   EM_VENTLATOR_STANDBY_MODE:
        return 0;
    case      EM_VENTLATOR_CPAP_MODE:
        return 1;
    case  EM_VENTLATOR_BIPAP_ST_MODE:
        return 2;
    case EM_VENTLATOR_BIPAP_PCV_MODE:
        return 3;
    default:
        return 0;
    }
}

void get_new_base_board_status(void)
{
    ///////////////////////////
    // byte 1  (bit0-bit7)   //
    ///////////////////////////
    if(get_patient_status() == EM_PATIENT_PIPE_OUT)
    {
        set_pipe_out_status();
    }
    else
    {
        reset_pipe_out_status();
    }


    // mode (bit 4-7)
    switch(get_patient_breath_mode())
    {
    case EM_VENTLATOR_COUGH_MODE:// 将咳痰模式独立出来，进行单独控制
        set_work_mode_to_status(get_work_mode_type_for_status(cough_control_data.prev_work_mode));
        break;
    default:
        set_work_mode_to_status(get_work_mode_type_for_status(get_patient_breath_mode()));
        break;
    }


    ///////////////////////////
    // byte 2  (bit8-bit15)  //
    ///////////////////////////


    ///////////////////////////
    // byte 3  (bit16-bit23) //
    ///////////////////////////


    ///////////////////////////
    // byte 4  (bit24-bit31) //
    ///////////////////////////
}

// End of file: alarm.c

