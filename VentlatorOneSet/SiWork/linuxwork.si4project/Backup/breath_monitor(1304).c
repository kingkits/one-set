//File:breath_monitor.c
// 用于计算呼吸过程中患者的呼吸状态
#include "stm32f4xx_hal.h"
#include "../adc/adc_manager.h"
#include "../ControlTask/base_control.h"

#include "../global.h"
#include "breath_rap.h"
#include "../comunication/UI_data.h"
#include "../lib/VentLIB.h"
// 注意：所有控制此变量的函数都在本文件里完成
// 外部只有读取状态的功能（控制模式(T/PCV)除外）
static uint8_t global_patient_status;
uint16_t pcv_get_expire_end_time(void);
uint8_t is_Pressure_bt_gate(int16_t pgate);
uint8_t is_Pressure_st_gate(int16_t pgate);
uint8_t is_Flow_bt_gate(int32_t Fgate);
uint8_t is_Flow_st_gate(int32_t Fgate);

void breath_force_operation_nebulizer(uint8_t status);

#define breath_get_expire_peak_Flow() (-display_count_data.Flow)
#define get_current_press() display_count_data.Press
#define breath_get_current_flow() display_count_data.Flow
uint8_t get_patient_status(void)
{
    return global_patient_status;
}

// 控制模式用于强制变更呼吸状态的函数
void set_patient_status(uint8_t status)
{
    global_patient_status = status;
}

static uint32_t time_t = 0;
void breath_monitor_set_time_t(void)
{
    time_t = ms_1_count;
}
// 检测患者吸气开始,此操作只在临近吸气时才可以调用，如果气道压力低于阈值（默认为0，不可以大于0），则界定为吸气开始
uint8_t check_breath_inspire_start(void)
{
    if(time_t > ms_1_count)
        time_t = ms_1_count; // correct errors!
    // 保留200ms
#if USE_SYSTEM_TIMER_COMPARE_FUNCTION
    if(!system_direct_compare_tm1_be_tm2(ms_1_count, time_t + 200))
        return PATIENT_SPONTANEOUS_RESPIRATION;
#else
    if(ms_1_count - time_t < 200)
        return PATIENT_SPONTANEOUS_RESPIRATION; // TDB
#endif
    // 如果有雾化，则需要多延迟一小会儿
    if(is_breath_nebulizer_turned_on())
    {
#if USE_SYSTEM_TIMER_COMPARE_FUNCTION
        if(!system_direct_compare_tm1_be_tm2(ms_1_count, time_t + 400))
            return PATIENT_SPONTANEOUS_RESPIRATION;
#else
        if(ms_1_count - time_t < 400)
            return PATIENT_SPONTANEOUS_RESPIRATION; // TDB
#endif
    }

    //如果吸气压力低于阈值，则吸气开始 ()
    //if(get_current_press() < -20)//if(is_epap_gate_ready(20))//
    if(is_Pressure_st_gate(main_control_data.epap - 20))
    {
        if(is_Flow_bt_gate(3000)) //if(breath_get_current_flow() > 1000) // 增加一个流量限制，以确认不是误触发
        {
            breath_set_inspire_trigger_flag();
            return PATIENT_AUTONOMOUS_RESPIRATION;
        }
    }

    // 如果吸气流量（口端）大于阈值 （默认 6000mL/min） 视为吸气开始
    if(is_Flow_bt_gate(5000))
    {
        // 增加一个压力检测门限
        if(!is_Pressure_st_gate(main_control_data.epap - 10))
            return PATIENT_SPONTANEOUS_RESPIRATION;
        breath_set_inspire_trigger_flag();
        return PATIENT_AUTONOMOUS_RESPIRATION;
    }

    // 其他情况视为未开始
    return PATIENT_SPONTANEOUS_RESPIRATION;
}

