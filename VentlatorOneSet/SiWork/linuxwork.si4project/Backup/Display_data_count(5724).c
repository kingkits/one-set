// File: display_data_count.c
#include "../global.h"
#include "breath_rap.h"
#include "../adc/adc_manager.h"
#include "../lib/MovingAverage.h"
#include "../lib/leak_flow_count.h"

#include "alpha_beta_filter.h"


uint8_t is_nebulizer_valve_open(void);

void set_one_cycle_complete_flag(void);

// for dsaplay wave data
#define MA_DISPLAY_FLOW_BUF_LEN   32
#define PRESSURE_COMPARE_GATE_NUM 24 // MA_DISPLAY_FLOW_BUF_LEN * 3 / 4
#define FLOW_COMPARE_GATE_NUM     24 // MA_DISPLAY_FLOW_BUF_LEN * 3 / 4
ST_MOVING_AVERAGE_DATA_S32 MA_display_flow;
ST_MOVING_AVERAGE_DATA_S32 MA_display_press;

int32_t MA_display_flow_buf[MA_DISPLAY_FLOW_BUF_LEN];
int32_t MA_display_press_buf[MA_DISPLAY_FLOW_BUF_LEN];

ST_ALPHA_BETA_FILTER_DATA alpha_beta_filter_flow;
ST_ALPHA_BETA_FILTER_DATA alpha_beta_filter_press;

/**
 * [is_second_vt_start description]
 * @method is_second_vt_start
 * @return [description]
 */
#define is_second_vt_start() (display_temp_data.vt_second_active_flag == 1)

/**
 * [set_system_breath_leak_level description]
 * @method set_system_breath_leak_level
 * @param  leak                         [description]
 */
void set_system_breath_leak_level(int32_t leak)
{
    if(leak < 5000)
        system_breath_leak_level = BREATH_LEAK_PERFACT; // 泄漏 <5 L/min
    else if(leak < 10000)
        system_breath_leak_level = BREATH_LEAK_GOOD;    // 泄漏 <10 L/min
    else if(leak < 30000)
        system_breath_leak_level = BREATH_LEAK_NORMAL;  // 泄漏 <30 L/min
    else if(leak < 60000)
        system_breath_leak_level = BREATH_LEAK_BAD;     // 泄漏 <60 L/min
    else if(leak < 80000)
        system_breath_leak_level = BREATH_LEAK_WORSE;   // 泄漏 <80 L/min
    else if(leak < 120000)
        system_breath_leak_level = BREATH_LEAK_WORST;   // 泄漏 <120 L/min
    else
        system_breath_leak_level = BREATH_LEAK_PIP_OUT;  // >120 L/min
}

/**
 * [correct_peep_for_leak description]
 * @method correct_peep_for_leak
 */
// 不考虑压力的影响，只考虑泄漏漏量及PeeP阀的流量
void correct_peep_for_leak(void)
{
    // 在出现大泄漏的情况下，要考虑减小PEEP阀的开度，为保障安全，应该强制给一个最小的安全开度
    // Peep阀开度需要提前定义
    switch (system_breath_leak_level)
    {
    case BREATH_LEAK_PERFACT: // 泄漏 <5 L/min
        break;

    case BREATH_LEAK_GOOD:    // 泄漏 <10 L/min
        break;

    case BREATH_LEAK_NORMAL:  // 泄漏 <30 L/min
        break;

    case BREATH_LEAK_BAD:     // 泄漏 <60 L/min
        break;

    case BREATH_LEAK_WORSE:   // 泄漏 <80 L/min
        break;

    case BREATH_LEAK_WORST:    // 泄漏 <100 L/min
        break;

    case BREATH_LEAK_PIP_OUT:  // >120 L/min
        break;
    default:
        break;
    }
}


/**
 * [reset_alpha_beta_filter_data description]
 * @method reset_alpha_beta_filter_data
 */
void reset_alpha_beta_filter_data(void)
{
    alpha_beta_filter_clear_data((ST_ALPHA_BETA_FILTER_DATA *) &alpha_beta_filter_flow);
    alpha_beta_filter_clear_data((ST_ALPHA_BETA_FILTER_DATA *) &alpha_beta_filter_press);
}

/**
 * [init_display_press_flow_MA_buf description]
 * @method init_display_press_flow_MA_buf
 */
void init_display_press_flow_MA_buf(void)
{
    init_moving_average_data_s32(&MA_display_flow,   MA_DISPLAY_FLOW_BUF_LEN, MA_display_flow_buf);
    init_moving_average_data_s32(&MA_display_press,  MA_DISPLAY_FLOW_BUF_LEN, MA_display_press_buf);
}

/**
 * [get_display_flow description]
 * @method get_display_flow
 * @return                  [description]
 */
int32_t get_display_flow(void)
{
    return get_moving_average_data_s32(&MA_display_flow);
}

/**
 * [get_display_press description]
 * @method get_display_press
 * @return                   [description]
 */
int32_t get_display_press(void)
{
    return get_moving_average_data_s32(&MA_display_press);
}

#if 0
//unsigned char is_Move_Average_big_than_gate(ST_MOVING_AVERAGE_DATA_S32 *ma_ptr, int gate_val, unsigned char gate_num)
//{
//	int i;
//	int count;
//	count = 0;
//	i = ma_ptr->data_ptr;
//	if(i == 0) i = ma_ptr->len-1;
//	else i--;
//	while(i != ma_ptr->data_ptr)
//	{
//		if((ma_ptr->data)[i] > gate_val)
//		{
//			count++;
//			if(count >= gate_num) return 1;
//		}
//		if(i == 0) i = ma_ptr->len-1;
//		else i--;
//	}
//	return 0;
//}

//unsigned char is_Move_Average_small_than_gate(ST_MOVING_AVERAGE_DATA_S32 *ma_ptr, int gate_val, unsigned char gate_num)
//{
//	int i;
//	int count;
//	count = 0;
//	i = ma_ptr->data_ptr;
//	if(i == 0) i = ma_ptr->len-1;
//	else i--;
//	while(i != ma_ptr->data_ptr)
//	{
//		if((ma_ptr->data)[i] < gate_val)
//		{
//			count++;
//			if(count >= gate_num) return 1;
//		}
//		if(i == 0) i = ma_ptr->len-1;
//		else i--;
//	}
//	return 0;
//}
#endif

#if 0
/**
 * [is_Flow_bt_gate description]
 * @method is_Flow_bt_gate
 * @param  Fgate           [description]
 * @return                 [description]
 */
uint8_t is_Flow_bt_gate(int32_t Fgate)
{
    int i;
    int count;

    count = 0;
    for(i = 0; i < MA_DISPLAY_FLOW_BUF_LEN; i++)
    {
        if(MA_display_flow.data[i] > Fgate) count++;
    }

    if(count >= PRESSURE_COMPARE_GATE_NUM) return EM_TRUE;
    else return EM_FALSE;
}

/**
 * [is_Flow_st_gate description]
 * @method is_Flow_st_gate
 * @param  Fgate           [description]
 * @return                 [description]
 */
uint8_t is_Flow_st_gate(int32_t Fgate)
{
    int i;
    int count;

    count = 0;
    for(i = 0; i < MA_DISPLAY_FLOW_BUF_LEN; i++)
    {
        if(MA_display_flow.data[i] < Fgate) count++;
    }

    if(count >= PRESSURE_COMPARE_GATE_NUM) return EM_TRUE;
    else return EM_FALSE;
}

/**
 * [is_Pressure_bt_gate description]
 * @method is_Pressure_bt_gate
 * @param  pgate               [description]
 * @return                     [description]
 */
// 当连续压力采样超过（大于）门限压力时，返回 TRUE，否则返回 FALSE
uint8_t is_Pressure_bt_gate(int16_t pgate)
{
    int i;
    int count;

    count = 0;
    for(i = 0; i < MA_DISPLAY_FLOW_BUF_LEN; i++)
    {
        if(MA_display_press.data[i] > pgate) count++;
    }

    if(count >= PRESSURE_COMPARE_GATE_NUM) return EM_TRUE;
    else return EM_FALSE;
}

/**
 * [is_Pressure_st_gate description]
 * @method is_Pressure_st_gate
 * @param  pgate               [description]
 * @return                     [description]
 */
// 当连续压力采样超过（小于）门限压力时，返回 TRUE，否则返回 FALSE
uint8_t is_Pressure_st_gate(int16_t pgate)
{
    int i;
    int count;

    count = 0;
    for(i = 0; i < MA_DISPLAY_FLOW_BUF_LEN; i++)
    {
        if(MA_display_press.data[i] < pgate) count++;
    }

    if(count >= PRESSURE_COMPARE_GATE_NUM) return EM_TRUE;
    else return EM_FALSE;
}

#else
/**
 * [is_Flow_bt_gate description]
 * @method is_Flow_bt_gate
 * @param  Fgate           [description]
 * @return                 [description]
 */
