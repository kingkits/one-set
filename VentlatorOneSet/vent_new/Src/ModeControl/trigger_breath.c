// File:trigger_breath.c
#include "../global.h"
#include "trigger_breath.h"
//#include "Display_data_count.h"

#define MAX_TRIGGER_BUFFER_LEN 64
typedef struct
{
    int16_t press[MAX_TRIGGER_BUFFER_LEN]; // 压力缓冲区
    int32_t flow[MAX_TRIGGER_BUFFER_LEN];  // 流量缓冲区
    uint8_t type;                          // 需要检测的模式
    uint8_t start_position;                // 环形链表起点
    uint8_t stop_position;                 // 环形链表结束点

    // 触发门限
    int32_t flow_gate_max_p;               // 正流量上限
    int32_t flow_gate_min_p;               // 正流量下限
    int32_t flow_gate_max_n;               // 负流量上限
    int32_t flow_gate_min_n;               // 负流量下限
    int32_t flow_gate_scale_p;             // 正流量范围
    int32_t flow_gate_scale_n;             // 负流量范围

    int16_t press_gate_max_p;               // 正压力上限
    int16_t press_gate_min_p;               // 正压力下限
    int16_t press_gate_max_n;               // 负压力上限
    int16_t press_gate_min_n;               // 负压力下限

    int16_t press_gate_scale_p;				// 正压范围
    int16_t press_gate_scale_n;             // 负压范围
    // 基准
    int16_t press_base_p;                   // 压力基准
    int32_t flow_base_p;                    // 流量基准
    int16_t press_base_n;                   // 压力基准
    int32_t flow_base_n;                    // 流量基准

    // 貌似应该有吸气呼气的区分
    //	int16_t press_status;
    //	int16_t flow_status;
    int16_t press_status_inhale;
    int16_t flow_status_inhale;
    int16_t press_status_exhale;
    int16_t flow_status_exhale;

    uint16_t inhale_status;                // 当前的吸气触发状态
    uint16_t exhale_status;                 // 当前的呼气触发状态
} ST_TRIGGER_DETECTED_DATA;
ST_TRIGGER_DETECTED_DATA st_trigger_data;

#define MAX_TRIGGER_DETECTED_NUM 8
/**
 * [dec_trigger_locate description]
 * @method dec_trigger_locate
 * @param  locate             [description]
 * @return                    [description]
 */
#define dec_trigger_locate(locate) 		if(st_trigger_data.stop_position)locate --;	else locate = (MAX_TRIGGER_BUFFER_LEN - 1);
/**
 * [inc_trigger_locate description]
 * @method inc_trigger_locate
 * @param  locate             [description]
 * @return                    [description]
 */
#define inc_trigger_locate(locate) 		if(st_trigger_data.stop_position == (MAX_TRIGGER_BUFFER_LEN - 1))locate = 0;else locate++;
/**
 * [is_trigger_inhale_actived description]
 * @method is_trigger_inhale_actived
 * @return [description]
 */
#define is_trigger_inhale_actived()     (st_trigger_data.inhale_status > 200) //(st_trigger_data.inhale_status > st_trigger_data.flow_gate_scale_p + st_trigger_data.press_gate_scale_n)
/**
 * [is_trigger_exhale_actived description]
 * @method is_trigger_exhale_actived
 * @return [description]
 */
#define is_trigger_exhale_actived()     (st_trigger_data.exhale_status > 200)

/**
 * [reset_trigger_gate description]
 * @method reset_trigger_gate
 */
void reset_trigger_gate(void)
{

}

void trigger_force_confirm_type(uint8_t type)
{
	//if(type >= EM_TRIGGER_TYPE_DEFINES_ENDS) return;
	//if(st_trigger_data.type!=type)
		st_trigger_data.type=type;
}
/**
 * [set_trigger_gate_fmaxp description]
 * @method set_trigger_gate_fmaxp
 * @param  fmaxp_flow             [description]
 */
void set_trigger_gate_fmaxp(int32_t fmaxp_flow)
{
    st_trigger_data.flow_gate_max_p = fmaxp_flow;
}

