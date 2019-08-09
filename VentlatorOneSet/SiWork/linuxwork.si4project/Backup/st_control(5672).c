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
#include "pcv_control.h"
#include "st_control.h"
#include "trigger_breath.h"
#include "../alarm/alarm.h"

ST_STV_CONTROL_DAT stv_control_dat;

void copy_st_parameter_to_main_control_data(void)
{
}


void prepare_ST_control_data(void)
{
    // T mode data (PCV -- Pressure Controlled Ventilation)

    // S mode data (Pressure Spontaneous Ventilation)

    // 触发
    trigger_simple_set_base_inhale(1000, main_control_data.epap);
    start_trigger(EM_TRIGGER_TYPE_E_INS);

    // 初始状态
    set_patient_status(EM_PATIENT_INHALE_DETECT);
}


void to_breath_st_mode(void)
{
    ui_set_data.change_parameter_flag = 0;
    // 如果当前工作模式就是ST模式
    if(get_patient_breath_mode() != EM_VENTLATOR_BIPAP_ST_MODE)
    {
        set_patient_breath_mode(EM_VENTLATOR_BIPAP_ST_MODE);
        oxygen_start();
    }

    copy_st_parameter_to_main_control_data();
    prepare_ST_control_data();
}


/**
 * [breath_st_mode description]
 * @method breath_st_mode
 */
void breath_st_mode(void)
{
    switch(global_patient_status)
    {
    case EM_PATIENT_NOT_WORK://standby
        break;
    case EM_PATIENT_BREATH_DETECT:
        break;

    case EM_PATIENT_INHALE_DETECT:
        break;

    case EM_PATIENT_INHALE_START:
        break;

    case EM_PATIENT_INHALE_PERIOD:
        break;

    case EM_PATIENT_BREATH_HOLD:
        break;

    case EM_PATIENT_EXHALE_DETECT:	// 监测呼气触发条件
        break;

    case EM_PATIENT_EXHALE_START:
        break;

    case EM_PATIENT_EXHALE_PERIOD:
        break;

    // 机械通气过程
    case EM_PATIENT_T_INHALE_START:
        // to PCV
        break;

    case EM_PATIENT_T_INHALE_PERIOD:
        // to PCV
        break;

    case EM_PATIENT_T_EXHALE_DETECT:
        // to PCV
        break;

    case EM_PATIENT_T_EXHALE_START:
        // To S-exhale-start
        break;

    // 状态结束标记
    case EM_PATIENT_STATUS_ENDS:
        break;
    default:
        break;
    }

}