uint8_t is_Flow_bt_gate(int32_t Fgate)
{
    return is_Move_Average_big_than_gate(&MA_display_flow, Fgate, PRESSURE_COMPARE_GATE_NUM);
}

/**
 * [is_Flow_st_gate description]
 * @method is_Flow_st_gate
 * @param  Fgate           [description]
 * @return                 [description]
 */
uint8_t is_Flow_st_gate(int32_t Fgate)
{
    return is_Move_Average_small_than_gate(&MA_display_flow, Fgate, PRESSURE_COMPARE_GATE_NUM);
}

/**
 * [is_Pressure_bt_gate description]
 * @method is_Pressure_bt_gate
 * @param  pgate               [description]
 * @return                     [description]
 */
// 当连续压力采样超过（大于）门限压力时，返回 TRUE，否则返回 FALSE
uint8_t is_Pressure_bt_gate(int16_t pgate)
{
    return is_Move_Average_big_than_gate(&MA_display_press, pgate, PRESSURE_COMPARE_GATE_NUM);
}

/**
 * [is_Pressure_st_gate description]
 * @method is_Pressure_st_gate
 * @param  pgate               [description]
 * @return                     [description]
 */
// 当连续压力采样超过（小于）门限压力时，返回 TRUE，否则返回 FALSE
uint8_t is_Pressure_st_gate(int16_t pgate)
{
    return is_Move_Average_small_than_gate(&MA_display_press, pgate, PRESSURE_COMPARE_GATE_NUM);
}

#endif

#if 0
/**
 * [clear_vti_count_flag description]
 * @method clear_vti_count_flag
 */
void clear_vti_count_flag(void)
{
    display_temp_data.vti_count_flag = 0;
}

/**
 * [set_vti_count_flag description]
 * @method set_vti_count_flag
 */
void set_vti_count_flag(void)
{
    display_temp_data.vti_count_flag = 1;
}

/**
 * [clear_vte_count_flag description]
 * @method clear_vte_count_flag
 */
void clear_vte_count_flag(void)
{
    display_temp_data.vte_count_flag = 0;
}

/**
 * [set_vte_count_flag description]
 * @method set_vte_count_flag
 */
void set_vte_count_flag(void)
{
    display_temp_data.vte_count_flag = 1;
}

/**
 * [is_vti_counted description]
 * @method is_vti_counted
 * @return                [description]
 */
uint8_t is_vti_counted(void)
{
    return display_temp_data.vti_count_flag == 1;
}

/**
 * [is_vte_counted description]
 * @method is_vte_counted
 * @return                [description]
 */
uint8_t is_vte_counted(void)
{
    return display_temp_data.vte_count_flag == 1;
}

#else
#define clear_vti_count_flag() (display_temp_data.vti_count_flag = 0)
#define set_vti_count_flag()   (display_temp_data.vti_count_flag = 1)
#define clear_vte_count_flag() (display_temp_data.vte_count_flag = 0)
#define set_vte_count_flag()   (display_temp_data.vte_count_flag = 1)
#define is_vti_counted()       (display_temp_data.vti_count_flag == 1)
#define is_vte_counted()       (display_temp_data.vte_count_flag == 1)
#endif
/* ******************************************
   系统复位时的初始化
* ******************************************/
/**
 * [reset_display_data description]
 * @method reset_display_data
 */
void reset_display_data(void)
{
    display_count_data.C                        = 0;
    display_count_data.freq                     = 0;
    display_count_data.MV                       = 0;
    display_count_data.P_mean                   = 0;
    display_count_data.P_peak                   = 0;
    display_count_data.P_peep                   = 0;
    display_count_data.P_plat                   = 0;
    display_count_data.R                        = 0;
    display_count_data.total_breath_cycles      = 0;
    display_count_data.total_tirgger_cycles     = 0;
    display_count_data.ventilation_work_seconds = 0;

    display_count_data.Ti                       = 0;
    display_count_data.Ttotal                   = 0;

    display_count_data.trigger_flag             = 0;
    display_count_data.vt                       = 0;
    display_count_data.vt_sec[0]                = 0;
    display_count_data.vt_sec[1]                = 0;
    display_count_data.vt_sec[2]                = 0;
    display_count_data.vt_sec[3]                = 0;
    display_count_data.flow_leak                = 0;
    display_count_data.inhale_peak_flow        = 0;
    display_count_data.exhale_peak_flow         = 0;

    display_count_data.Flow                     = 0;
    display_count_data.Press                    = 0;
    display_count_data.volume                   = 0;

    display_count_data.default_flow_leak        = 0;
    display_count_data.default_flow_leak_refresh_flag = 0;
    init_flow_leak_compensation_data();
    init_st_real_flow_press_compensation_data();

    reset_alpha_beta_filter_data();
}



// 这个函数用于清除有关“频率”及“潮气量”计算的相关数据，
// 由于这些数据需要保存多个周期的数据，所以不能随着每个周期完成就清理，
// 只有在模式最初工作的一开始，才进行一次清理
/**
 * [reset_display_temp_data description]
 * @method reset_display_temp_data
 */
void reset_display_temp_data(void)
{
    int i;
    for (i = 0; i < MAX_BREATH_COUNT; ++i)
    {
        display_temp_data.breath_time[i]       = 0;
        display_temp_data.breath_time_stamp[i] = ms_1_count;
        display_temp_data.exhale_time[i]       = 0;
        display_temp_data.inhale_time[i]      = 0;
        display_temp_data.vt_rec[i]            = 0;
    }
    display_temp_data.current_data_count       = 0;
    //display_temp_data.last_work_status         = EM_PATIENT_NOT_WORK;
    display_set_last_work_status(EM_PATIENT_NOT_WORK);
    breath_reset_display_temp_data();
    breath_clear_second_vt();
}


/* **************************************
* 呼吸周期开始时(吸气开始)的相关函数
* **************************************/
/**
 * [breath_reset_display_temp_data description]
 * @method breath_reset_display_temp_data
 */
void breath_reset_display_temp_data(void)
{
    display_temp_data.flow_count        = 0;
    display_temp_data.flow_e_sum        = 0;
    display_temp_data.flow_i_sum        = 0;
    display_temp_data.inhale_peak_flow = 0;
    display_temp_data.last_flow         = 0;
    display_temp_data.last_time         = ms_1_count;
    display_temp_data.P_mean            = get_current_press();
    display_temp_data.P_meancount       = 1;
    display_temp_data.P_peak            = display_temp_data.P_mean;
    display_temp_data.P_peep            = display_temp_data.P_mean;
    display_temp_data.P_peep_start_flag = 0;
    display_temp_data.P_peepcount       = 1;
    display_temp_data.T_stamp           = ms_1_count;
    display_temp_data.vt_sum            = 0;
    display_temp_data.vti               = 0;
    display_temp_data.vte               = 0;

    display_temp_data.inhale_peak_flow = 0;
    display_temp_data.exhale_peak_flow  = 0;

    reset_second_vt_temp();

    breath_reset_R_C_data();
#if USER_TEST_PPFFVV
    display_temp_data.v_i               = 0;
    display_temp_data.v_e               = 0;
#endif

}

/**
 * [display_set_last_work_status description]
 * @method display_set_last_work_status
 * @param  status                       [description]
 */
void display_set_last_work_status(uint8_t status)
{
    display_temp_data.last_work_status = status;
}

/* **************************************
* 获取当前状态的相关函数
* **************************************/
/**
 * [breath_get_current_flow description]
 * @method breath_get_current_flow
 * @return                         [description]
 */
int32_t breath_get_current_flow(void)
{
    return display_count_data.Flow;//display_temp_data.last_flow;
}

// =2 近似管路脱落
// =1 管路脱落
// =0 管路正常
/**
 * [check_pipe_out_status description]
 * @method check_pipe_out_status
 * @return                       [description]
 */