/**
 * [set_trigger_gate_fminp description]
 * @method set_trigger_gate_fminp
 * @param  fminp_flow             [description]
 */
void set_trigger_gate_fminp(int32_t fminp_flow)
{
    st_trigger_data.flow_gate_min_p = fminp_flow;
}

/**
 * [set_trigger_gate_fmaxn description]
 * @method set_trigger_gate_fmaxn
 * @param  fmaxn_flow             [description]
 */
void set_trigger_gate_fmaxn(int32_t fmaxn_flow)
{
    st_trigger_data.flow_gate_max_n = fmaxn_flow;
}

/**
 * [set_trigger_gate_fminn description]
 * @method set_trigger_gate_fminn
 * @param  fminn_flow             [description]
 */
void set_trigger_gate_fminn(int32_t fminn_flow)
{
    st_trigger_data.flow_gate_min_n = fminn_flow;
}

/**
 * [set_trigger_gate_pmaxp description]
 * @method set_trigger_gate_pmaxp
 * @param  pmaxp_press            [description]
 */
void set_trigger_gate_pmaxp(int32_t pmaxp_press)
{
    st_trigger_data.press_gate_max_p = pmaxp_press;
}

/**
 * [set_trigger_gate_pminp description]
 * @method set_trigger_gate_pminp
 * @param  pminp_press            [description]
 */
void set_trigger_gate_pminp(int32_t pminp_press)
{
    st_trigger_data.press_gate_min_p = pminp_press;
}

/**
 * [set_trigger_gate_pmaxn description]
 * @method set_trigger_gate_pmaxn
 * @param  pmaxn_press            [description]
 */
void set_trigger_gate_pmaxn(int32_t pmaxn_press)
{
    st_trigger_data.press_gate_max_n = pmaxn_press;
}

/**
 * [set_trigger_gate_pminn description]
 * @method set_trigger_gate_pminn
 * @param  pminn_press            [description]
 */
void set_trigger_gate_pminn(int32_t pminn_press)
{
    st_trigger_data.press_gate_min_n = pminn_press;
}

/**
 * [refresh_trigger_gate_scale description] 重置门限上下限
 * 如果将来确定都是常数，可以考虑合并成常数，并取消这个函数
 * @method refresh_trigger_gate_scale
 */
void refresh_trigger_gate_scale(void)
{
    st_trigger_data.flow_gate_scale_p = 90;//st_trigger_data.flow_gate_max_p-st_trigger_data.flow_gate_min_p;
    st_trigger_data.flow_gate_scale_n = 90;//st_trigger_data.flow_gate_min_n-st_trigger_data.flow_gate_max_n;

    st_trigger_data.press_gate_scale_p = 90;//st_trigger_data.press_gate_max_p-st_trigger_data.press_gate_min_p;
    st_trigger_data.press_gate_scale_n = 90;//st_trigger_data.press_gate_min_n-st_trigger_data.press_gate_max_n;
}

/**
 * [reset_trigger_detected_data description]使用当前状态数据，复位触发工作变量，为重新开始触发检测做准备
 * @method reset_trigger_detected_data
 * @param  press                       [description]当前的压力
 * @param  flow                        [description]当前的流量
 */
void reset_trigger_detected_data(int16_t press, int32_t flow)
{
    int i;
    for(i = 0; i < MAX_TRIGGER_BUFFER_LEN; i++)
    {
        st_trigger_data.press[i]        = press;
        st_trigger_data.flow[i]         = flow;
    }
    st_trigger_data.inhale_status       = 0;
    st_trigger_data.exhale_status       = 0;
    st_trigger_data.start_position      = 0;
    st_trigger_data.stop_position       = 0;
    st_trigger_data.type                = EM_TRIGGER_TYPE_IDLE;// 不做触发检测

    // temp data
    st_trigger_data.flow_status_inhale  = 0;
    st_trigger_data.press_status_inhale = 0;
    st_trigger_data.flow_status_exhale  = 0;
    st_trigger_data.press_status_exhale = 0;

    st_trigger_data.inhale_status       = 0;
    st_trigger_data.exhale_status       = 0;
}