// 检测患者吸气开始,此操作只在临近吸气时才可以调用，如果气道压力低于阈值（默认为0，不可以大于0），则界定为吸气开始
uint8_t check_CPAP_breath_inspire_start(void)
{
    //如果吸气压力低于阈值，则吸气开始 ()
    if(is_epap_gate_ready(30))//
    {
        //if(breath_get_current_flow() > 1000) // 增加一个流量限制，以确认不是误触发
        if(is_Flow_bt_gate(2000))
        {
            breath_set_inspire_trigger_flag();
            return PATIENT_AUTONOMOUS_RESPIRATION;
        }
    }
#if USE_SYSTEM_TIMER_COMPARE_FUNCTION
    if(!system_direct_compare_tm1_be_tm2(ms_1_count, time_t + 200))
        return PATIENT_SPONTANEOUS_RESPIRATION;
#else
    if(ms_1_count - time_t < 200)
        return PATIENT_SPONTANEOUS_RESPIRATION; // TDB
#endif

    // 如果吸气流量（口端）大于阈值 （默认 5000mL/min） 视为吸气开始
    if(is_Flow_bt_gate(4000))//if(breath_get_current_flow() > 2000)
    {
        // 增加一个压力检测门限
        if(!is_Pressure_st_gate(main_control_data.epap - 20))
            return PATIENT_SPONTANEOUS_RESPIRATION;

        breath_set_inspire_trigger_flag();
        return PATIENT_AUTONOMOUS_RESPIRATION;
    }

    // 其他情况视为未开始
    return PATIENT_SPONTANEOUS_RESPIRATION;
}

// 是否满足开始检测呼气的条件
uint8_t check_breath_inspire_period(void)
{
    // 满足最小吸气时间 0.2秒
    if(breath_get_time_interval() < 200)
    {
        // 如果很快就没有吸气了，说明是假的呼吸
        if(is_Flow_st_gate(1000)) //if(breath_get_current_flow() < 1000)
            return EM_BREATH_FAULT_BREATH_STATUS_DETECTED;
        return EM_BREATH_NOT_FINISHED_SIGNAL; // 状态未触发
    }
    // 如果流量小于2000 则 结束
    if(is_Flow_st_gate(2000))//if(breath_get_current_flow() < 2000)
    {
        if(get_display_temp_vti() < 30)
            return EM_BREATH_FAULT_BREATH_STATUS_DETECTED; // 非正常呼吸,需要特殊处理
        return EM_BREATH_FINISHED_SIGNAL_DETECTED;
    }
    // 压力大控制压力+2cmH2O 结束
    if(is_Pressure_bt_gate(main_control_data.ipap + 20)) //if(get_current_press() > main_control_data.ipap + 20)
        return EM_BREATH_FINISHED_SIGNAL_DETECTED;

    // CPAP 需要检测特殊状态
    if(breath_get_time_interval() > 20000)
        return EM_BREATH_PATIENT_APEA_DETECTD;

    return EM_BREATH_NOT_FINISHED_SIGNAL;     // 状态未触发
}

// 如果流量小于阈值（流量峰值的百分比） 则结束 BiLevel S/T
uint8_t is_expire_flow_gate_begin(void)
{
    int32_t expire_flow;
    int32_t gate_flow;
    // 得到口端的气体流速
    expire_flow = breath_get_current_flow();

    // 暂定 如果口端流量大于零（患者正在处于吸气阶段） 直接满足条件
    if(expire_flow > 0)
        return EM_TRUE;

    // 取反，求绝对值
    expire_flow = 0 - expire_flow;

    // 计算 门限
    gate_flow = breath_get_expire_peak_Flow() * main_control_data.gate_flow_limited_e / 100;

    if(is_Flow_st_gate(gate_flow)) //if(expire_flow < gate_flow)
        return EM_TRUE;

    // 如果都没有满足，则返回0
    return EM_FALSE;
}

// 是否满足开始检测自主吸气的条件
uint8_t check_breath_expire_period(void)
{
    // 满足最小呼气时间 0.2秒
    if(breath_get_time_interval() < 200)
        return EM_BREATH_NOT_FINISHED_SIGNAL; // 状态未触发

    // 如果呼气流量小于2000 则满足条件 并结束
    if(is_Flow_st_gate(-2000))//if(breath_get_current_flow() > -2000)
    {
        return EM_BREATH_FINISHED_SIGNAL_DETECTED;// 检测到触发信号
    }
    // 压力小于PEEP压力 （- 3cmH2O） 结束 专家的建议是1.5cmH2O
    if(is_Pressure_st_gate(main_control_data.epap - 30)) //if(!is_epap_gate_ready(30))
        return EM_BREATH_FINISHED_SIGNAL_DETECTED; // 检测到触发信号
    return EM_BREATH_NOT_FINISHED_SIGNAL;// 状态未触发
}