uint8_t check_pipe_out_status(void)
{
    // 要考虑瞬间的不稳定状态
    static uint16_t count = 0;
    if(is_rap_actived())
    {
        count = 0;
        return 0; // 在叩击时不检测
    }
    // 如果呼气压力2 则管路没有脱落
    if(get_ex_press() > 20)
    {
        count = 0;
        return 0;
    }
    // 如果呼气流量接近吸气流量，则管路没有脱落。
    //if(display_temp_data.exhale_Flow > display_temp_data.inhale_Flow - 5000)
    if(get_ex_flow() > 5000)
    {
        count = 0;
        return 0;
    }

    // 如果泄漏超过100L/min 认为是脱落
    if(display_count_data.flow_leak > 100000)
    {
        if(count++ > 3000)
            return 1;
        else
            return 0;
    }
    // 如果是排痰状态工作，暂不测试管路脱落
    if(get_patient_breath_mode() == EM_VENTLATOR_COUGH_MODE) return 0;

    //****************************************************************
    // 要考虑单管模式的状态如何处理 ！！！
    //****************************************************************

    // 如果吸气流量>5L/min && 呼气流量< 0.5L/min
    // 吸气压力>2cmH2O &&     -1 <= 呼气压力 <= 1 (cmH2O)
    // 管路脱落成立
    // !!!! 貌似要考虑患者主动吸气时造成的特殊状态
    if(     (get_ins_flow()  > 5000)  &&
            (get_ex_flow()   <  500)  &&
            (get_ins_press() >   50)  &&
            (get_ex_press()  <   10)  &&// 这里检测呼气流速的绝对值 < 1 L/min
            (get_ex_press()  >  -10))
    {
        if(count++ > 3000)
            return 1;
        else
            return 0;
    }


    // 在特殊情况下：计算的泄漏流量 接近 吸气端口的流量,
    //               此状态发生后，泄漏不会被改变---即会持续发生，
    //               所以可能需要把泄漏值 清0，但清零后，又无法继续检测管路脱落的状态了
    //               或许需要刷新泄漏值才好
    if(display_count_data.flow_leak > get_ins_flow() - 500)
    {
        if(count++ > 3000)
            return 1;
        else
            return 0;
    }
    // 否则 不成立
    count = 0;
    return 0;
}



void breath_count_Te(void);
//查看是否到了呼气结束
/**
 * [monitor_exhale_end description]
 * @method monitor_exhale_end
 * @return                    [description]
 */
uint8_t monitor_exhale_end(void)
{
    // 呼气流速小于500ml/Min 则认为呼气结束
    if(display_count_data.Flow > -500)//if(breath_get_current_flow() > -500)
    {
        return 1;
    }
    // 如果呼气超过15秒 则认为呼气结束
    if(breath_get_time_interval() > 15000)
    {
        return 1;
    }
    // 否则未结束
    return 0;
}



/* **************************************
* 呼气开始时的相关函数
* **************************************/
/**
 * [reset_second_vt_temp description]
 * @method reset_second_vt_temp
 */
void reset_second_vt_temp(void)
{
    display_temp_data.vt_sec[0]             = 0;
    display_temp_data.vt_sec[1]             = 0;
    display_temp_data.vt_sec[2]             = 0;
    display_temp_data.vt_sec[3]             = 0;
    display_temp_data.vt_second_cnt         = 0;
    display_temp_data.vt_second_active_flag = 0;
}

/**
 * [breath_clear_second_vt description]
 * @method breath_clear_second_vt
 */
void breath_clear_second_vt(void)
{
    display_count_data.vt_sec[0] = 0;
    display_count_data.vt_sec[1] = 0;
    display_count_data.vt_sec[2] = 0;
    display_count_data.vt_sec[3] = 0;

}

/**
 * [start_second_vt description]
 * @method start_second_vt
 */
void start_second_vt(void)
{
    display_temp_data.vt_second_active_flag = 1;
}

/**
 * [stop_second_vt description]
 * @method stop_second_vt
 */
void stop_second_vt(void)
{
    display_temp_data.vt_second_active_flag = 0;
}

/**
 * [get_secont_vt_location description]
 * @method get_secont_vt_location
 * @return                        [description]
 */
//is_second_vt_start()
uint8_t get_secont_vt_location(void)
{
    uint32_t t;
    t = breath_get_time_interval();
    if(t < 1000)
        return 0;
    if(t < 2000)
        return 1;
    if(t < 3000)
        return 2;
    return 3;
#if 0
    t /= 1000;
    if(t > 3) return 3;
    return t;
#endif
}

/**
 * [append_second_vt description]
 * @method append_second_vt
 * @param  vt               [description]
 */
void append_second_vt(uint32_t vt)
{
    uint8_t num;
    // 如果还没有开始计数就不用执行
    if(!is_second_vt_start())
        return;

    num = get_secont_vt_location();

    display_temp_data.vt_sec[num] += vt;
}

/**
 * [breath_set_secont_vt description]
 * @method breath_set_secont_vt
 */
void breath_set_secont_vt(void)
{
    display_count_data.vt_sec[0] = display_temp_data.vt_sec[0] / 60000;
    display_count_data.vt_sec[1] = display_temp_data.vt_sec[1] / 60000;
    display_count_data.vt_sec[2] = display_temp_data.vt_sec[2] / 60000;
    display_count_data.vt_sec[3] = display_temp_data.vt_sec[3] / 60000;
}

/* **************************************
* 患者主动呼吸的相关函数
* **************************************/
/**
 * [breath_set_trigger_flag description]
 * @method breath_set_trigger_flag
 */
void breath_set_trigger_flag(void)
{
    display_count_data.trigger_flag = 1;
}

/**
 * [breath_reset_trigger_flag description]
 * @method breath_reset_trigger_flag
 */
void breath_reset_trigger_flag(void)
{
    display_count_data.trigger_flag = 0;
}

/**
 * [is_breath_T_mode_start description]
 * @method is_breath_T_mode_start
 * @return                        [description]
 */
uint8_t is_breath_T_mode_start(void)
{
    return (display_count_data.trigger_flag == 0);
}

/**
 * [breath_set_inhale_trigger_flag description]
 * @method breath_set_inhale_trigger_flag
 */
void breath_set_inhale_trigger_flag(void)
{
    display_temp_data.trigger_inhale_flag = 1;
}

/**
 * [breath_reset_inhale_trigger_flag description]
 * @method breath_reset_inhale_trigger_flag
 */
void breath_reset_inhale_trigger_flag(void)
{
    display_temp_data.trigger_inhale_flag = 0;
}

/**
 * [breath_set_exhale_trigger_flag description]
 * @method breath_set_exhale_trigger_flag
 */
void breath_set_exhale_trigger_flag(void)
{
    display_temp_data.trigger_exhale_flag = 1;
}

/**
 * [breath_reset_exhale_trigger_flag description]
 * @method breath_reset_exhale_trigger_flag
 */
void breath_reset_exhale_trigger_flag(void)
{
    display_temp_data.trigger_exhale_flag = 0;
}

/* **************************************
* PEEP开始时的相关函数
* **************************************/

// 清除peep计算相关的数据，为新一轮的计算做准备
/**
 * [display_reset_peep_data description]
 * @method display_reset_peep_data
 */
void display_reset_peep_data(void)
{
    display_temp_data.P_peep = get_current_press();
    display_temp_data.P_peepcount = 1;
    reset_peep_start_flag();
}

/**
 * [display_append_peep description]
 * @method display_append_peep
 * @param  pval                [description]
 */
void display_append_peep(uint16_t pval)
{
    if(!display_temp_data.P_peep_start_flag)
        return; // zzx:2019.3.6 // 在没有启动时，不计算PEEP

    // 如果累计超过20秒，则认为患者未呼吸，理论上需要返回空闲模式，这里暂时重新计数， ---未完成
    if(display_temp_data.P_peepcount < 20000)
    {
        display_temp_data.P_peep += pval;
        display_temp_data.P_peepcount++;
    }
    else
    {
        // 时间太长了，需要重新计数
        display_temp_data.P_peep      = pval;
        display_temp_data.P_peepcount = 1;
    }
}

/**
 * [set_peep_start_flag description]
 * @method set_peep_start_flag
 */
void set_peep_start_flag(void)
{
    display_temp_data.P_peep_start_flag = 1;
}

/**
 * [reset_peep_start_flag description]
 * @method reset_peep_start_flag
 */
void reset_peep_start_flag(void)
{
    display_temp_data.P_peep_start_flag = 0;
}

/**
 * [display_set_peep description]
 * @method display_set_peep
 */
void display_set_peep(void)
{
    // 如果没有开始记录PEEP，则取当前气道压力
    if((display_temp_data.P_peep_start_flag == 0) || (display_temp_data.P_peepcount == 0))
    {
        display_count_data.P_peep = get_current_press();
        return;
    }

    // 计算PEEP
    display_count_data.P_peep = display_temp_data.P_peep / display_temp_data.P_peepcount;
}

/**
 * [display_get_last_peep description]
 * @method display_get_last_peep
 * @return                       [description]
 */
// 得到最近一次的PEEP值
int16_t display_get_last_peep(void)
{
    return display_count_data.P_peep;
}



/* **************************************
* P-mean开始时的相关函数
* **************************************/
/**
 * [reset_Pmean_data description]
 * @method reset_Pmean_data
 */
void reset_Pmean_data(void)
{
    display_temp_data.P_mean      = get_current_press();
    display_temp_data.P_meancount = 1;
}

// Pmean的累计函数
/**
 * [display_append_Pmean description]
 * @method display_append_Pmean
 * @param  pval                 [description]
 */
void display_append_Pmean(uint16_t pval)
{
    if(display_temp_data.P_meancount < 50000)
    {
        display_temp_data.P_mean += pval;
        display_temp_data.P_meancount++;
    }
    else
    {
        // 时间太长了，需要重新计数
        display_temp_data.P_mean      = pval;
        display_temp_data.P_meancount = 1;
    }
}