/**
 * [get_trigger_data_counts description]返回当前计数量
 * @method get_trigger_data_counts
 * @return                         [description]
 */
uint8_t get_trigger_data_counts(void)
{
    uint8_t num;
    if(st_trigger_data.start_position == st_trigger_data.stop_position) return EM_FALSE;
    num  = (uint8_t) st_trigger_data.stop_position - st_trigger_data.start_position;
    num &= (MAX_TRIGGER_BUFFER_LEN - 1);
    return num;
}

/**
 * [get_trigger_prenum_locate description]得到之前若干步骤的位置
 * @method get_trigger_prenum_locate
 * @param  pre_v                     [description]
 * @return                           [description]
 */
// 为减少运行时间，这里不判断是否有足够的数据
uint8_t get_trigger_prenum_locate(uint8_t pre_v)
{
    uint8_t locate;
    if(st_trigger_data.stop_position)
        locate = st_trigger_data.stop_position - 1;
    else locate = (MAX_TRIGGER_BUFFER_LEN - 1);
    while(pre_v --)
    {
        dec_trigger_locate(locate);
    }
    return locate;
}


/**
 * [trigger_detect_inhale description]
 * @method trigger_detect_inhale
 * @return                        [description]
 */
uint16_t trigger_detect_inhale(void)
{
    uint8_t locate;
    uint8_t press_cnt;
    uint8_t flow_cnt;
    uint8_t num;
    int i;
    //return EM_TRIGGER_TYPE_E_INS
    num = get_trigger_data_counts();
    if(num < 16) return EM_FALSE;

    locate = get_trigger_prenum_locate(MAX_TRIGGER_DETECTED_NUM);

    // 这里的几个门限 ***都需要验证
    // 或许不同的通气模式也不一样，或许不同的状态也不一样，
    // 主要是做两个方面的工作，
    // 1 --- 如何尽量不遗漏触发信号，
    // 2 --- 如何规避误触发的信号
    // 3 --- 或许用个比值(而不是0-1)会更好的描述触发状态，这需要很多算法，不知道时间是否充裕
    // 吸气流量连续超过门限
    //A 连续8个流量数据超过门限，视为吸气触发
    // 压力连续超过门限
    //A 连续8个压力数据超过门限，视为吸气触发
    // 要充分考虑上一呼吸周期的泄漏水平
    // 泄漏水平会影响PEEP控制，排痰，叩击，通气参数补偿和触发控制方案
    // 需要在后期进行完善
    i = 0;
    press_cnt = 0;
    flow_cnt = 0;

    while(i++ < MAX_TRIGGER_DETECTED_NUM)
    {
        // 吸气流量连续超过门限
        //A 连续8个流量数据超过门限，视为吸气触发
        if(st_trigger_data.flow[i] < st_trigger_data.flow_gate_min_p)
        {
            flow_cnt = 0;// 不连续时，要清零
        }
        else if(st_trigger_data.flow[i] > st_trigger_data.flow_gate_max_p)
        {
            flow_cnt += st_trigger_data.flow_gate_scale_p >> 2; // 触发时，流量比压力更敏感，这里极限放大了超过流量阈值上限后的检测值，以加快检测结果。
            // 但压力可能更好测试，需要等新版硬件更换后再做处理
        }
        else
        {
            flow_cnt += st_trigger_data.flow[i] - st_trigger_data.flow_gate_min_p;
        }


        // ！！！ 非常重要：这个是参照旧版硬件（压力传感器采样在机器内部），若换作新版硬件（在口端进行压力采样），需要重新考量数据及阈值
        // 压力连续超过门限
        //A 连续8个压力数据超过门限，视为吸气触发
        if(st_trigger_data.press[i] > st_trigger_data.press_gate_min_n)
        {
            press_cnt = 0;
        }
        else if(st_trigger_data.press[i] < st_trigger_data.press_gate_max_n)
        {
            press_cnt += st_trigger_data.press_gate_scale_n;
        }
        else
        {
            press_cnt += st_trigger_data.press_gate_min_p - st_trigger_data.press[i];
        }

        // correct locate
        inc_trigger_locate(locate);
    }
    // 求平均值 /MAX_TRIGGER_DETECTED_NUM
    press_cnt >>= 3;
    flow_cnt >>= 3;
    // 判断

    return EM_FALSE;
}

