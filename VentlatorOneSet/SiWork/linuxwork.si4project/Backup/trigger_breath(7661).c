// File:trigger_breath.c
typedef enum
{
    EM_TRIGGER_TYPE_IDLE,
    EM_TRIGGER_TYPE_E_EXP     = 1, // 水平压力，检测主动呼气
    EM_TRIGGER_TYPE_E_INS     = 2, // 水平压力，检测主动吸气
    EM_TRIGGER_TYPE_E_EXP_INS = 3
} ENUM_TRIGGER_DETECTED_TYPES;
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
    int16_t press_status;
    int16_t flow_status;
    uint16_t inshale_status;                // 当前的吸气触发状态
    uint16_t exhale_status;                 // 当前的呼气触发状态
} ST_TRIGGER_DETECTED_DATA;
ST_TRIGGER_DETECTED_DATA st_trigger_data;

#define MAX_TRIGGER_DETECTED_NUM 8
#define dec_trigger_locate(locate) 		if(st_trigger_data.stop_position)locate --;	else locate = (MAX_TRIGGER_BUFFER_LEN - 1);
#define inc_trigger_locate(locate) 		if(st_trigger_data.stop_position == (MAX_TRIGGER_BUFFER_LEN - 1))locate = 0;else locate++;
#define is_trigger_inhale_actived()     (st_trigger_data.inshale_status > st_trigger_data.flow_gate_scale_p + st_trigger_data.press_gate_scale_n)
/**
 * [reset_trigger_gate description]
 * @method reset_trigger_gate
 */