// 计算Pmean的结果，并放入 display_count_data.P_mean
/**
 * [display_set_Pmean description]
 * @method display_set_Pmean
 */
void display_set_Pmean(void)
{
    if(display_temp_data.P_meancount)
    {
        display_count_data.P_mean = display_temp_data.P_mean / display_temp_data.P_meancount;
    }
    else
    {
        display_count_data.P_mean = get_current_press();
    }
}

/**
 * [display_set_Pplat description]
 * @method display_set_Pplat
 */
void display_set_Pplat(void)
{
    display_count_data.P_mean = get_current_press();
}
/* **************************************
* P-peak开始时的相关函数
* **************************************/
/**
 * [reset_Ppeak_data description]
 * @method reset_Ppeak_data
 */
void reset_Ppeak_data(void)
{
    display_temp_data.P_peak   = get_current_press();
    display_temp_data.P_lowest = display_temp_data.P_peak;
}

/**
 * [display_append_Ppeak description]
 * @method display_append_Ppeak
 * @param  pval                 [description]
 */
void display_append_Ppeak(uint16_t pval)
{
    if(display_temp_data.P_peak < pval)
    {
        display_temp_data.P_peak   = pval;
    }

    if(display_temp_data.P_lowest > pval)
    {
        display_temp_data.P_lowest = pval;
    }
}

/**
 * [display_set_Ppeak description]
 * @method display_set_Ppeak
 */
void display_set_Ppeak(void)
{
    display_count_data.P_peak   = display_temp_data.P_peak;
    display_count_data.P_lowest = display_temp_data.P_lowest;
}


/* *************************************
    计算峰值流速相关
* *************************************/
/**
 * [display_append_inhaleFlowpeak description]
 * @method display_append_inhaleFlowpeak
 * @param  fval                          [description]
 */
__INLINE void display_append_inhaleFlowpeak(uint32_t fval)
{
    if(display_temp_data.inhale_peak_flow < fval)
    {
        display_temp_data.inhale_peak_flow = fval;
    }
}

/**
 * [display_append_exhaleFlowpeak description]
 * @method display_append_exhaleFlowpeak
 * @param  fval                          [description]
 */
__INLINE void display_append_exhaleFlowpeak(uint32_t fval)
{
    if(display_temp_data.exhale_peak_flow < fval)
    {
        display_temp_data.exhale_peak_flow = fval;
    }
}

/**
 * [breath_get_inhale_peak_Flow description]
 * @method breath_get_inhale_peak_Flow
 * @return                             [description]
 */
int32_t breath_get_inhale_peak_Flow(void)
{
    if(display_temp_data.inhale_peak_flow < 1000)
        return 1000;
    else
        return display_temp_data.inhale_peak_flow;
}

/**
 * [breath_get_exhale_peak_Flow description]
 * @method breath_get_exhale_peak_Flow
 * @return                             [description]
 */
int32_t breath_get_exhale_peak_Flow(void)
{
    if(display_temp_data.exhale_peak_flow < 1000)
        return 1000;
    else
        return display_temp_data.exhale_peak_flow;
}

/**
 * [display_set_peak_flow description]
 * @method display_set_peak_flow
 */
void display_set_peak_flow(void)
{
    display_count_data.inhale_peak_flow = breath_get_inhale_peak_Flow();
    display_count_data.exhale_peak_flow = breath_get_exhale_peak_Flow();
}

/* *************************************
    计算触发比率及相关
* *************************************/
/**
 * [reset_trigger_rate_data description]
 * @method reset_trigger_rate_data
 */
void reset_trigger_rate_data(void)
{
    display_count_data.total_breath_cycles  = 0;
    display_count_data.total_tirgger_cycles = 0;
}

// 返回值 0-100, 触发比率 (单位 1%)
/**
 * [display_get_trigger_rate description]
 * @method display_get_trigger_rate
 * @return                          [description]
 */
uint16_t display_get_trigger_rate(void)
{
    uint32_t rate;
    if(display_count_data.total_breath_cycles <   50)
        return 0;
    if(display_count_data.total_breath_cycles == 100)
        return display_count_data.total_tirgger_cycles;
    rate  = display_count_data.total_tirgger_cycles * 100;
    rate /= display_count_data.total_breath_cycles;
    return (uint16_t) rate;
}

/**
 * [inc_trigger_rate description]
 * @method inc_trigger_rate
 */
void inc_trigger_rate(void)
{
    if(display_count_data.total_breath_cycles  < 100)
        (display_count_data.total_breath_cycles)++;
    if(display_count_data.total_tirgger_cycles < 100)
        (display_count_data.total_tirgger_cycles)++;
}

/**
 * [dec_trigger_rate description]
 * @method dec_trigger_rate
 */
void dec_trigger_rate(void)
{
    if(display_count_data.total_breath_cycles  < 100)
        (display_count_data.total_breath_cycles)++;
    if(display_count_data.total_tirgger_cycles > 0)
        (display_count_data.total_tirgger_cycles)--;
}

/**
 * [display_refresh_trigger_rate description]
 * @method display_refresh_trigger_rate
 */
void display_refresh_trigger_rate(void)
{
    if(display_temp_data.trigger_inhale_flag)
    {
        breath_set_trigger_flag();
        inc_trigger_rate();
    }
    else
    {
        breath_reset_trigger_flag();
        dec_trigger_rate();
    }
    breath_reset_inhale_trigger_flag();
    breath_reset_exhale_trigger_flag();
    // clear for next breath loop
}


/* **********************************************
* 泄漏相关
* **********************************************/
// for leak
/**
 * [append_flow_for_leak description]
 * @method append_flow_for_leak
 * @param  Flow_i               [description]
 * @param  Flow_e               [description]
 */
void append_flow_for_leak(uint32_t Flow_i, uint32_t Flow_e)
{
    display_temp_data.flow_i_sum += Flow_i;
    display_temp_data.flow_e_sum += Flow_e;
    display_temp_data.flow_count++;
}

/**
 * [reset_leak_count_data description]
 * @method reset_leak_count_data
 */
void reset_leak_count_data(void)
{
    display_temp_data.flow_i_sum = 0;
    display_temp_data.flow_e_sum = 0;
    display_temp_data.flow_count = 0;
}

/**
 * [display_count_leak description]
 * @method display_count_leak
 */
//每个呼吸开始时，都需要计算一下泄漏，不过需要
void display_count_leak(void)
{
    uint32_t flow_l;

    // 要考虑单管模式的状态如何处理

    if(display_temp_data.flow_i_sum <= display_temp_data.flow_e_sum)
    {
        display_count_data.flow_leak = 0;
    }
    else
    {
        flow_l  = display_temp_data.flow_i_sum - display_temp_data.flow_e_sum;
        flow_l /= display_temp_data.flow_count;
        display_count_data.flow_leak = flow_l;
        flow_leak_refresh_data(display_count_data.P_mean, flow_l );
    }

    set_system_breath_leak_level(display_count_data.flow_leak);
    reset_leak_count_data();
}

/**
 * [breath_get_flow_leak description]
 * @method breath_get_flow_leak
 * @return                      [description]
 */
int32_t breath_get_flow_leak(void)
{
    // 泄漏（这个可能不是定数，会依赖压力变化而变化）需要后期进行压力补偿 ---未完成
    return display_count_data.flow_leak;
}


/* **********************************************
* 肺气阻及肺顺应性
* **********************************************/
/**
 * [breath_reset_R_C_data description]
 * @method breath_reset_R_C_data
 */
void breath_reset_R_C_data(void)
{
}

/**
 * [breath_integral_R_C_DATA description]
 * @method breath_integral_R_C_DATA
 */
void breath_integral_R_C_DATA(void)
{
}

// 必须先计算出Ppeak，Pplat Flowpeak，VT, PEEP
/**
 * [breath_calculate_R_C description]
 * @method breath_calculate_R_C
 */
void breath_calculate_R_C(void)
{
    // 先计算静态RC
    float64_t ftmp;
#if USE_STATIC_R_AND_C
    // 静态 R
    if(display_count_data.P_peak >= display_count_data.P_plat)
        ftmp = (float32_t)(display_count_data.P_peak - display_count_data.P_plat);
    else
        ftmp = 0.0;
    ftmp /= (float32_t) display_count_data.inhale_peak_flow;

    // 修正数据单位
    ftmp *= 1000.0; // 0.1(P) / 0.001(F) * 10(Unit) = 1000
    display_count_data.R = (uint16_t) ftmp;

    // 静态 C
    if(display_count_data.P_peak >= (display_count_data.P_peep - 20))
        ftmp = (float32_t) (display_count_data.P_peak - display_count_data.P_peep);
    else
        ftmp = 20.0;
    ftmp = (float32_t) display_count_data.vt / ftmp;

    // 修正数据单位
    //ftmp *= 1.0; // 1 / 10 * 10
    display_count_data.C = (uint16_t) ftmp;
#else
    // 动态的R&C
#endif
    // correct
    if(display_count_data.R > 2000)
        display_count_data.R = 2000;
    if(display_count_data.R <   30)
        display_count_data.R =   30;


    if(display_count_data.C > 2000)
        display_count_data.C = 2000;
    if(display_count_data.C <   10)
        display_count_data.C =   10;
}