// 检测患者呼气开始
// 返回值： = 0 未检测到呼气信号
//          = 1 检测到有主动呼气信号
uint8_t check_breath_expire_start(void)
{
    // 如果呼气峰值流速超过阈值（这个阈值应该可以设置）
    if(is_Flow_st_gate(st_ST_mode_control_data.flow_e_gate))//if(breath_get_expire_peak_Flow() > 3000)
    {
        return PATIENT_AUTONOMOUS_RESPIRATION;
    }

    // 压力超过控制压力 + 2 cmH2O （推荐是1.5）
    //if(get_current_press() > main_control_data.ipap + 50)
    if(is_Pressure_bt_gate(main_control_data.ipap + 50))
    {
        return PATIENT_AUTONOMOUS_RESPIRATION; // 这里还未完成
    }

    return PATIENT_SPONTANEOUS_RESPIRATION;
}

void breath_ST_inspire_monitor_status(void)
{
    // 如果吸气时间超过限定值则呼气开始
    switch(check_breath_inspire_period())
    {
    case EM_BREATH_NOT_FINISHED_SIGNAL:
        break;
    case EM_BREATH_PATIENT_FORCE_TO_BREATH:
        set_patient_status(EM_PATIENT_EXPIRE_DETECT);
        break;
    case EM_BREATH_FINISHED_SIGNAL_DETECTED: // 正常的吸气过程中，检测到吸气完成
        set_patient_status(EM_PATIENT_EXPIRE_DETECT);
        break;
    case EM_BREATH_FAULT_BREATH_STATUS_DETECTED: //非正常的吸气过程--可能是系统的动作导致的误触发
        set_patient_status(EM_PATIENT_BREATH_DETECT);
        break;
    case EM_BREATH_PATIENT_APEA_DETECTD: // 长时间未检测到呼吸
        set_patient_status(EM_PATIENT_BREATH_DETECT);
        break;
    case EM_BREATH_PATIENT_TIME_OUT: // 通气超时，需要转换通气相（吸呼气转换）
        set_patient_status(EM_PATIENT_BREATH_DETECT);
        break;
    default: // 状态未触发
        // 没有检测到吸气完成
        set_patient_status(EM_PATIENT_BREATH_DETECT);
        break;
    }

}

// 初始化函数
void init_patient_data(void)
{
    global_patient_status = EM_PATIENT_NOT_WORK;
}

void breath_monitor_CPAP(void)
{


    switch(global_patient_status)
    {
    case EM_PATIENT_NOT_WORK:   // 未工作
        // 刚刚开始时，强制进入下一工作状态
        // 恢复到EPAP 压力快速调整的模式
        to_default_breath_status();
        break;
    case EM_PATIENT_PIPE_OUT:   // 当管路脱落时，需要检测管路状态
        if(check_pipe_out_status())
            break; //在检测到脱落时，不再进行处理
        // 恢复到EPAP 压力快速调整的模式
        to_default_breath_status();
        break;
    case EM_PATIENT_BREATH_DETECT: // 呼吸检测

        break;
    case EM_PATIENT_INSPIRE_DETECT: // 吸气检测

        // 否则等待下次检测
        break;

    case EM_PATIENT_INSPIRE_START: // 吸气开始
        // 在呼吸（吸气）开始时，需要处理一些事件

        break;

    case EM_PATIENT_INSPIRE_PERIOD: // 吸气过程
        // 要等压力上升完成
        //Blower_start_flow_compensate_for_inspire();// 这个动作应该考虑延迟一小会，以避免误操作（怎么延迟？待确认，我考虑延迟0.1-0.3秒）
        // 吸气过程
        // 如果患者有主动呼气，则转换到呼气开始
        // 如果流量减少到初始流量的阈值，则呼气开始

        break;

    case EM_PATIENT_BREATH_HOLD: // 屏气
        // 仅容量控制使用
        // 等待屏气完成
        set_patient_status(EM_PATIENT_EXPIRE_DETECT);
        break;
    case EM_PATIENT_EXPIRE_DETECT: // 监测呼气触发条件

        // 如果是控制模式则有最大吸气时间的限制，如果不是控制模式，则吸气时间不应该超过30秒
        if(breath_get_time_interval() > 30000)
        {
            //Blower_start_epap_control();
            set_patient_status(EM_PATIENT_BREATH_DETECT);
            break;
        }
        break;
    case EM_PATIENT_EXPIRE_START: // 呼气开始

        break;
    case EM_PATIENT_EXPIRE_PERIOD: // 呼气过程
        // 在呼气时，需要监测的内容
#if 0
        if(check_breath_expire_period())
        {
            // pre-set a flag,用于检测计算PEEP的标记
            monitor_flag = 0;
            display_reset_peep_data();
            set_patient_status(EM_PATIENT_INSPIRE_DETECT);

            // 恢复到EPAP压力,Press keep mode (epap/cpap)
            //Blower_start_epap_control();
            breath_reset_inspire_trigger_flag();
            time_t = ms_1_count;
        }
#else
        switch(check_breath_expire_period())
        {
        case EM_BREATH_NOT_FINISHED_SIGNAL:    // 状态未触发
            //继续等待
            break;
        case EM_BREATH_FINISHED_SIGNAL_DETECTED: // 检测到触发信号
            // pre-set a flag,用于检测计算PEEP的标记
            display_reset_peep_data();
            set_patient_status(EM_PATIENT_INSPIRE_DETECT);

            // 恢复到EPAP压力,Press keep mode (epap/cpap)
            //Blower_start_epap_control();
            breath_reset_inspire_trigger_flag();
            time_t = ms_1_count;
            break;
        case EM_BREATH_PATIENT_TIME_OUT: // 通气超时，需要转换通气相（吸呼气转换）
        case EM_BREATH_PATIENT_APEA_DETECTD: // 长时间未检测到呼吸
        default:          // 其他情况
            set_patient_status(EM_PATIENT_BREATH_DETECT);
            break;
        }
#endif
        break;

    ////////////////////////////////////////////////////
    // 状态结束标记
    case EM_PATIENT_STATUS_ENDS: // 其它
        break;
    default:
        //强制进入下一工作状态
        set_patient_status(EM_PATIENT_BREATH_DETECT);
        break;
    }

}

