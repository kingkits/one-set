/**
 * File:pcv_control.c
 */

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

/**
 * [prepare_PCV_control_data description]
 * @method prepare_PCV_control_data
 */
void prepare_PCV_control_data(void)
{}

/**
 * [breath_pcv_mode description]
 * @method breath_pcv_mode
 */
void breath_pcv_mode(void)
{
    /* *******************************************************************/
    // 非正常状态
    /* *******************************************************************/
case EM_PATIENT_NOT_WORK:       // 待机 standby
    break;
case EM_PATIENT_PIPE_OUT:       // 管路处于脱落状态
    break;
    /* *******************************************************************/
    // 正常工作状态
    /* *******************************************************************/
case EM_PATIENT_BREATH_DETECT:  // 专门用于测试的状态
    break;
case EM_PATIENT_INSPIRE_DETECT: // 监测吸气触发条件
    break;
    /* *******************************************************************/
    // 机械通气过程
    /* *******************************************************************/
case EM_PATIENT_T_INSPIRE_START:
    break;
case EM_PATIENT_T_INSPIRE_PERIOD:
    break;
case EM_PATIENT_T_EXPIRE_START:
    break;
case EM_PATIENT_T_EXPIRE_DETECT:
    break;
case EM_PATIENT_T_EXPIRE_PERIOD:
    break;
case EM_PATIENT_T_INSPIRE_DETECT:
    break;
    /* *******************************************************************/
    // 状态结束标记
    /* *******************************************************************/
case EM_PATIENT_STATUS_ENDS:
    break;
default:
    break;
}