/**
 * [trigger_simple_set_base_p description]
 * @method trigger_simple_set_base_p
 * @param  flow                      [description]
 * @param  press                     [description]
 */
void trigger_simple_set_base_p(int32_t flow, int16_t press)
{
    st_trigger_data.flow_base_p      = flow;
    st_trigger_data.press_base_p     = press;
    st_trigger_data.flow_gate_max_p  = flow / 100 + 100;
    st_trigger_data.flow_gate_min_p  = flow / 100 + 10;
    st_trigger_data.press_gate_max_p = press + 100;
    st_trigger_data.press_gate_min_p = press + 10;
    refresh_trigger_gate_scale();
}

/**
 * [trigger_simple_set_base_n description]
 * @method trigger_simple_set_base_n
 * @param  flow                      [description]
 * @param  press                     [description]
 */
void trigger_simple_set_base_n(int32_t flow, int16_t press)
{
    st_trigger_data.flow_base_n      = flow;
    st_trigger_data.press_base_n     = press;
    st_trigger_data.flow_gate_max_n  = flow / 100 - 100;
    st_trigger_data.flow_gate_min_n  = flow / 100 - 10;
    st_trigger_data.press_gate_max_n = press - 100;
    st_trigger_data.press_gate_min_n = press - 10;
    refresh_trigger_gate_scale();
}

/**
 * [trigger_simple_set_base_n description] 设置CPAP吸气触发门限
 * @method trigger_simple_set_base_n
 * @param  flow                      [description] 基本流量(单位ml/min)，实际检测时，会在高于门限1000ml/min
 * @param  press                     [description] 基本压力(单位0.1cmH2O) 实际检测时，会比门限低1.0 cmH2O (10)
 */
void trigger_simple_set_base_inhale(int32_t flow, int16_t press)
{
    st_trigger_data.flow_base_p      = flow;
    st_trigger_data.press_base_n     = press;
    st_trigger_data.flow_gate_max_p  = flow / 100 + 100;
    st_trigger_data.flow_gate_min_p  = flow / 100 + 10;
    st_trigger_data.press_gate_max_n = press - 100;
    st_trigger_data.press_gate_min_n = press - 10;
    refresh_trigger_gate_scale();
}

/**
 * [trigger_simple_detect_inhale description]
 * @method trigger_simple_detect_inhale
 * @param  flow1                         [description]
 * @param  press                         [description]
 */
void trigger_simple_detect_inhale(int32_t flow1, int16_t press)
{
    int32_t flow;

    // flow must devided by 100
    flow = flow1 / 100;
    if(flow < st_trigger_data.flow_gate_min_p)
    {
        st_trigger_data.flow_status_inhale = 0;
    }
    else if(flow > st_trigger_data.flow_gate_max_p)
    {
        // 超出设定范围时，使用最大值 （限制幅度）  确保为正值
        st_trigger_data.flow_status_inhale += st_trigger_data.flow_gate_scale_p >> 2;
    }
    else
    {
        // 要确保数据为正值
        st_trigger_data.flow_status_inhale += flow - st_trigger_data.flow_gate_min_p;
    }

    // 压力连续超过门限
    //A 连续8个压力数据超过门限，视为吸气触发
    if(display_get_Pmean() > main_control_data.epap-15)
    {
	    if(press > st_trigger_data.press_gate_min_n)
	    {
	        // 未满足要求时，清除积累
	        st_trigger_data.press_status_inhale = 0;
	    }
	    else if(press < st_trigger_data.press_gate_max_n)
	    {
	        // 超出设定范围时，使用最大值 （限制幅度）  确保为正值
	        st_trigger_data.press_status_inhale += st_trigger_data.press_gate_scale_n;
	    }
	    else
	    {
	        // 要确保数据为正值
	        st_trigger_data.press_status_inhale += st_trigger_data.press_gate_min_n - press;
	    }
    }
	else
	{
		// 当气道压力降到超出触发阈值时，会产生误触发（此时应该调整转速以恢复压力）
    	st_trigger_data.press_status_inhale = 0; //张志新：20190806 仅仅用于测试
	}
    // 重要：在这里应该是 2个正值(或0)相加
    st_trigger_data.inhale_status = st_trigger_data.flow_status_inhale + st_trigger_data.press_status_inhale;
    st_trigger_data.inhale_status >>= 3;
}