uint8_t ST_check_breath_inspire_period(void)
{
    // 满足最小吸气时间 0.2秒
    if(breath_get_time_interval() < 200)
    {
        // 如果很快就没有吸气了，说明是假的呼吸 ---- ST模式下似乎要考虑患者呼吸困难的情况 !!!! 需要重新斟酌一下
        //if(breath_get_current_flow() < 1000) return EM_BREATH_FAULT_BREATH_STATUS_DETECTED;  // 错误的状态（当前的状态不是正常的呼吸状态---可能是个错误触发产生的）
        return EM_BREATH_NOT_FINISHED_SIGNAL; // 状态未触发
    }
    // 如果流量小于2000 则 结束
    if(is_Flow_st_gate(2000)) //if(breath_get_current_flow() < 2000)
    {
        //  ---- ST模式下似乎要考虑患者呼吸困难的情况 !!!! 需要重新斟酌一下
        if(get_display_temp_vti() < 30)
            return EM_BREATH_FAULT_BREATH_STATUS_DETECTED; // 错误的状态（当前的状态不是正常的呼吸状态---可能是个错误触发产生的） // 非正常呼吸,需要特殊处理
        else
            return EM_BREATH_FINISHED_SIGNAL_DETECTED; // 检测到触发信号
    }
    // 压力大控制压力+2cmH2O 结束 (检测到患者主动呼气？)
    if(is_Pressure_bt_gate(main_control_data.ipap + 20)) //if(get_current_press() > main_control_data.ipap + 20)
        return EM_BREATH_PATIENT_FORCE_TO_BREATH; // 检测到触发信号

    if(is_ST_breath_expire_start_time())
        return EM_BREATH_FINISHED_SIGNAL_DETECTED; // 检测到触发信号
    else
        return EM_BREATH_NOT_FINISHED_SIGNAL; // 状态未触发
}