void reset_trigger_gate(void)
{

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
 * [refresh_trigger_gate_scale description]
 * @method refresh_trigger_gate_scale
 */
void refresh_trigger_gate_scale(void)
{
    st_trigger_data.flow_gate_scale_p = st_trigger_data.flow_gate_max_p - st_trigger_data.flow_gate_min_p;
    st_trigger_data.flow_gate_scale_n = st_trigger_data.flow_gate_min_n - st_trigger_data.flow_gate_max_n;

    st_trigger_data.press_gate_scale_p = st_trigger_data.press_gate_max_p - st_trigger_data.press_gate_min_p;
    st_trigger_data.press_gate_scale_n = st_trigger_data.press_gate_min_n - st_trigger_data.press_gate_max_n;
}
/**
 * [reset_trigger_detected_data description]
 * @method reset_trigger_detected_data
 * @param  press                       [description]
 * @param  flow                        [description]
 */
void reset_trigger_detected_data(int16_t press, int32_t flow)
{
    int i;
    for(i = 0; i < MAX_TRIGGER_BUFFER_LEN; i++)
    {
        st_trigger_data.press[i]   = press;
        st_trigger_data.flow[i]    = flow;
    }
    st_trigger_data.inshale_status = 0;
    st_trigger_data.exhale_status  = 0;
    st_trigger_data.start_position = 0;
    st_trigger_data.stop_position  = 0;
    st_trigger_data.type           = EM_TRIGGER_TYPE_IDLE;

    // temp data
    st_trigger_data.flow_status = 0;
    st_trigger_data.press_status = 0;
}

/**
 * [get_trigger_data_counts description]
 * @method get_trigger_data_counts
 * @return                         [description]
 */
uint8_t get_trigger_data_counts(void)
{
    uint8_t num;
    if(st_trigger_data.start_position == st_trigger_data.stop_position) return 0;
    num  = (uint8_t) st_trigger_data.stop_position - st_trigger_data.start_position;
    num &= (MAX_TRIGGER_BUFFER_LEN - 1);
}

/**
 * [start_trigger description]
 * @method start_trigger
 * @param  type          [description]
 */
void start_trigger(uint8_t type)
{
    reset_trigger_detected_data(display_count_data.Flow, display_count_data.Press);
    st_trigger_data.type = type;
}

/**
 * [get_trigger_prenum_locate description]
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
 * [trigger_detect_inspire description]
 * @method trigger_detect_inspire
 * @return                        [description]
 */
uint16_t trigger_detect_inspire(void)
{
    uint8_t locate;
    uint8_t press_cnt;
    uint8_t flow_cnt;
    uint8_t num;
    int i;
    //return EM_TRIGGER_TYPE_E_INS
    num = get_trigger_data_counts();
    if(num < 16) return 0;

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
    i = 0;
    press_cnt = 0;
    flow_cnt = 0;

    while(i++ < MAX_TRIGGER_DETECTED_NUM)
    {
        // 吸气流量连续超过门限
        //A 连续8个流量数据超过门限，视为吸气触发
        if(st_trigger_data.flow[i] < st_trigger_data.flow_gate_min_p)
        {
            flow_cnt = 0;
        }
        else if(st_trigger_data.flow[i] > st_trigger_data.flow_gate_max_p)
        {
            flow_cnt += st_trigger_data.flow_gate_scale_p;
        }
        else
        {
            flow_cnt += st_trigger_data.flow[i] - st_trigger_data.flow_gate_min_p;
        }

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

    return 0;
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
    st_trigger_data.flow_gate_max_n  = flow / 100 + 100;
    st_trigger_data.flow_gate_min_n  = flow / 100 + 10;
    st_trigger_data.press_gate_max_n = press + 100;
    st_trigger_data.press_gate_min_n = press + 10;
}

/**
 * [trigger_simple_detect_inspire description]
 * @method trigger_simple_detect_inspire
 * @param  flow1                         [description]
 * @param  press                         [description]
 */
void trigger_simple_detect_inspire(int32_t flow1, int16_t press)
{
    int32_t flow;

    // flow must devided by 100
    flow = flow1 / 100;
    if(flow < st_trigger_data.flow_gate_min_p)
    {
        st_trigger_data.flow_status = 0;
    }
    else if(flow > st_trigger_data.flow_gate_max_p)
    {
        st_trigger_data.flow_status += st_trigger_data.flow_gate_scale_p;
    }
    else
    {
        st_trigger_data.flow_status += st_trigger_data.flow[i] - st_trigger_data.flow_gate_min_p;
    }

    // 压力连续超过门限
    //A 连续8个压力数据超过门限，视为吸气触发
    if(press > st_trigger_data.press_gate_min_n)
    {
        st_trigger_data.press_status = 0;
    }
    else if(press < st_trigger_data.press_gate_max_n)
    {
        st_trigger_data.press_status += st_trigger_data.press_gate_scale_n;
    }
    else
    {
        st_trigger_data.press_status += st_trigger_data.press_gate_min_p - st_trigger_data.press[i];
    }

    st_trigger_data.flow_status >>= 3;
    st_trigger_data.press_status >>= 3;
    st_trigger_data.inshale_status = st_trigger_data.flow_status + st_trigger_data.press_status;
}

/**
 * [trigger_detect_expire description]
 * @method trigger_detect_expire
 * @return                       [description]
 */
uint16_t trigger_detect_expire(void)
{
    //return EM_TRIGGER_TYPE_E_EXP
    uint8_t num;
    num = get_trigger_data_counts();
    if(num < 16) return 0;

    return 0;
}

/**
 * [stop_trigger description]
 * @method stop_trigger
 */
void stop_trigger(void)
{
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

}

/**
 * [trigger_set_current_status description]
 * 设置当前的状态信息，同时检测触发条件，状态信息包括流量和压力，(默认的还有时间--这个很重要)
 * @method trigger_set_current_status
 */
void trigger_set_current_status()
{
    switch(st_trigger_data.type)
    {
    default:
    case EM_TRIGGER_TYPE_IDLE:
        // do nothing
        break;
    case EM_TRIGGER_TYPE_E_EXP: // 水平压力，检测主动呼气
        trigger_detect_expire();
        break;
    case EM_TRIGGER_TYPE_E_INS: // 水平压力，检测主动吸气
        //trigger_detect_inspire();
        trigger_simple_detect_inspire(display_count_data.Flow, display_count_data.Press);
        break;
    case EM_TRIGGER_TYPE_E_EXP_INS:
        trigger_detect_inspire();
        trigger_detect_expire();
        break;
    }
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
        break;
    case EM_TRIGGER_TYPE_E_INS: // 水平压力，检测主动吸气
        if(is_trigger_inhale_actived()) return 1;
        break;
    case EM_TRIGGER_TYPE_E_EXP_INS:
        break;
    }

    return 0;
}
// file end