/**
 * [trigger_simple_set_base_exhale description]
 * @method trigger_simple_set_base_exhale
 * @param  flow                           [description]
 * @param  press                          [description]
 */
void trigger_simple_set_base_exhale(int32_t flow, int16_t press)
{
    st_trigger_data.flow_base_n      = flow;
    st_trigger_data.press_base_p     = press;
    st_trigger_data.flow_gate_max_n  = flow / 100 - 100;
    st_trigger_data.flow_gate_min_n  = flow / 100 - 10;
    st_trigger_data.press_gate_max_p = press + 100;
    st_trigger_data.press_gate_min_p = press + 10;
    refresh_trigger_gate_scale();
}

/**
 * [trigger_detect_exhale description]
 * @method trigger_detect_exhale
 */
void trigger_simple_detect_exhale(int32_t flow1, int16_t press)
{
    //return EM_TRIGGER_TYPE_E_EXP
    int32_t flow;
    //uint8_t num;
    // flow must devided by 100
    flow = flow1 / 100;
    if(flow > st_trigger_data.flow_gate_min_n)
    {
        // 未满足要求时，清除
        st_trigger_data.flow_status_exhale = 0;
    }
    else if(flow < st_trigger_data.flow_gate_max_n)
    {
        // 超出范围时，选用最大值
        st_trigger_data.flow_status_exhale += st_trigger_data.flow_gate_scale_n; //=st_trigger_data.flow_gate_min_n-st_trigger_data.flow_gate_max_n
    }
    else
    {
        st_trigger_data.flow_status_exhale += st_trigger_data.flow_gate_min_n - flow;
    }

    // 压力连续超过门限
    //A 连续8个压力数据超过门限，视为吸气触发
    if(press < st_trigger_data.press_gate_min_p)
    {
        // 未满足要求时，清除积累
        st_trigger_data.press_status_exhale = 0;
    }
    else if(press > st_trigger_data.press_gate_max_p)
    {
        // 超出设定范围时，使用最大值 （限制幅度）  确保为正值
        st_trigger_data.press_status_exhale += st_trigger_data.press_gate_scale_p;
    }
    else
    {
        // 确保为正值
        st_trigger_data.press_status_exhale +=  press - st_trigger_data.press_gate_min_p;
    }

    st_trigger_data.exhale_status = st_trigger_data.flow_status_exhale + st_trigger_data.press_status_exhale;
    st_trigger_data.exhale_status >>= 3;	// 暂定除8
}

/**
 * [start_trigger description]
 * @method start_trigger
 * @param  type          [description]
 */
void start_trigger(uint8_t type)
{
    reset_trigger_detected_data(courent_counted_flow, courent_counted_press);
    st_trigger_data.type = type;

    // 在启动触发时同步进行的准备工作
    switch(type)
    {
    default:
    case EM_TRIGGER_TYPE_IDLE:
        // do nothing
        break;

    case EM_TRIGGER_TYPE_E_EXP: // 水平压力，检测主动呼气
        break;

    case EM_TRIGGER_TYPE_E_INS: // 水平压力，检测主动吸气
        break;

    case EM_TRIGGER_TYPE_E_EXP_INS:
        break;

    case EM_TRIGGER_TYPE_CPAP_INS_FINISHED: // = 4, // CPAP 检测吸气结束
        break;

    case EM_TRIGGER_TYPE_CPAP_EXP_FINISHED: // = 5, // CPAP 检测呼气结束
        break;
    }
}