uint8_t st_check_breath_expire_period(void)
{
    // 满足最小呼气时间 0.2秒
    if(breath_get_time_interval() < 200)
        return EM_BREATH_NOT_FINISHED_SIGNAL; // 状态未触发

    // time gate
#if USE_SYSTEM_TIMER_COMPARE_FUNCTION
    if(!system_direct_compare_tm1_be_tm2(ms_1_count, ST_get_expire_end_time()))
#else
    if(ms_1_count < ST_get_expire_end_time())
#endif
        return EM_BREATH_FINISHED_SIGNAL_DETECTED; // 检测到触发信号

    // for BiLEVEL/BiPAP
    if(is_expire_flow_gate_begin())
        return EM_BREATH_FINISHED_SIGNAL_DETECTED; // 检测到触发信号

    // 如果呼气流量小于2000 则满足条件 并结束
    if(is_Flow_bt_gate(-2000))//if(breath_get_current_flow() > -2000)
    {
        return EM_BREATH_FINISHED_SIGNAL_DETECTED;// 检测到触发信号
    }
    // 压力小于PEEP压力 （- 3cmH2O） 结束 专家的建议是1.5cmH2O
    if(is_Pressure_bt_gate(main_control_data.epap + 20)) //if(!is_epap_gate_ready(30))
        return EM_BREATH_FINISHED_SIGNAL_DETECTED; // 检测到触发信号
    return EM_BREATH_NOT_FINISHED_SIGNAL;// 状态未触发
}

void st_check_breath_inspire_period(void)
{
    // 吸气过程
    // 如果患者有主动呼气，则转换到呼气开始
    // 如果流量减少到初始流量的阈值，则呼气开始
    // 如果吸气时间超过限定值则呼气开始
    switch(ST_check_breath_inspire_period())
    {
    case EM_BREATH_NOT_FINISHED_SIGNAL:  // 吸气状态未完成
        // 继续等待
        break;

    case EM_BREATH_FINISHED_SIGNAL_DETECTED:// 正常的吸气过程中，检测到吸气完成
        set_patient_status(EM_PATIENT_EXPIRE_DETECT);
        break;

    // 错误的状态（当前的状态不是正常的呼吸状态---可能是个错误触发产生的）
    case EM_BREATH_FAULT_BREATH_STATUS_DETECTED: //非正常的吸气过程--可能是系统的动作导致的误触发
        to_default_breath_status();
        break;

    // 检测到患者努力做与当前状态相反的努力
    case EM_BREATH_PATIENT_FORCE_TO_BREATH:// 正常的吸气过程中，检测到患者呼气动作，需要提前
        // PEEP release mode （C-FLEX）
        Blower_start_flow_compensate_for_expire();
        to_default_breath_status();

        set_patient_status(EM_PATIENT_EXPIRE_DETECT);
        break;
    //case EM_BREATH_PATIENT_TIME_OUT:       // 通气超时，需要转换通气相（吸呼气转换）
    // 由 breath_control.c 完成
    //case EM_BREATH_PATIENT_APEA_DETECTD:		   // 长时间未检测到呼吸
    // 在ST模式下不会出现这个状态（因为T模式这个条件）
    default://这是个错误的情况，需要恢复到初始状态
        to_default_breath_status();
        break;
    }
}