/* **********************************************
* 呼吸潮气量及分钟通气量
* **********************************************/
/**
 * [get_breath_cycle_locate description]
 * @method get_breath_cycle_locate
 * @param  cycles                  [description]
 * @return                         [description]
 */
uint8_t get_breath_cycle_locate(uint8_t cycles)
{
    uint8_t locate;
    uint8_t count;
    locate = display_temp_data.current_data_count; // current
    count  = cycles;
    while(count-- > 0)
    {
        if(locate)
            locate--;
        else
            locate = MAX_BREATH_COUNT - 1;
    }
    return locate;
}

/**
 * [display_append_vt description]
 * @method display_append_vt
 * @param  vt                [description]
 */
void display_append_vt(uint32_t vt)
{
    display_temp_data.vt_rec[display_temp_data.current_data_count] = vt;
    display_count_data.vt = vt;
}

/**
 * [display_inc_breath_count_for_freq_vt description]
 * @method display_inc_breath_count_for_freq_vt
 */
void display_inc_breath_count_for_freq_vt(void)
{
    if(display_temp_data.current_data_count >= MAX_BREATH_COUNT - 1)
        display_temp_data.current_data_count = 0;
    else
        display_temp_data.current_data_count++;
}

/**
 * [add_vt_temp description]
 * @method add_vt_temp
 * @param  vt          [description]
 */
__INLINE void add_vt_temp(int16_t vt)
{
    display_temp_data.vt_sum += vt;
    if(display_temp_data.vt_sum < 0)
        display_temp_data.vt_sum = 0;
}

#if 0
/**
 * [display_refresh_flow description]
 * @method display_refresh_flow
 */
void display_refresh_flow(void)
{
    // 要考虑单管模式的状态如何处理

    // flow
    display_temp_data.inhale_Flow = (int32_t)get_inhale_flow();
    display_temp_data.exhale_Flow  = (int32_t)get_exhale_flow();

    display_count_data.Flow  = display_temp_data.inhale_Flow - breath_get_flow_leak();// 消除泄漏
    display_count_data.Flow -= display_temp_data.exhale_Flow;

    MA_APPEND_DISPLAY_FLOW(display_count_data.Flow);
}

/**
 * [display_refresh_press description]
 * @method display_refresh_press
 */
void display_refresh_press(void)
{
    // Press
    display_temp_data.inhale_Press = get_inhale_press();
    display_temp_data.exhale_Press  = get_exhale_press();
    display_count_data.Press        = get_current_press();

    MA_APPEND_DISPLAY_PRESS(display_count_data.Press);
}
#endif

// 需要计算吸气潮气量
// 吸气时间
// 平均压力
// 泄漏流量
// for pfv data
/**
 * [count_PFV_sum description]
 * @method count_PFV_sum
 */
void count_PFV_sum(void)
{
    // get real flow
    int32_t flow_tmp;//flow_now,
    int32_t p_tmp;
    //int32_t flow_inhale,flow_exhale;
    int32_t dv;
    int32_t time;

    // 要考虑单管模式的状态如何处理

    // Press
    //display_append_Pmean(display_count_data.Press);
    //display_append_Ppeak(display_count_data.Press);
    p_tmp = get_display_press();
    display_append_Pmean(p_tmp);
    display_append_Ppeak(p_tmp);

    // leak sum
    append_flow_for_leak(get_ins_flow(), get_ex_flow());
    if((display_count_data.Flow > -1000) && (display_count_data.Flow < 1000))
    {
        // too small flow, no not need to count vt;
        //return;
        display_count_data.Flow = 0;
    }

    // 计算dV 目前流量单位是 mL/Min, 时间单位是ms dv 的单位是 mL
    flow_tmp   = display_count_data.Flow + display_temp_data.last_flow;
    flow_tmp >>= 1;

    // 计算时间
    time = ms_1_count - display_temp_data.last_time;

    dv = flow_tmp * time;
#if USER_TEST_PPFFVV
    display_temp_data.v_i += get_ins_flow() * time;
    display_temp_data.v_e += get_ex_flow() * time;
#endif
    if((get_patient_status() == EM_PATIENT_EXHALE_DETECT) || (get_patient_status() == EM_PATIENT_T_EXHALE_DETECT))
    {
        dv = 0;
    }
    else if((get_patient_status() == EM_PATIENT_EXHALE_PERIOD) || (get_patient_status() == EM_PATIENT_T_EXHALE_PERIOD))
    {
        if(display_count_data.Flow > 0)
            dv = 0;
    }
    // 这个dv还有可能用于其他计算
    display_temp_data.last_dv = dv;

    display_temp_data.vt_sum += dv;

    //张志新： 貌似没啥用---20190318
#if 0
    if(display_temp_data.vt_sum < 10)
    {
        display_temp_data.vt_sum = 0;
    }
#endif

    display_count_data.volume    = display_temp_data.vt_sum / 60000 / 2;
    if(display_count_data.Flow > 0)
    {
        // 吸气相
        display_temp_data.vti += dv;

        // 峰值流量
        display_append_inhaleFlowpeak(display_count_data.Flow);
    }
    else
    {
        // 此时dv为负数
        // 呼气相
        display_temp_data.vte -= dv;

        // 咳痰专用显示参数
        append_second_vt((uint32_t)(-dv));

        // 峰值流量
        display_append_exhaleFlowpeak(0 - display_count_data.Flow);
    }

    // 用于vt累加
    add_vt_temp(dv);

    //修正上次的流量
    display_temp_data.last_flow = display_count_data.Flow;

    // 修正时间
    display_temp_data.last_time = ms_1_count;

    // 峰值流量
    //if(display_temp_data.inhale_peak_flow < display_count_data.Flow) display_temp_data.inhale_peak_flow = display_count_data.Flow;

    // R&C的计算
    breath_integral_R_C_DATA();
}

/**
 * [breath_get_Vte description]
 * @method breath_get_Vte
 * @return                [description]
 */
uint32_t breath_get_Vte(void)
{
    return display_temp_data.vte / 60000; // ml/ms  ->  ml/min
}

/**
 * [breath_get_Vti description]
 * @method breath_get_Vti
 * @return                [description]
 */
uint32_t breath_get_Vti(void)
{
    return display_temp_data.vti / 60000; // ml/ms  ->  ml/min
}

/**
 * [breath_get_remaining_vt description]
 * @method breath_get_remaining_vt
 * @return                         [description]
 */
int16_t breath_get_remaining_vt(void)
{
    return display_count_data.volume; //(display_temp_data.vti - display_temp_data.vte) / 60000;
}

/* **********************************************
* 呼吸频率及相关
* **********************************************/
/**
 * [breath_count_freq_mv_static description]
 * @method breath_count_freq_mv_static
 * @param  option                      [description]
 */
void breath_count_freq_mv_static(uint8_t option)
{
    static uint8_t Freq_mv_count          = 0;
    static uint32_t Freq_mv_mv_add         = 0;
    static uint32_t Freq_mv_cycle_time_add = 0;
    int tmp;
    switch (option)
    {
    default:
    case EM_FREQ_MV_OPERATION_RESET:     // 直接清除数据
        Freq_mv_count          = 0;
        Freq_mv_mv_add         = 0;
        Freq_mv_cycle_time_add = 0;
        break;

    case EM_FREQ_MV_OPERATION_NORMAL:    // 正常计算MV/FREQ
        Freq_mv_mv_add         += display_count_data.vt;
        Freq_mv_cycle_time_add += display_count_data.Ttotal;
        Freq_mv_count++;
        if(Freq_mv_count >= 8)
        {
            // 到了可以计算数据的时候了
            Freq_mv_cycle_time_add /= Freq_mv_count;

            // 确保频率 < 200BPM
            if(Freq_mv_cycle_time_add < 300)
                Freq_mv_cycle_time_add = 300;
            display_count_data.freq = 600000 / Freq_mv_cycle_time_add;
            Freq_mv_mv_add          = Freq_mv_mv_add / Freq_mv_count;
            tmp                     = Freq_mv_mv_add * display_count_data.freq / 10;
            display_count_data.MV   = tmp;//60000 / Freq_mv_cycle_time_add * Freq_mv_mv_add * display_count_data.freq;

            // 清除数据，为下次计算做准备
            Freq_mv_count           = 0;
            Freq_mv_mv_add          = 0;
            Freq_mv_cycle_time_add  = 0;
        }
        break;
    case EM_FREQ_MV_OPERATION_FORCE:     // 单周期计算MV/FREQ
        Freq_mv_mv_add              = display_count_data.vt;
        Freq_mv_cycle_time_add      = display_count_data.Ttotal;
        Freq_mv_count = 1;

        // 确保频率 < 200BPM
        if(Freq_mv_cycle_time_add < 300)
            Freq_mv_cycle_time_add = 300;
        display_count_data.freq     = 60000 / Freq_mv_cycle_time_add;
        display_count_data.MV       = 60000 / Freq_mv_cycle_time_add * Freq_mv_mv_add;
        break;
    }
}