/**
 * [stop_trigger description]
 * @method stop_trigger
 */
void stop_trigger(void)
{
    // 只清除工作模式，保留触发检测数据---可能没啥用
    st_trigger_data.type = EM_TRIGGER_TYPE_IDLE;
}

/**
 * [trigger_set_next_control description]
   在一个检测触发周期内，原则上禁止再对系统进行设置，
   但是，总有例外，当新的设置生效时，需要考虑如何修
   正触发检测条件，以避免因控制改变造成的错误---比如误触发或无法触发
 * @method trigger_set_next_control
 */
void trigger_set_next_control(void)
{
    switch(st_trigger_data.type)
    {
    default:
    case EM_TRIGGER_TYPE_IDLE:
        // do nothing
        break;

    case EM_TRIGGER_TYPE_E_EXP: // 水平压力，检测主动呼气
        break;

    case EM_TRIGGER_TYPE_E_INS: // 水平压力，检测主动吸气
        break;

    case EM_TRIGGER_TYPE_E_EXP_INS:
        break;

    case EM_TRIGGER_TYPE_CPAP_INS_FINISHED: // = 4, // CPAP 检测吸气结束
        break;

    case EM_TRIGGER_TYPE_CPAP_EXP_FINISHED: // = 5, // CPAP 检测呼气结束
        break;
    case EM_TRIGGER_TYPE_STV_INS_FINISHED:  // = 6, // ST 检测吸气结束
        break;
    case EM_TRIGGER_TYPE_STV_EXP_FINISHED:  // = 7, // ST 检测呼气结束
        break;
    }
}

/**
 * [trigger_set_cpap_flow_for_inhale_finished description]
 * @method trigger_set_cpap_flow_for_inhale_finished
 * @param  flow                                      [description]
 */
void trigger_set_cpap_flow_for_inhale_finished(int32_t flow)
{
    if(flow > TRIGGER_INHALE_FLOW_GATE)
    {
        // inhale is not finished
        st_trigger_data.flow_base_p = 0;
        st_trigger_data.start_position = 0;
    }
    else
    {
        if(st_trigger_data.start_position < 200)
        {
            st_trigger_data.start_position++;
            st_trigger_data.flow_base_p += flow >> 3; // divided by MAX_TRIGGER_DETECTED_NUM
        }
        else
        {
            st_trigger_data.start_position >>= 3;
            st_trigger_data.flow_base_p >>= 3;
        }
    }
}

/**
 * [trigger_set_cpap_flow_for_exhale_finished description]
 * @method trigger_set_cpap_flow_for_exhale_finished
 * @param  flow                                      [description]
 */
void trigger_set_cpap_flow_for_exhale_finished(int32_t flow)
{
    if(flow < TRIGGER_EXHALE_FLOW_GATE)
    {
        // inhale is not finished
        st_trigger_data.flow_base_n = 0;
        st_trigger_data.start_position = 0;
    }
    else
    {
        if(st_trigger_data.start_position < 200)
        {
            st_trigger_data.start_position++;
            st_trigger_data.flow_base_n += flow / 8; // divided by MAX_TRIGGER_DETECTED_NUM
        }
        else
        {
            st_trigger_data.start_position >>= 3;
            st_trigger_data.flow_base_n /= 8; // 负数移位做除法可能会有问题
        }
    }
}

/**
 * [trigger_set_stv_flow_for_inhale_finished description]
 * @method trigger_set_stv_flow_for_inhale_finished
 * @param  flow                                     [description]
 */
void trigger_set_stv_flow_for_inhale_finished(int32_t flow)
{
    int32_t peak_flow;

    // 如果流量为0(<2L/min) 则直接成立
    if(flow <= TRIGGER_INHALE_FLOW_GATE)
    {
        st_trigger_data.inhale_status += 4;
        return;
    }

    // 需要峰值流量
    peak_flow = breath_get_inhale_peak_Flow();

    // 这里仅判断是峰值流速的一半，将来需要使用自动判断的流速门限
    // 需要重新改写
    peak_flow >>= 1; // 除以2
    if(flow < peak_flow)
    {
        st_trigger_data.inhale_status ++;
    }
    else
    {
        st_trigger_data.inhale_status = 0;
    }
    // 不检测压力
}

