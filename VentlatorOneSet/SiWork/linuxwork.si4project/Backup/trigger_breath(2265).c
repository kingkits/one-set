// File:trigger_breath.c
typedef enum
{
    EM_TRIGGER_TYPE_IDLE,
    EM_TRIGGER_TYPE_E_EXP, // 水平压力，检测主动呼气
    EM_TRIGGER_TYPE_E_INS, // 水平压力，检测主动吸气
    EM_TRIGGER_TYPE_E_EXP_INS
} ENUM_TRIGGER_DETECTED_TYPES;
#define MAX_TRIGGER_BUFFER_LEN 64
typedef struct
{
    int16_t press[MAX_TRIGGER_BUFFER_LEN];
    int32_t flow[MAX_TRIGGER_BUFFER_LEN];
    uint8_t type;
    uint8_t start_position;
    uint8_t stop_position;

    uint8_t cur_status;
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
 * @method trigger_set_next_control
 */
void trigger_set_next_control(void)
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