/* **********************************************
* 时间相关
* **********************************************/
// 计算当前时间和时间戳的时间间隔
/**
 * [breath_get_time_interval description]
 * @method breath_get_time_interval
 * @return                          [description]
 */
uint32_t breath_get_time_interval(void)
{
#if 0
    if(ms_1_count < display_temp_data.T_stamp)
    {
        // Error
        display_temp_data.T_stamp = ms_1_count;
        return 0;
    }
    else
    {
        return ms_1_count - display_temp_data.T_stamp;
    }
#else
    // 毫秒时间在确保溢出时不会出错
    return ms_1_count - display_temp_data.T_stamp;
#endif
}

/**
 * [display_append_inhale_time description]
 * @method display_append_inhale_time
 * @param  Ti                         [description]
 */
void display_append_inhale_time(uint32_t Ti)
{
    display_temp_data.inhale_time[display_temp_data.current_data_count] = Ti;
}

/**
 * [dispaly_append_exhale_time description]
 * @method dispaly_append_exhale_time
 * @param  Te                         [description]
 */
void dispaly_append_exhale_time(uint32_t Te)
{
    display_temp_data.exhale_time[display_temp_data.current_data_count] = Te;
}

/**
 * [update_breath_time description]
 * @method update_breath_time
 */
__INLINE void update_breath_time(void)
{
    // 更新整个呼吸时间
    display_temp_data.breath_time[display_temp_data.current_data_count] =
        display_temp_data.exhale_time[display_temp_data.current_data_count] +
        display_temp_data.inhale_time[display_temp_data.current_data_count];
}

// 设置时间戳，用于计算时间间隔
/**
 * [mark_breath_T_stmap description]
 * @method mark_breath_T_stmap
 */
void mark_breath_T_stmap(void)
{
    display_temp_data.T_stamp = ms_1_count;
}


// 设置吸气开始时间戳，用于计算FREQ/MV
/**
 * [set_breath_start_stamp description]
 * @method set_breath_start_stamp
 */
void set_breath_start_stamp(void)
{
    display_temp_data.breath_time_stamp[display_temp_data.current_data_count] = ms_1_count;
}

// 由之前的时间戳计算吸气时间
/**
 * [breath_count_Ti description]
 * @method breath_count_Ti
 */
void breath_count_Ti(void)
{
    uint32_t t;
    if(is_vti_counted()) return;
    set_vti_count_flag();

    t = breath_get_time_interval();

    if(t > 30000)
    {
        t = 30000; // 吸气时间 最大30秒
        // 张志新： 需要做点什么
    }

    display_temp_data.inhale_time[display_temp_data.current_data_count] = t;
    display_count_data.Ti                                                = t;
}

// 由之前的时间戳计算呼气时间
/**
 * [breath_count_Te description]
 * @method breath_count_Te
 */
void breath_count_Te(void)
{
    uint32_t t;
    if(is_vte_counted())
        return;

    set_vte_count_flag();
    t = breath_get_time_interval();
    if(t > 30000)
    {
        t = 30000; // 呼气时间 最大30秒
        // 张志新： 需要做点什么
    }

    display_temp_data.exhale_time[display_temp_data.current_data_count] = t;

    display_count_data.Ttotal = t + display_count_data.Ti;

    // 更新整个呼吸时间
    update_breath_time();
}

// 在呼气结束时需要单独计算的内容
/**
 * [count_data_for_exhale_end description]
 * @method count_data_for_exhale_end
 */
void count_data_for_exhale_end(void)
{
    //计算一周期完成的吸呼气数据
    breath_count_Te();
}

/**
 * [get_display_temp_vti description]
 * @method get_display_temp_vti
 * @return                      [description]
 */
uint32_t get_display_temp_vti(void)
{
    return display_temp_data.vti / 60000;
}

/**
 * [get_display_temp_vte description]
 * @method get_display_temp_vte
 * @return                      [description]
 */
uint32_t get_display_temp_vte(void)
{
    return display_temp_data.vte / 60000;
}

/**
 * [breath_cycle_completed_actions description]
 * @method breath_cycle_completed_actions
 */
void breath_cycle_completed_actions(void)
{
    // 如果是从EM_PATIENT_BREATH_DETECT过来的，则不处理
    if(display_temp_data.last_work_status != EM_PATIENT_INHALE_DETECT)
        return;

    //计算一周期完成的吸呼气数据
    breath_count_Te();

    // reset Vti & Vte count flag
    clear_vti_count_flag();
    clear_vte_count_flag();

    //潮气量<20ml时，是做错误状态（会有个机械动作，造成吸气的误判）
    if(get_display_temp_vte() < 20) //20ml
    {
        return;
    }

    //确保 PEEP至少有一组数据
    display_append_peep(display_count_data.Press);

    // （呼气）潮气量更新
    display_append_vt(breath_get_Vte());

    //峰值压力/平均压力/PEEP
    display_set_Ppeak();
    display_set_Pmean();
    display_set_peep();

    //本周期平均泄漏流量（用于下一周期）
    // 咳痰时不进行计算
    if(cough_control_data.cough_display_data_ready == EM_INACTIVED)
    {
        display_count_leak();
    }

    // 峰值流速
    display_set_peak_flow();

    // 更新触发比率
    display_refresh_trigger_rate();

    //计算并更新R/C
    breath_calculate_R_C();

    //更新呼吸周期
    display_inc_breath_count_for_freq_vt();

    //呼吸周期及频率/分钟通气量
    //breath_count_freq_mv();
    breath_count_freq_mv_static(EM_FREQ_MV_OPERATION_NORMAL);

    // 每秒的潮气量计算
    breath_set_secont_vt();


    // 做BTPS和管路补偿
    display_count_data.vt = basic_VT_btps_and_pipe_compensation(display_count_data.vt, display_count_data.P_peak - display_count_data.P_peep);
    // 设置一呼吸周期数据准备好的标志，== 用于给UI通讯
    // Tell UI that one breath have been completed
    set_one_cycle_complete_flag();
}

/**
 * [calculate_defult_leak_flow_old description]
 * @method calculate_defult_leak_flow_old
 */
// 系统在缺省模式下，使用独立的变量计算泄漏量
void calculate_defult_leak_flow_old(void)
{
    // defines
    static int16_t count  = 0;
    static int32_t flow_i = 0;
    static int32_t flow_e = 0;
    // 要考虑叩击时的影响

    // 要考虑单管模式的状态如何处理

    // code start --->
    // add
    if(count++ < 3000) // 3s once
    {
        flow_i += get_ins_flow();
        flow_e += get_ex_flow();
        return;
    }

    // calculate
    display_count_data.flow_leak = (flow_i - flow_e) / count;
    flow_leak_refresh_data(get_real_press(), display_count_data.flow_leak);

    // clear for next calculate
    count  = 0;
    flow_i = 0;
    flow_e = 0;
}

/**
 * [calculate_refresh_defult_leak description]
 * @method calculate_refresh_defult_leak
 */
void calculate_refresh_defult_leak(void)
{
    if(!display_count_data.default_flow_leak_refresh_flag)
        return;
    display_count_data.default_flow_leak_refresh_flag = 0;
    display_count_data.flow_leak = display_count_data.default_flow_leak;
    flow_leak_refresh_data(get_real_press(), display_count_data.flow_leak);
}

/**
 * [calculate_defult_leak_flow1 description]
 * @method calculate_defult_leak_flow1
 */
// 系统在缺省模式下，使用独立的变量计算泄漏量
void calculate_defult_leak_flow1(void)
{
    // defines
    static int16_t count  = 0;
    static int32_t flow_i = 0;
    static int32_t flow_e = 0;
    // 要考虑叩击时的影响

    // 要考虑单管模式的状态如何处理

    // code start --->
    // add
    if(count++ < 3000) // 3s once
    {
        flow_i += get_ins_flow();
        flow_e += get_ex_flow();
        return;
    }

    // calculate
    display_count_data.default_flow_leak = (flow_i - flow_e) / count;
    display_count_data.default_flow_leak_refresh_flag = 1;
    //flow_leak_refresh_data(get_real_press(), display_count_data.flow_leak);

    // clear for next calculate
    count  = 0;
    flow_i = 0;
    flow_e = 0;
}

