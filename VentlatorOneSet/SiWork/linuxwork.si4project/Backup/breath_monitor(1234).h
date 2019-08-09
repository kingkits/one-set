// File: breath_monitor.h
#ifndef __BREATH_MONITOR_H
#define __BREATH_MONITOR_H
//#define PATIENT_AUTONOMOUS_RESPIRATION 1  // 有自主呼吸
//#define PATIENT_SPONTANEOUS_RESPIRATION 0 // 无自主呼吸
typedef enum __ENUM_RESPIRATION_TYPES_DEFINED
{
    PATIENT_SPONTANEOUS_RESPIRATION, // 无自主呼吸
    PATIENT_AUTONOMOUS_RESPIRATION   // 有自主呼吸
} ENUM_RESPIRATION_TYPES_DEFINED;
// 用于检测患者呼吸状态的返回标志
typedef enum __ENUM_BREATH_INSPIRE_END_TYPES
{
    EM_BREATH_NOT_FINISHED_SIGNAL,         // 状态未触发
    EM_BREATH_FINISHED_SIGNAL_DETECTED,	   // 检测到触发信号
    EM_BREATH_FAULT_BREATH_STATUS_DETECTED,// 错误的状态（当前的状态不是正常的呼吸状态---可能是个错误触发产生的）
    EM_BREATH_PATIENT_FORCE_TO_BREATH,     // 检测到患者努力做与当前状态相反的努力
    EM_BREATH_PATIENT_TIME_OUT,			   // 通气超时，需要转换通气相（吸呼气转换）
    EM_BREATH_PATIENT_APEA_DETECTD         // 长时间未检测到呼吸
} ENUM_BREATH_INSPIRE_END_TYPES;

typedef enum __ENUM_PATIENT_STATUS
{
    /* *******************************************************************/
    // 非正常状态
    /* *******************************************************************/
    EM_PATIENT_NOT_WORK,       // 待机 standby
    EM_PATIENT_PIPE_OUT,       // 管路处于脱落状态

    /* *******************************************************************/
    // 正常工作状态
    /* *******************************************************************/
    EM_PATIENT_BREATH_DETECT,  // 专门用于测试的状态
    EM_PATIENT_INSPIRE_DETECT, // 监测吸气触发条件
    EM_PATIENT_INSPIRE_START,  // 吸气开始的时刻
    EM_PATIENT_INSPIRE_PERIOD, // 吸气的过程中
    EM_PATIENT_EXPIRE_DETECT,  // 监测呼气触发条件
    EM_PATIENT_BREATH_HOLD,    // 屏气过程（仅容控过程可能使用）
    EM_PATIENT_EXPIRE_START,   // 吸气转呼气的时刻
    EM_PATIENT_EXPIRE_PERIOD,  // 呼气过程

    /* *******************************************************************/
    // 咳痰过程
    /* *******************************************************************/
    EM_PATIENT_COUGH_PREPARE,             // 系统准备阶段
    EM_PATIENT_COUGH_PRESSURE_INCREASE,   // 压力上升阶段
    EM_PATIENT_COUGH_DETECTING_START,     // 咳痰检测过程（hold）
    EM_PATIENT_COUGH_PERIOD,              // 负压咳痰过程
    EM_PATIENT_COUGH_COMPLETE_AND_REPEAT, // 咳痰结束

    /* *******************************************************************/
    // 机械通气过程
    /* *******************************************************************/
    EM_PATIENT_T_INSPIRE_START,
    EM_PATIENT_T_INSPIRE_PERIOD,
    EM_PATIENT_T_EXPIRE_START,
    EM_PATIENT_T_EXPIRE_DETECT,
    EM_PATIENT_T_EXPIRE_PERIOD,
    EM_PATIENT_T_INSPIRE_DETECT,
    /* *******************************************************************/
    // 状态结束标记
    /* *******************************************************************/
    EM_PATIENT_STATUS_ENDS
} ENUM_PATIENT_STATUS_DEFINES;
/*
switch(global_patient_status)
{
	case EM_PATIENT_NOT_WORK://standby
		break;
	case EM_PATIENT_BREATH_DETECT:
		break;
	case EM_PATIENT_INSPIRE_DETECT:
		break;
	case EM_PATIENT_INSPIRE_START:
		break;
	case EM_PATIENT_INSPIRE_PERIOD:
		break;
	case EM_PATIENT_BREATH_HOLD:
		break;
	case EM_PATIENT_EXPIRE_DETECT:  // 监测呼气触发条件
		break;
	case EM_PATIENT_EXPIRE_START:
		break;
	case EM_PATIENT_EXPIRE_PERIOD:
		break;

	////////////////////////////////////////////////////
	// 咳痰过程
    case EM_PATIENT_COUGH_PREPARE:
    	break;
    case EM_PATIENT_COUGH_PRESSURE_INCREASE:
    	break;
    case EM_PATIENT_COUGH_DETECTING_START:
    	break;
    case EM_PATIENT_COUGH_COMPLETE_AND_REPEAT:
    ////////////////////////////////////////////////////
    	break;
    // 机械通气过程
    case EM_PATIENT_T_INSPIRE_START:
    	break;
    case EM_PATIENT_T_INSPIRE_PERIOD:
    	break;
    case EM_PATIENT_T_EXPIRE_PERIOD:
    	break;
    ////////////////////////////////////////////////////
    // 状态结束标记
    case EM_PATIENT_STATUS_ENDS:
    	break;
	default:
		break;
}
 */


uint8_t get_patient_status(void);
void    set_patient_status(uint8_t status);

void breath_monitor_set_time_t(void);

// 检测患者吸气开始
uint8_t check_breath_inspire_start(void);

// 检测患者呼气开始
uint8_t check_breath_expire_start(void);

// 初始化函数
void init_patient_data(void);

// 总的入口函数,所有模式通用
void patient_status_monitor(void);

#endif