void breath_monitor_ST(void)
{
    static int monitor_flag = 0;

    switch(global_patient_status)
    {
    case EM_PATIENT_NOT_WORK:     // 未工作
        // 恢复到EPAP 压力快速调整的模式
        to_default_breath_status();
        break;

    case EM_PATIENT_PIPE_OUT:     // 当管路脱落时，需要检测管路状态
        if(check_pipe_out_status())
            break; //在检测到脱落时，不再进行处理

        // 恢复到EPAP 压力快速调整的模式
        to_default_breath_status();
        break;

    case EM_PATIENT_BREATH_DETECT: // 呼吸检测
        // 如果是在叩击工作模式，那么就不能进行进入下一工作状态的检测


        break;
    case EM_PATIENT_INSPIRE_DETECT: // 吸气检测
        // 如果是在叩击工作模式，那么就不能进行进入下一工作状态的检测

        // 首先要检测是不是出现了呼气结束信号---（非常重要）这时还没有吸气动作
        if(monitor_flag == 0)
        {
            if(monitor_expire_end())
            {
                // 告诉系统，现在开始计算PEEP
                set_peep_start_flag();
                breath_set_expire_trigger_flag(); // 这个主动呼气的标记暂时没有用

                monitor_flag = 1; // 避免重复执行上面的操作
            }
            break;
        }


        // 否则等待下次检测
        break;
    case EM_PATIENT_INSPIRE_START: // 吸气开始
        // 在呼吸（吸气）开始时，需要处理一些事件
        break;
    case EM_PATIENT_INSPIRE_PERIOD: // 吸气过程

        break;
    case EM_PATIENT_BREATH_HOLD:  // 屏气
        // 仅容量控制使用
        // 等待屏气完成
        set_patient_status(EM_PATIENT_EXPIRE_DETECT);
        break;
    case EM_PATIENT_EXPIRE_DETECT: // 监测呼气触发条件
        // 如果是在叩击工作模式，那么就不能进行进入下一工作状态的检测

        // ****** 如果长时间（60）检测不到吸气或呼气，则视为呼吸停止，需要强制回到EM_PATIENT_BREATH_DETECT状态中
        break;
    case EM_PATIENT_EXPIRE_START: // 呼气开始

        break;
    case EM_PATIENT_EXPIRE_PERIOD: // 呼气过程
        // 如果是在叩击工作模式，那么就不能进行进入下一工作状态的检测
        if(is_rap_enable())
            break;

        // 在呼气时，需要监测的内容
        switch(st_check_breath_expire_period())
        {
        case EM_BREATH_NOT_FINISHED_SIGNAL: // 状态未触发
            break; // zzx 2019-3-4 append for do nothing

        case EM_BREATH_PATIENT_FORCE_TO_BREATH: // 检测到患者努力做与当前状态相反的努力
        // 与下面的操作相同
        case EM_BREATH_FINISHED_SIGNAL_DETECTED: // 检测到触发信号
            // pre-set a flag,用于检测计算PEEP的标记
            monitor_flag = 0;
            display_reset_peep_data();
            to_default_breath_status();
            set_patient_status(EM_PATIENT_INSPIRE_DETECT);

            breath_reset_inspire_trigger_flag();
            time_t = ms_1_count;
            break;
        case EM_BREATH_FAULT_BREATH_STATUS_DETECTED:// 错误的状态（当前的状态不是正常的呼吸状态---可能是个错误触发产生的）
            to_default_breath_status();
            break;

        default:
            to_default_breath_status();
            break;
        }
        break;
    ////////////////////////////////////////////////////
    // 机械通气过程
    case EM_PATIENT_T_INSPIRE_START: // T吸气开始
        // 在 breath_control.c 中处理
        break;
    case EM_PATIENT_T_INSPIRE_PERIOD: // T吸气过程
        // 在 breath_control.c 中处理
        break;
    case EM_PATIENT_T_EXPIRE_PERIOD: // T呼气过程
        break;
    case EM_PATIENT_T_EXPIRE_DETECT: // T呼气检测
        // 在 breath_control.c 中处理
        break;

    ////////////////////////////////////////////////////
    // 状态结束标记
    case EM_PATIENT_STATUS_ENDS:  // 其它
        break;
    default:
        break;
    }
}

// ***********************************************************
// **************************************PCV mode
// ***********************************************************

uint8_t PCV_check_breath_expire_period(void)
{
    // 满足最小呼气时间
#if USE_SYSTEM_TIMER_COMPARE_FUNCTION
    if(system_direct_compare_tm1_be_tm2(ms_1_count, pcv_get_expire_end_time()))
#else
    if(ms_1_count >= pcv_get_expire_end_time())
#endif
        return EM_BREATH_NOT_FINISHED_SIGNAL; // 状态未触发
    else
        return EM_BREATH_FINISHED_SIGNAL_DETECTED; // 状态触发
}

//查看是否到了呼气结束
uint8_t pcv_monitor_expire_end(void)
{
    // 呼气流速小于500ml/Min 则认为呼气结束
    if(breath_get_current_flow() > -500)
    {
        return EM_TRUE;
    }
    // 如果呼气超过15秒 则认为呼气结束
    if(breath_get_time_interval() > pcv_get_inspire_end_time())
    {
        return EM_TRUE;
    }
    // 否则未结束
    return EM_FALSE;
}