/**
 * [trigger_set_stv_flow_for_exhale_finished description]
 * @method trigger_set_stv_flow_for_exhale_finished
 * @param  flow                                     [description]
 */
void trigger_set_stv_flow_for_exhale_finished(int32_t flow)
{
    if(flow < TRIGGER_EXHALE_FLOW_GATE)
    {
        // inhale is not finished
        st_trigger_data.flow_base_n = 0;
        st_trigger_data.start_position = 0;
    }
    else
    {
        if(st_trigger_data.start_position < 200)
        {
            st_trigger_data.start_position++;
            st_trigger_data.flow_base_n += flow / MAX_TRIGGER_DETECTED_NUM; // divided by MAX_TRIGGER_DETECTED_NUM
        }
        else
        {
            st_trigger_data.start_position >>= 3;
            st_trigger_data.flow_base_n /= MAX_TRIGGER_DETECTED_NUM; // 负数移位做除法可能会有问题
        }
    }
}

/**
 * [trigger_set_current_status description]
 * 设置当前的状态信息，同时检测触发条件，状态信息包括流量和压力，(默认的还有时间--这个很重要)
 * @method trigger_set_current_status
 */
void trigger_set_current_status(void)
{
    switch(st_trigger_data.type)
    {
    default:
    case EM_TRIGGER_TYPE_IDLE:
        // do nothing
        break;

    case EM_TRIGGER_TYPE_E_EXP: // 水平压力，检测主动呼气
        trigger_simple_detect_exhale(courent_counted_flow, courent_counted_press);
        break;

    case EM_TRIGGER_TYPE_E_INS: // 水平压力，检测主动吸气
        //trigger_detect_inhale();
        trigger_simple_detect_inhale(courent_counted_flow, courent_counted_press);
        break;

    case EM_TRIGGER_TYPE_E_EXP_INS:
        trigger_simple_detect_inhale(courent_counted_flow, courent_counted_press);
        trigger_simple_detect_exhale(courent_counted_flow, courent_counted_press);
        break;

    case EM_TRIGGER_TYPE_CPAP_INS_FINISHED: // = 4, // CPAP 检测吸气结束
        trigger_set_cpap_flow_for_inhale_finished(courent_counted_flow);
        break;

    case EM_TRIGGER_TYPE_CPAP_EXP_FINISHED: // = 5, // CPAP 检测呼气结束
        trigger_set_cpap_flow_for_exhale_finished(courent_counted_flow);
        break;

    case EM_TRIGGER_TYPE_STV_INS_FINISHED:  // = 6, // ST 检测吸气结束
        trigger_set_stv_flow_for_inhale_finished(courent_counted_flow);
        break;
    case EM_TRIGGER_TYPE_STV_EXP_FINISHED:  // = 7, // ST 检测呼气结束
        trigger_set_stv_flow_for_exhale_finished(courent_counted_flow);
        break;
    }
}

/**
 * [is_trigger_inhale_finished description]
 * @method is_trigger_inhale_finished
 * @return                            [description]
 */
uint8_t is_trigger_inhale_finished(void)
{
    int32_t flow;
    if(st_trigger_data.start_position < 16) return EM_FALSE;
    flow = st_trigger_data.flow_base_p / st_trigger_data.start_position;
    if(flow < TRIGGER_INHALE_FLOW_GATE) // 待最后确认
        return EM_TRUE;
    return EM_FALSE;
}

/**
 * [is_trigger_exhale_finished description]
 * @method is_trigger_exhale_finished
 * @return                            [description]
 */
uint8_t is_trigger_exhale_finished(void)
{
    int32_t flow;
    if(st_trigger_data.start_position < 16) return EM_FALSE;
    flow = st_trigger_data.flow_base_n / st_trigger_data.start_position;
    if(flow > TRIGGER_EXHALE_FLOW_GATE) // 待最后确认
        return EM_TRUE;
    return EM_FALSE;
}

