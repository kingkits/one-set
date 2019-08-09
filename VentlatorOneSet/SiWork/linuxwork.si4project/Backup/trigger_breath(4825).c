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
    int16_t press[MAX_TRIGGER_BUFFER_LEN];
    int32_t flow[MAX_TRIGGER_BUFFER_LEN];
    uint8_t type; // 需要检测的模式
    uint8_t start_position;
    uint8_t stop_position;

    uint8_t cur_status; // 当前的触发状态
} ST_TRIGGER_DETECTED_DATA;
ST_TRIGGER_DETECTED_DATA st_trigger_data;

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
        st_trigger_data.press[i] = press;
        st_trigger_data.flow[i] = flow;
    }
    st_trigger_data.cur_status = EM_TRIGGER_TYPE_IDLE;
    st_trigger_data.start_position = 0;
    st_trigger_data.stop_position = 0;
    st_trigger_data.type = EM_TRIGGER_TYPE_IDLE;
}

/**
 * [start_trigger description]
 * @method start_trigger
 * @param  type          [description]
 */
void start_trigger(uint8_t type)
{
    reset_trigger_detected_data();
    st_trigger_data.type = type;
}


uint16_t trigger_detect_inspire(void)
{
    //return EM_TRIGGER_TYPE_E_INS
    return 0;
}


uint16_t trigger_detect_expire(void)
{
    //return EM_TRIGGER_TYPE_E_EXP
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
 * [trigger_set_next_control description]在一次
 * @method trigger_set_next_control
 */
void trigger_set_next_control(void)
{
    //uint8_t status;
    switch(st_trigger_data.type)
    {
    default:
    case EM_TRIGGER_TYPE_IDLE:
        // do nothing
        break;
    case EM_TRIGGER_TYPE_E_EXP: // 水平压力，检测主动呼气
        st_trigger_data.cur_status |= trigger_detect_expire();
        break;
    case EM_TRIGGER_TYPE_E_INS: // 水平压力，检测主动吸气
        st_trigger_data.cur_status |= trigger_detect_inspire();
        break;
    case EM_TRIGGER_TYPE_E_EXP_INS:
        st_trigger_data.cur_status |= trigger_detect_inspire() + trigger_detect_expire();
        break;
    }
}

/**
 * [trigger_set_current_status description]
 * @method trigger_set_current_status
 */
void trigger_set_current_status()
{
    switch(st_trigger_data.type)
    {
    default:
    case EM_TRIGGER_TYPE_IDLE:
        break;
    case EM_TRIGGER_TYPE_E_EXP: // 水平压力，检测主动呼气
        break;
    case EM_TRIGGER_TYPE_E_INS: // 水平压力，检测主动吸气
        break;
    case EM_TRIGGER_TYPE_E_EXP_INS:
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
        break;
    case EM_TRIGGER_TYPE_E_EXP_INS:
        break;
    }

    return 0;
}
// file end