/**
 * [calculate_breath_data_for_CPAP description]
 * @method calculate_breath_data_for_CPAP
 */
// CPAP 数据计算
void calculate_breath_data_for_CPAP(void)
{
    //display_refresh_flow();
    switch(get_patient_status())
    {
    case EM_PATIENT_NOT_WORK:   // 未工作
        // only for leak flow
        //calculate_defult_leak_flow();
        calculate_refresh_defult_leak();
        breath_count_freq_mv_static(EM_FREQ_MV_OPERATION_RESET);
        // 清除本周期的临时变量用于新的周期计数
        breath_reset_display_temp_data();
        breath_count_freq_mv_static(EM_FREQ_MV_OPERATION_RESET);
        break;

    case EM_PATIENT_PIPE_OUT:   // 管路脱落
        // 需要刷新泄漏流量值，用于判定管路脱落后，是否有被还原的状态
        //calculate_defult_leak_flow();
        calculate_refresh_defult_leak();
        breath_count_freq_mv_static(EM_FREQ_MV_OPERATION_RESET);
        break;

    case EM_PATIENT_BREATH_DETECT: // 呼吸检测
        // 泄漏检测流程 （每统计3秒做一次计算）
        //calculate_defult_leak_flow();
        calculate_refresh_defult_leak();
        break;

    case EM_PATIENT_INHALE_DETECT: // 吸气检测
        // 需要计算呼气潮气量        // 呼气时间        // 平均压力        // display Vt
        count_PFV_sum();

        // PEEP
        display_append_peep(display_count_data.Press);
        break;
    case EM_PATIENT_INHALE_START: // 吸气开始
        //breath_count_Te();

        breath_cycle_completed_actions();

        // 清除本周期的临时变量用于新的周期计数
        breath_reset_display_temp_data();

        // ？？？ 是不是还要初始数据
        // 吸呼气 时间戳
        mark_breath_T_stmap();

        // 设置吸气开始的时间
        set_breath_start_stamp();

        // display Vt
        count_PFV_sum();
        break;
    case EM_PATIENT_INHALE_PERIOD: // 吸气过程

        // 呼气潮气量积分
        // 峰值压力（控制模式才有意义）
        // 峰值流速
        // 泄漏流量
        count_PFV_sum();

        // 如果吸气时间小于200ms,清除峰值流速
        if(breath_get_time_interval() < 200)
        {
            display_temp_data.inhale_peak_flow = 0;
        }
        break;
    case EM_PATIENT_BREATH_HOLD: // 屏气
        //暂时不做任何处理
        count_PFV_sum(); // for new pcv control used
        break;
    case EM_PATIENT_EXHALE_DETECT: // 呼气检测
        // 需要计算吸气潮气量
        // 吸气时间
        // 平均压力
        // 泄漏流量
        count_PFV_sum();
        break;
    case EM_PATIENT_EXHALE_START: // 呼气开始
        // 吸气时间
        breath_count_Ti();
        // 吸气潮气量

        // 吸呼气 时间戳
        mark_breath_T_stmap();

        // 开始计算second vt
        start_second_vt();

        // 需要计算吸气潮气量
        // 吸气时间
        // 平均压力
        // 泄漏流量
        count_PFV_sum();
        break;
    case EM_PATIENT_EXHALE_PERIOD: // 呼气过程
        // 呼气潮气量积分
        // 呼气时间积分
        // 峰值压力
        // 泄漏流量
        // display Vt
        count_PFV_sum();

        // 如果呼气时间小于200ms,清除峰值流速
        if(breath_get_time_interval() < 200)
        {
            display_temp_data.exhale_peak_flow = 0;
        }

        // 这里一直清0，确保PEEP计算接近真实的数据
        display_reset_peep_data();
        break;

    ////////////////////////////////////////////////////
    // 状态结束标记
    case EM_PATIENT_STATUS_ENDS: // 其它
        break;

    default:
        break;
    }

}

/**
 * [calculate_breath_data_for_ST description]
 * @method calculate_breath_data_for_ST
 */
// S/T 数据计算
void calculate_breath_data_for_ST(void)
{
    //display_refresh_flow();
    switch(get_patient_status())
    {
    case EM_PATIENT_NOT_WORK:               // 未工作
        reset_display_data();
        // only for leak flow
        //calculate_defult_leak_flow();
        calculate_refresh_defult_leak();
        // do nothing
        // 清除本周期的临时变量用于新的周期计数
        breath_reset_display_temp_data();
        breath_count_freq_mv_static(EM_FREQ_MV_OPERATION_RESET);
        return;

    case EM_PATIENT_BREATH_DETECT:          // 呼吸检测
        // 泄漏检测流程 （每统计3秒做一次计算）
        //calculate_defult_leak_flow();
        calculate_refresh_defult_leak();
        break;

    case EM_PATIENT_INHALE_DETECT:         // 吸气检测
        // 需要计算呼气潮气量        // 呼气时间        // 平均压力        // display Vt
        count_PFV_sum();

        // PEEP
        display_append_peep(display_count_data.Press);
        break;

    case EM_PATIENT_INHALE_START:          // 吸气开始
        breath_cycle_completed_actions();
        // 清除本周期的临时变量用于新的周期计数
        breath_reset_display_temp_data();

        // ？？？ 是不是还要初始数据
        // 吸呼气 时间戳
        mark_breath_T_stmap();
        // 设置吸气开始的时间
        set_breath_start_stamp();

        // display Vt
        count_PFV_sum();
        break;

    case EM_PATIENT_INHALE_PERIOD:         // 吸气过程
        // 呼气潮气量积分
        // 峰值压力（控制模式才有意义）
        // 峰值流速
        // 泄漏流量
        count_PFV_sum();
        display_set_Pplat();
        break;

    case EM_PATIENT_BREATH_HOLD:            // 屏气
        //暂时不做任何处理
        break;

    case EM_PATIENT_EXHALE_DETECT:          // 呼气检测
        // 需要计算吸气潮气量
        // 吸气时间
        // 平均压力
        // 泄漏流量
        count_PFV_sum();
        break;

    case EM_PATIENT_T_EXHALE_START:
    case EM_PATIENT_EXHALE_START:           // 呼气开始
        // 吸气时间
        breath_count_Ti();
        // 吸气潮气量

        // 吸呼气 时间戳
        mark_breath_T_stmap();

        // 开始计算second vt
        start_second_vt();

        // 需要计算吸气潮气量
        // 吸气时间
        // 平均压力
        // 泄漏流量
        count_PFV_sum();
        break;

    case EM_PATIENT_EXHALE_PERIOD:          // 呼气过程
        // 呼气潮气量积分
        // 呼气时间积分
        // 峰值压力
        // 泄漏流量
        // display Vt
        count_PFV_sum();

        // 这里一直清0，确保PEEP计算接近真实的数据
        display_reset_peep_data();
        break;

    ////////////////////////////////////////////////////
    // 机械通气过程
    case EM_PATIENT_T_INHALE_START:        // T吸气开始
        breath_cycle_completed_actions();
        // 清除本周期的临时变量用于新的周期计数
        breath_reset_display_temp_data();
        // 吸呼气 时间戳
        mark_breath_T_stmap();
        // 设置吸气开始的时间
        set_breath_start_stamp();

        // display Vt
        count_PFV_sum();
        break;

    case EM_PATIENT_T_INHALE_PERIOD:       // T吸气过程
        count_PFV_sum();
        display_set_Pplat();
        break;

    case EM_PATIENT_T_EXHALE_DETECT:        // T呼气检测
        // 需要计算吸气潮气量
        // 吸气时间
        // 平均压力
        // 泄漏流量
        count_PFV_sum();
        break;

    case EM_PATIENT_T_EXHALE_PERIOD:        // T呼气过程
        count_PFV_sum();
        // 这里一直清0，确保PEEP计算接近真实的数据
        display_reset_peep_data();
        break;

    ////////////////////////////////////////////////////
    // 状态结束标记
    case EM_PATIENT_STATUS_ENDS:            // 其它
        break;

    default:
        return;
    }

}

/**
 * [calculate_breath_data_for_PCV description]
 * @method calculate_breath_data_for_PCV
 */