/**
 * [is_trigger_stv_inhale_finished description]
 * @method is_trigger_stv_inhale_finished
 * @return                                [description]
 */
uint8_t is_trigger_stv_inhale_finished(void)
{
    if(st_trigger_data.inhale_status >= 8)
        return EM_TRUE;
    return EM_FALSE;
}

/**
 * [is_trigger_stv_exhale_finished description]
 * @method is_trigger_stv_exhale_finished
 * @return                                [description]
 */
uint8_t is_trigger_stv_exhale_finished(void)
{
    int32_t flow;
    if(st_trigger_data.start_position < 16) return EM_FALSE;
    flow = st_trigger_data.flow_base_n / st_trigger_data.start_position;
    if(flow > TRIGGER_EXHALE_FLOW_GATE) // 待最后确认
        return EM_TRUE;
    return EM_FALSE;
}

/**
 * [is_patient_actived_breath description]
 * @method is_patient_actived_breath
 * @return                           [description]
 */
uint8_t is_patient_actived_breath(void)
{
    switch(st_trigger_data.type)
    {
    default:
    case EM_TRIGGER_TYPE_IDLE:
        break;

    case EM_TRIGGER_TYPE_E_EXP: // 水平压力，检测主动呼气
        return is_trigger_exhale_actived();
    //break;

    case EM_TRIGGER_TYPE_E_INS: // 水平压力，检测主动吸气
        return is_trigger_inhale_actived();

    case EM_TRIGGER_TYPE_E_EXP_INS:
        if(is_trigger_inhale_actived())
        {
            return 1; // inhale active
        }
        else if(is_trigger_exhale_actived())
        {
            return 2; // exhale active
        }
        else return EM_FALSE; // no active

    case EM_TRIGGER_TYPE_CPAP_INS_FINISHED: // = 4, // CPAP 检测吸气结束
        return is_trigger_inhale_finished();
    //break;

    case EM_TRIGGER_TYPE_CPAP_EXP_FINISHED: // = 5, // CPAP 检测呼气结束
        return is_trigger_exhale_finished();
    //break;

    case EM_TRIGGER_TYPE_STV_INS_FINISHED:  // = 6, // ST 检测吸气结束
        return is_trigger_stv_inhale_finished();
    //break;

    case EM_TRIGGER_TYPE_STV_EXP_FINISHED:  // = 7, // ST 检测呼气结束
        return is_trigger_stv_exhale_finished();
        //break;
    }
    return EM_FALSE;
}

/** *********************************************************************************
 * auto trigger，
 * 在连续3个有效自主呼吸（触发）之后，要停止自动触发模式的调整，（高频呼吸时，每3个呼吸
 * 周期降低一次灵敏度）以检测是否有错误的触发状态。
 * 启动自动触发模式，自动触发会在临近患者呼吸（频率）节奏的时段，适当提高灵敏度，
 * 当连续发生时控模式之后，需要改变和调整灵敏度，以适应更微弱的触发信号
 * 在没有自主呼吸的情况下，考虑在T模式开始前，呼气时间的1/3 时段内，提高出发灵敏度
 * --> 要充分考虑泄漏带来的影响
 * --> 要考虑患者潮气量的影响
 * --> 要考虑患者频率的影响
 * --> 要考虑患者控制目标压力的影响
 * --> 考虑患者峰值流量的影响
 * --> 考虑患者R/C参数的影响
 * --> 高频率触发时要考虑适当降低灵敏度（减少误触发）
 * **********************************************************************************
 */
/**
 * [auto_trigger_adjust_stv_gate description]
 * @method auto_trigger_adjust_stv_gate
 */
void auto_trigger_adjust_stv_gate(void)
{
}
/**
 * [auto_trigger_append_inhale_status description]
 * @method auto_trigger_append_inhale_status
 */
void auto_trigger_append_inhale_status(void)
{
}
/**
 * [auto_trigger_append_exhale_status description]
 * @method auto_trigger_append_exhale_status
 */
void auto_trigger_append_exhale_status(void)
{
}

// file end