void breath_monitor_PCV(void)
{
    static int monitor_flag = 0;
    switch(global_patient_status)
    {
    case EM_PATIENT_PIPE_OUT: // 当管路脱落时，需要检测管路状态
        if(check_pipe_out_status())
            break; //在检测到脱落时，不再进行处理

        // 恢复到EPAP 压力快速调整的模式
        Blower_start_epap_control(); // 使用EPAP调整模式

        // 似乎这里有些霸道了，应该先等等患者能否有吸气动作
        set_patient_status(EM_PATIENT_T_INSPIRE_START); // 强制进入吸气模式？
        break;

    case EM_PATIENT_NOT_WORK:     // 未工作
        // 似乎这里有些霸道了，应该先等等患者能否有吸气动作
        set_patient_status(EM_PATIENT_T_INSPIRE_START);
        break;

    case EM_PATIENT_EXPIRE_START: // 呼气开始
        // 在 breath_control.c 中处理
        break;

    case EM_PATIENT_T_INSPIRE_START: // T吸气开始
        // 在 breath_control.c 中处理
        break;

    case EM_PATIENT_T_INSPIRE_PERIOD: // T吸气过程
        // 在 breath_control.c 中处理
        break;

    case EM_PATIENT_T_EXPIRE_PERIOD: // T呼气过程
        // 在 breath_control.c 中处理
        monitor_flag = 0;
        time_t = ms_1_count;
        break;

    case EM_PATIENT_T_EXPIRE_DETECT: // T呼气检测
        // 在 breath_control.c 中处理
        break;

    case EM_PATIENT_T_INSPIRE_DETECT: // T吸气检测
        // 其它的在 breath_control.c 中处理

        // 这里处理PEEP的计算
        if(monitor_flag == 0)
        {
            if(pcv_monitor_expire_end())
            {
                // 告诉系统，现在开始计算PEEP
                set_peep_start_flag();
                breath_set_expire_trigger_flag(); // 这个主动呼气的标记暂时没有用

                monitor_flag = 1; // 避免重复执行上面的操作
            }
            break;
        }

        // 如果检测到患者吸气的动作，则开始吸气

        break;

    ////////////////////////////////////////////////////
    // 状态结束标记
    default:
        break;
    }

}

// 总的入口函数,所有模式通用
// 感觉应该按照不同模式进行区分会比较合理一些
//
void patient_status_monitor(void)
{
    static uint8_t last_status = 0;
    static uint32_t drop_count = 0;
    // 检测管路脱落状态
    if(global_patient_status != EM_PATIENT_PIPE_OUT)
    {
        if(check_pipe_out_status())
        {
            // stop Atomizer
            breath_force_operation_nebulizer(EM_OFF);

            // stop blower PID
            set_blower_pid_stop_adjust();

            // 要不要停止混氧？

            global_patient_status = EM_PATIENT_PIPE_OUT;
        }
    }

    // 如果是排痰模式，则返回
    if(get_patient_breath_mode() == EM_VENTLATOR_COUGH_MODE)
        return;


    // 超时的时候（15秒），转到初始检测
    if((global_patient_status > EM_PATIENT_BREATH_DETECT))
    {
        if(last_status != global_patient_status)
        {
            last_status = global_patient_status;
            drop_count = ms_1_count;
        }

        //if(breath_get_time_interval() > 15000)
#if USE_SYSTEM_TIMER_COMPARE_FUNCTION
        if(system_direct_compare_tm1_be_tm2(ms_1_count, drop_count + 15000))
#else
        if(ms_1_count - drop_count > 15000)// TDB
#endif
        {
            display_count_leak();
            // 重新设置标记
            drop_count = ms_1_count;
            global_patient_status = EM_PATIENT_NOT_WORK;
        }
    }



    switch(get_patient_breath_mode())
    {
    case EM_VENTLATOR_STANDBY_MODE:
        //breath_monitor_CPAP();
        break;

    //基本模式
    case EM_VENTLATOR_CPAP_MODE:
        if(is_rap_actived())
            break;
        breath_monitor_CPAP();
        break;

    // 高级模式
    case EM_VENTLATOR_PAV_MODE:

        break;

    //兼容 PHILIPS 伟康 V60
    case EM_VENTLATOR_BIPAP_ST_MODE:
        breath_monitor_ST();
        break;
    case EM_VENTLATOR_BIPAP_PCV_MODE:
        breath_monitor_PCV();
        break;

    // 伟康高级模式，不可用

    //兼容五合一的模式 VOCSN
    case EM_VENTLATOR_BiLEVEL_MODE:
        break;
    case EM_VENTLATOR_SPONTANEOUS_MODE:
        break;
    case EM_VENTLATOR_AC_P_MODE:
        break;
    case EM_VENTLATOR_SIMV_PCV_MODE:
        break;
    case EM_VENTLATOR_AC_V_MODE:
        break;
    case EM_VENTLATOR_SIMV_VCV_MODE:
        break;
    default:
        break;
    }

}

// end of file breath_monitor.c