// PCV 数据计算
void calculate_breath_data_for_PCV(void)
{
    switch(get_patient_status())
    {
    case EM_PATIENT_PIPE_OUT:               // 管路脱落
        reset_display_data();
        // only for leak flow
        //calculate_defult_leak_flow();
        calculate_refresh_defult_leak();
        // do nothing
        // 清除本周期的临时变量用于新的周期计数
        breath_reset_display_temp_data();
        breath_count_freq_mv_static(EM_FREQ_MV_OPERATION_RESET);
        return;

    // 呼吸检测
    case EM_PATIENT_NOT_WORK:     // 未工作
        reset_display_data();
        // only for leak flow
        //calculate_defult_leak_flow();
        calculate_refresh_defult_leak();
        // do nothing
        // 清除本周期的临时变量用于新的周期计数
        breath_reset_display_temp_data();
        breath_count_freq_mv_static(EM_FREQ_MV_OPERATION_RESET);
        return;

    case EM_PATIENT_EXHALE_START:           // 呼气开始
        // 吸气时间
        breath_count_Ti();
        // 吸气潮气量

        // 吸呼气 时间戳
        mark_breath_T_stmap();

        // 开始计算second vt
        start_second_vt();

        // 需要计算吸气潮气量
        // 吸气时间
        // 平均压力
        // 泄漏流量
        count_PFV_sum();
        break;

    ////////////////////////////////////////////////////
    case EM_PATIENT_T_INHALE_START:        // T吸气开始

        breath_cycle_completed_actions();

        // 清除本周期的临时变量用于新的周期计数
        breath_reset_display_temp_data();

        // 吸呼气 时间戳
        mark_breath_T_stmap();

        // 设置吸气开始的时间
        set_breath_start_stamp();

        // display Vt
        count_PFV_sum();
        break;

    case EM_PATIENT_T_INHALE_PERIOD:       // T吸气过程
        // display Vt
        count_PFV_sum();

        // 这里一直清0，确保PEEP计算接近真实的数据
        display_reset_peep_data();
        display_set_Pplat();
        break;

    case EM_PATIENT_T_EXHALE_DETECT:        // T呼气检测
        // 需要计算吸气潮气量
        // 吸气时间
        // 平均压力
        // 泄漏流量
        count_PFV_sum();
        break;

    case EM_PATIENT_T_EXHALE_PERIOD:        // T呼气过程
        count_PFV_sum();

        // 这里一直清0，确保PEEP计算接近真实的数据
        display_reset_peep_data();
        break;
    case EM_PATIENT_T_INHALE_DETECT: // T吸气检测
        // 需要计算呼气潮气量        // 呼气时间        // 平均压力        // display Vt
        count_PFV_sum();

        // PEEP
        display_append_peep(display_count_data.Press);
        break;

    ////////////////////////////////////////////////////
    // 状态结束标记
    default:
        break;
    }
}

/* **********************************************
* 咳痰数据 计算的相关函数
* **********************************************/
// 初始化咳痰相关的数据
/**
 * [reset_display_data_for_cough description]
 * @method reset_display_data_for_cough
 */
void reset_display_data_for_cough(void)
{
    // 如果吸气半途中开始咳痰，则计数有效，其他情况则重新开始
    /*
       switch(global_patient_status)
       {

       case EM_PATIENT_INHALE_PERIOD:
       case EM_PATIENT_T_INHALE_PERIOD:
        break;

       default:
        break;
       }
     */

    breath_cycle_completed_actions();
    // 清除本周期的临时变量用于新的周期计数
    breath_reset_display_temp_data();

    // ？？？ 是不是还要初始数据
    // 吸呼气 时间戳
    mark_breath_T_stmap();
    // 设置吸气开始的时间
    set_breath_start_stamp();
}

/**
 * [display_start_cough_count description]
 * @method display_start_cough_count
 */
// 在负压排咳痰开始时，需要调用这个函数
void display_start_cough_count(void)
{
    // 吸气时间
    breath_count_Ti();
    // 吸气潮气量

    // 吸呼气 时间戳
    mark_breath_T_stmap();

    // 开始计算second vt
    start_second_vt();
}

/**
 * [calculate_cough_cycle_display_data description]
 * @method calculate_cough_cycle_display_data
 */
void calculate_cough_cycle_display_data(void)
{
    // 发送显示数据
    // 计算排痰数据
    //display_temp_data.last_work_status = EM_PATIENT_INHALE_DETECT;
    display_set_last_work_status(EM_PATIENT_INHALE_DETECT);
    breath_cycle_completed_actions();

    // 避免下一个呼吸数据覆盖咳痰数据
    //display_temp_data.last_work_status = EM_PATIENT_BREATH_DETECT;
    display_set_last_work_status(EM_PATIENT_BREATH_DETECT);
    // 清除本周期的临时变量用于新的周期计数
    breath_reset_display_temp_data();

    // ？？？ 是不是还要初始数据
    // 吸呼气 时间戳
    mark_breath_T_stmap();
}

/**
 * [calculate_breath_data_for_COUGH description]
 * @method calculate_breath_data_for_COUGH
 */
void calculate_breath_data_for_COUGH(void)
{
    // 咳痰过程
    switch(get_patient_status())
    {
    case EM_PATIENT_COUGH_PREPARE:        // 咳痰准备
        // 准备阶段不需要做其他的动作
        break;
    case EM_PATIENT_COUGH_PRESSURE_INCREASE: // 咳痰压力上升
        // 计算吸气潮气量
        // display Vt
        count_PFV_sum();
        break;
    case EM_PATIENT_COUGH_DETECTING_START: // 自主咳痰检测
        // 时刻关注咳痰动作，并计算相应的呼气潮气量
        // display Vt
        count_PFV_sum();
        break;
    case EM_PATIENT_COUGH_PERIOD:         // 咳痰过程
        // display Vt
        count_PFV_sum();
        // 此时正在检测咳痰条件, 此时吸气压力传感器有可能是负的压力（目前还没处理）
        break;
    case EM_PATIENT_COUGH_COMPLETE_AND_REPEAT:// 结束
        // display Vt
        count_PFV_sum();
        // 操作放在 breath_control.c 的breath_cough_mode() 函数
        break;
    }
}

/**
 * [refresh_flow_press description]
 * @method refresh_flow_press
 */
static void refresh_flow_press(void)
{
    if(basic_control_parameters.air_type == EM_BTPS) // 因为流量传感器出错，此状态仅为测试
    {
        // 需要界定是否是雾化启动状态
        if(is_nebulizer_valve_open())
        {
            set_refresh_compensation_press_flow(
                get_inhale_press(),
                get_exhale_press(),
                get_inhale_flow() + DEFAULT_NEBULIZER_FLOW,
                0);
        }
        else
        {
            set_refresh_compensation_press_flow(
                get_inhale_press(),
                get_exhale_press(),
                get_inhale_flow(),
                0);
        }
    }
    else
    {
        // 需要界定是否是雾化启动状态
        if(is_nebulizer_valve_open())
        {
            set_refresh_compensation_press_flow(
                get_inhale_press(),
                get_exhale_press(),
                get_inhale_flow() + DEFAULT_NEBULIZER_FLOW,
                get_exhale_flow());
        }
        else
        {
            set_refresh_compensation_press_flow(
                get_inhale_press(),
                get_exhale_press(),
                get_inhale_flow(),
                get_exhale_flow());
        }
    }
    display_refresh_compensation_press_flow();
#if 0
    // 非滤波
    display_count_data.Flow  = get_real_flow();
    display_count_data.Press = get_real_press();
#else
    // 滤波
    display_count_data.Flow  = alpha_beta_filter(get_real_flow(), (ST_ALPHA_BETA_FILTER_DATA *) &alpha_beta_filter_flow);
    display_count_data.Press  = alpha_beta_filter(get_real_press(), (ST_ALPHA_BETA_FILTER_DATA *) &alpha_beta_filter_press);
#endif

    // 加入移动平均窗
    MA_APPEND_DISPLAY_FLOW(display_count_data.Flow);
    MA_APPEND_DISPLAY_PRESS(display_count_data.Press);
}

/* **********************************************
* 呼吸数据 总的计算入口
* **********************************************/
// 感觉应该按照不同模式进行区分会比较合理一些
/**
 * [calculate_breath_data description]
 * @method calculate_breath_data
 */
void calculate_breath_data(void)
{
    refresh_flow_press();
    //calculate_defult_leak_flow();
    calculate_defult_leak_flow1();
    switch(get_patient_breath_mode())
    {
    case EM_VENTLATOR_STANDBY_MODE:
        break;

    //基本模式
    case EM_VENTLATOR_CPAP_MODE:
#if 0
        if(is_rap_actived()) break;
        if(is_rap_enable())
        {
            set_breath_rap_start();
            breath_count_freq_mv_static(EM_FREQ_MV_OPERATION_RESET);
            break;
        }
#endif
        calculate_breath_data_for_CPAP();
        break;

    // 咳痰模式
    case EM_VENTLATOR_COUGH_MODE:
        calculate_breath_data_for_COUGH();
        break;

    // 高级模式
    case EM_VENTLATOR_PAV_MODE:

        break;

    //兼容 PHILIPS 伟康 V60
    case EM_VENTLATOR_BIPAP_ST_MODE:
        calculate_breath_data_for_ST();
        break;

    case EM_VENTLATOR_BIPAP_PCV_MODE:
        calculate_breath_data_for_PCV();
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

// ===============The end
