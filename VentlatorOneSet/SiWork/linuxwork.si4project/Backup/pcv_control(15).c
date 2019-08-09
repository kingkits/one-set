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
case EM_PATIENT_NOT_WORK,       // 待机 standby
         case EM_PATIENT_PIPE_OUT,       // 管路处于脱落状态

                      /* *******************************************************************/
                      // 正常工作状态
                      /* *******************************************************************/
                  case EM_PATIENT_BREATH_DETECT,  // 专门用于测试的状态
                           case EM_PATIENT_INSPIRE_DETECT, // 监测吸气触发条件
                                    case EM_PATIENT_INSPIRE_START,  // 吸气开始的时刻
                                             case EM_PATIENT_INSPIRE_PERIOD, // 吸气的过程中
                                                      case EM_PATIENT_EXPIRE_DETECT,  // 监测呼气触发条件
                                                               case EM_PATIENT_BREATH_HOLD,    // 屏气过程（仅容控过程可能使用）
                                                                        case EM_PATIENT_EXPIRE_START,   // 吸气转呼气的时刻
                                                                                 case EM_PATIENT_EXPIRE_PERIOD,  // 呼气过程

                                                                                              /* *******************************************************************/
                                                                                              // 咳痰过程
                                                                                              /* *******************************************************************/
                                                                                          case EM_PATIENT_COUGH_PREPARE,             // 系统准备阶段
                                                                                                   case EM_PATIENT_COUGH_PRESSURE_INCREASE,   // 压力上升阶段
                                                                                                            case EM_PATIENT_COUGH_DETECTING_START,     // 咳痰检测过程（hold）
                                                                                                                     case EM_PATIENT_COUGH_PERIOD,              // 负压咳痰过程
                                                                                                                              case EM_PATIENT_COUGH_COMPLETE_AND_REPEAT, // 咳痰结束

                                                                                                                                           /* *******************************************************************/
                                                                                                                                           // 机械通气过程
                                                                                                                                           /* *******************************************************************/
                                                                                                                                       case EM_PATIENT_T_INSPIRE_START,
                                                                                                                                                case EM_PATIENT_T_INSPIRE_PERIOD,
                                                                                                                                                         case EM_PATIENT_T_EXPIRE_START,
                                                                                                                                                                  case EM_PATIENT_T_EXPIRE_DETECT,
                                                                                                                                                                           case EM_PATIENT_T_EXPIRE_PERIOD,
                                                                                                                                                                                    case EM_PATIENT_T_INSPIRE_DETECT,
                                                                                                                                                                                                 /* *******************************************************************/
                                                                                                                                                                                                 // 状态结束标记
                                                                                                                                                                                                 /* *******************************************************************/
                                                                                                                                                                                             case EM_PATIENT_STATUS_ENDS:
default:
    break;
}
