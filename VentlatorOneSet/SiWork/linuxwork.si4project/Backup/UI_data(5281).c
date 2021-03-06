
#include "stm32f4xx_hal.h"
#include "cmsis_os.h"
#include "stm32f4xx_it.h"
#include "../global.h"
#include "../ModeControl/breath_rap.h"
#include "../GPIO/gpio.h"

#include "../blower/blower.h"
#include "../OxygenMixer/OxygenMixter.h"
#include "../ControlTask/base_control.h"
#include "../ADC/adc_manager.h"
#include "../ADC/Adc_data_solution.h"
//#include "../ADC/Calibration.h"

//#include "../ai/breath_AI.h"

#define extern
///////////////////////////////////////////////////////////////
// fumction declares
///////////////////////////////////////////////////////////////
void     count_blower_speed(void);
int32_t  get_oxygen_flow(void);
void     uart3_put_char(uint8_t ch);
void     set_peep_valve_control_val(uint16_t val);
uint8_t  get_ui_byte(char *byteptr);
void     ui_set_press_sensor_data(uint8_t *bufptr);

////////////////////////////////////////////////////////////////

typedef enum
{
    BASE_HANDSHAKE_MSG           = 0x0,
    BASE_ACK_MSG                 = 0xA,
    BASE_PFV_MSG                 = 0x10,
    BASE_BREATH_DATA_MSG         = 0x11,
    BASE_BREATH_TRIGGER_I_MSG    = 0x12,
    BASE_BREATH_TRIGGER_E_MSG    = 0x13,
    BASE_BREATH_OXYGEN_MSG       = 0x14,
    BASE_BREATH_SECOND_VT_MSG    = 0x15,
    BASE_COUGH_SECOND_VT_MSG     = 0x16,
    BASE_BOARD_STATUS            = 0x20,
    BASE_SENSOR_DATA_MSG         = 0x27,//上传 传感器数据		0x27
    BASE_SET_PRESS_SENSOR_ACK_MSG = 0x28, //设置压力传感器校准值的 ACK		0x28
    BASE_PRESS_SENSOR_DATA_MSG   = 0x29,//上传压力传感器校准数据		0x29
    BASE_SET_FLOW_SENSOR_ACK_MSG = 0x2A,//设置流量传感器校准值的ACK		0x2A
    BASE_FLOW_SENSOR_DATA_MSG    = 0x2B,//上传流量传感器校准值		0x2B
    BASE_SET_LIST_ACK_MSG        = 0x2C,//设置可控制单元的校准值的ACK   0x2C
    BASE_LIST_DATA_MSG           = 0x2D,//上传可控制单元的校准值		0x2D
    BASE_SET_OXYGEN_ACK_MSG      = 0x2E,//设置氧传感器校准值的ACK		0x2E
    BASE_OXYGEN_SENSOR_DATA_MSG  = 0x2F,//读取氧传感器校准值   0x2F
    BASE_AIC_INFORMATION_MSG     = 0x74,
    BASE_OXYGEN_MIXER_CONTROL    = 0xD8,
    BASE_OXYGEN_MIXER_INFO_MSG   = 0xD9,
    BASE_VALVE_INFO_MSG          = 0xDB,
    BASE_PID_ADJUST_INFO_MSG     = 0xE0,
    BASE_PPFFVV_MSG              = 0xE2
} EM_BASE_MESSAGE_TYPES;

typedef enum
{
    UI_HANDSHAKE_COM             = 0x0,
    UI_STANDBY_MODE_COM          = 0x20,
    UI_STANDBY_SET_INVASIVE_COM  = 0x21,//	插管状态		0x21	字节1：	 invasive_support;			插管状态：      =0 无创通气			   =1：有创通气
    UI_STANDBY_SET_AIR_TYPE_COM  = 0x22,//  气体标准		0x22	字节1：	 air_type;					气体标准：      =0：ATP           =1：BTPS
    UI_STANDBY_SET_PATIENT_COM   = 0x23,//	患者类型		0x23	字节1：	 patient_types;       患者类型：      =0：Adult				 =1：Children/=2：Infant
    UI_STANDBY_SET_PIPE_TYPE_COM = 0x24, //  管路连接形式		0x24	字节1：	 pipe_connect_type;     管路连接形式： =0：双管				  =1：单管
    UI_STANDBY_SET_PIPE_C_COM    = 0x25,//  路顺应性      0x25	字节1：	 breathing_circuit_C;	管路顺应性：	  0.1cmH2O
    UI_STANDBY_SET_DIESAESE_COM  = 0x26,//	症状		    0x26	字节1：	 body_disease;				症状					 =0: (正常)Normal    =1:(特殊)ARDS	 /=2:(特殊)COPD/etc.
    UI_REAL_SENSOR_DATA_COM      = 0x27,//  读取传感器数据	              0x27	无 "此指令用于启动传感器上传，如果主控板接收这个指令。会持续上传传感器数据,直到UI发出任意模式控制指令（比如STANDBY指令0x20）才会停止"
    UI_SET_PRESS_SENSOR_DATA_COM = 0x28, //	设置压力传感器 校准值           0x28
    UI_GET_PRESS_SENSOR_DATA_COM = 0x29, //	读取压力传感器校准数据		        0x29
    UI_SET_FLOW_SENSOR_DATA_COM  = 0x2A,//	设置流量传感器校准值		        0x2A
    UI_GET_FLOW_SENSOR_DATA_COM  = 0x2B,//	读取流量传感器校准值		        0x2B
    UI_SET_LIST_UNIT_DATA_COM    = 0x2C,//	设置可控制单元的校准值		        0x2C
    UI_GET_LIST_UNIT_DATA_COM    = 0x2D,//	读取可控制单元的校准值		        0x2D
    UI_SET_OXYGEN_SENSOR_DATA_COM = 0x2E, //	设置氧传感器校准值               0x2E
    UI_GET_OXYGEN_SENSOR_DATA_COM = 0x2F, //	读取氧传感器校准值               0x2F

    UI_CPAP_MODE_COM             = 0x31,
    UI_COUGH_START_COM           = 0x32,
    UI_COUGH_REPEAT_COM          = 0x33,
    UI_COUGH_STOP_COM        = 0x34,
    UI_ST_MODE_COM               = 0x42,
    UI_PCV_MODE_COM              = 0x43,
    UI_SET_EPAP_COM            = 0x60,
    UI_SET_FREQ_COM            = 0x61,
    UI_SET_IPAP_COM              = 0x62,
    UI_START_RAP_COM             = 0x78,
    UI_STOP_RAP_COM              = 0x79,
    UI_START_NEBULIZER_COM       = 0x80,
    UI_STOP_NEBULIZER_COM        = 0x81,
    UI_STOP_COMMUNICATION_COM    = 0xD0,
    UI_START_COMMUNICATION_COM   = 0xD1,
    UI_STOP_O2_MIXER_COM         = 0xD2,
    UI_START_O2_MIXTER_COM       = 0xD3,
    UI_CHANGE_BLOWER_COM         = 0xD4,
    UI_CHANGE_OXYGEN_COM         = 0xD5,
    UI_CHANGE_OVERFLOW_VALVE_COM = 0xD6,
    UI_CHANGE_VALVE_COM          = 0xDA,
    UI_CHANGE_PEEP_VALVE_COM     = 0xDC
} EM_UI_COMMAND_TYPES;


#define UI_PROTOCOL_TRANSFER_CODE 0xfd
#define UI_PROTOCOL_END           0xfe
#define UI_PROTOCOL_HEAD          0xff

#include "Ui_data.h"
// 统一使用的数据
//*
#define MAX_UI_MESSAGE_LENTH 256
static uint8_t buf[MAX_UI_MESSAGE_LENTH];
static uint8_t xor;
static UN_TRANS_U32_2_BYTE trans_32bits_data;
static UN_TRANS_U16_2_BYTE trans_16bits_data;

static uint8_t ui_command_buf[256];
static int ui_command_buf_ptr;
static uint8_t ui_convert_flag;
static uint8_t ui_command_xor_data;
static uint16_t           *st_ui_send_message_flag_ptr;
static struct
{
    uint8_t press_sensor_ack_status;
    uint8_t press_sensor_data_type;
    uint8_t oxygen_sensor_ack_status;
} st_ui_calibration_ack_msg;

enum
{
    EM_ACK_COMMAND_SUCCEED = 0,
    EM_ACK_COMMAND_FAILURE = 1
} __UI_COMMAND_ACK_DEFINES;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function defines

#define is_one_breath_cycle_completed()                  (st_ui_send_message_flag.breath_one_cycle_completed_flag == 1)
#define is_sensor_data_active_send()                     (st_ui_send_message_flag.sensor_data_active_flag == 1)
#define is_calibration_message_ready()                   (*st_ui_send_message_flag_ptr != 0)
#define is_calibration_press_sensor_data_set_complete()  (st_ui_send_message_flag.sensor_press_set_data_ack_flag == 1)
#define is_calibration_upload_sensor_press_data()        (st_ui_send_message_flag.sensor_press_data_upload_flag == 1)
#define is_calibration_upload_sensor_oxygen_data()       (st_ui_send_message_flag.sensor_flow_data_upload_flag == 1)
#define is_calibration_oxygen_sensor_data_set_complete() (st_ui_send_message_flag.oxygen_set_data_ack == 1)
#define set_upload_sensor_press_data_type(type)          (st_ui_calibration_ack_msg.press_sensor_data_type = type)
#define set_sensor_press_data_upload_flag()              (st_ui_send_message_flag.sensor_press_data_upload_flag = 1)
#define reset_sensor_press_data_upload_flag()            (st_ui_send_message_flag.sensor_press_data_upload_flag = 0)
#define set_oxygen_data_upload_flag()                    (st_ui_send_message_flag.oxygen_data_upload_flag = 1)
#define reset_oxygen_data_upload_flag()                  (st_ui_send_message_flag.oxygen_data_upload_flag = 0)
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


void clear_st_ui_calibration_ack_msg(void)
{
    st_ui_calibration_ack_msg.press_sensor_ack_status = EM_ACK_COMMAND_SUCCEED;
    st_ui_calibration_ack_msg.press_sensor_data_type = EM_SENSOR_PRESS_INHALE;
}

void set_oxygen_sensor_ack_status(void)
{
    st_ui_calibration_ack_msg.oxygen_sensor_ack_status = EM_ACK_COMMAND_FAILURE;
}

void reset_oxygen_sensor_ack_status(void)
{
    st_ui_calibration_ack_msg.oxygen_sensor_ack_status = EM_ACK_COMMAND_SUCCEED;
}

void set_sensor_oxygen_set_data_ack_flag(void)
{
    st_ui_send_message_flag.oxygen_set_data_ack = EM_ACK_COMMAND_FAILURE;
}

void reset_sensor_oxygen_set_data_ack_flag(void)
{
    st_ui_send_message_flag.oxygen_set_data_ack = EM_ACK_COMMAND_SUCCEED;
}

void set_press_sensor_ack_status(void)
{
    st_ui_calibration_ack_msg.press_sensor_ack_status = EM_ACK_COMMAND_FAILURE;
}

void reset_press_sensor_ack_status(void)
{
    st_ui_calibration_ack_msg.press_sensor_ack_status = EM_ACK_COMMAND_SUCCEED;
}


void set_sensor_press_set_data_ack_flag(void)
{
    st_ui_send_message_flag.sensor_press_set_data_ack_flag = EM_ACK_COMMAND_FAILURE;
}

void reset_sensor_press_set_data_ack_flag(void)
{
    st_ui_send_message_flag.sensor_press_set_data_ack_flag = EM_ACK_COMMAND_SUCCEED;
}

//void set_sensor_press_data_upload_flag(void)
//{
//	st_ui_send_message_flag.sensor_press_data_upload_flag = 1;
//}

//void reset_sensor_press_data_upload_flag(void)
//{
//	st_ui_send_message_flag.sensor_press_data_upload_flag = 0;
//}


void set_sensor_data_active_flag(void)
{
    st_ui_send_message_flag.sensor_data_active_flag = EM_ACK_COMMAND_FAILURE;
}

void reset_sensor_data_active_flag(void)
{
    st_ui_send_message_flag.sensor_data_active_flag = EM_ACK_COMMAND_SUCCEED;
}


void set_one_cycle_complete_flag(void)
{
    st_ui_send_message_flag.breath_one_cycle_completed_flag = EM_ACK_COMMAND_FAILURE;
}

void reset_one_cycle_complete_flag(void)
{
    st_ui_send_message_flag.breath_one_cycle_completed_flag = EM_ACK_COMMAND_SUCCEED;
}




//*/
void set_base_info_pid(int dat)
{
    if(dat) base_info_flags.oxygen_mixer_ctrol_pid = 1;
    else base_info_flags.oxygen_mixer_ctrol_pid = 0;
}

void reset_base_info_flags(void)
{
    base_info_flags.oxygen_mixer_ctrol_info = 0;
    base_info_flags.oxygen_mixer_ctrol_pid  = 0;
}

typedef enum _ENUM_BASE_INFORMATION_TYPES
{
    EM_BASE_INFO_MIXTER_CONTROL,
    EM_BASE_INFO_MIXTER_PID
} ENUM_BASE_INFORMATION_TYPES;

void set_base_info_flags(uint16_t flag)
{
}

void trans_handshake_data(void)
{
    buf[0] = BASE_HANDSHAKE_MSG; // handshake massage
    buf[1] = 0x0;// xor data byte

    packed_commnication_data((uint8_t *)buf, 2);
}

void trans_ack_data(uint8_t com_data, uint8_t result)
{
    buf[0] = BASE_ACK_MSG; // handshake massage
    xor    = BASE_ACK_MSG;
    buf[1] = com_data;
    xor   ^= com_data;
    buf[2] = result;
    xor   ^= result;
    buf[3] = xor;

    packed_commnication_data((uint8_t *)buf, 4);
}

void trans_base_board_status(void)
{
    get_new_base_board_status();
    buf[0] = BASE_BOARD_STATUS; // handshake massage
    xor    = BASE_BOARD_STATUS;
    buf[1] = base_board_status.bytes[0];
    xor   ^= base_board_status.bytes[0];
    buf[2] = base_board_status.bytes[1];
    xor   ^= base_board_status.bytes[1];
    buf[3] = base_board_status.bytes[2];
    xor   ^= base_board_status.bytes[2];
    buf[4] = base_board_status.bytes[3];
    xor   ^= base_board_status.bytes[3];
    buf[5] = xor;
    packed_commnication_data((uint8_t *)buf, 6);
}

void reset_ui_data(void)
{
    //ui_trans_actived_flag = EM_ACTIVED;
    reset_ui_command_buf();

    // set
    st_ui_send_message_flag_ptr  = (uint16_t *)&st_ui_send_message_flag;
    *st_ui_send_message_flag_ptr = 0;
}

#if 0
/* ***********************************************
 * Flow Sensor Value   :  2     (mV)
 * Flow value:            3    (mL/Min)
 * O2 Sensor Value:       2    (mV)
 * O2 value:              1    (0.1%)
 * Press Sensor Value:    2    (mV)
 * Press Value:           2    (0.1cmH2O)
 * Blower Rate:           2   (rpm)
 * ***********************************************
   adc_voltage
   adc_convert_data
   blower_status
 */
void trans_Oxygen_Mixer_data(void)
{
    uint8_t data;
    uint8_t i;

    count_blower_speed();
    i = 0;
    buf[i++] = BASE_OXYGEN_MIXER_INFO_MSG;
    xor      = BASE_OXYGEN_MIXER_INFO_MSG;
    // * Flow Sensor Value   :  2     (mV)
    data = (uint8_t)(adc_voltage.adc_flow_exhale_low & 0xff); // 低字节
    buf[i++] = data;
    xor     ^= data;
    data     = (uint8_t)(adc_voltage.adc_flow_exhale_low >> 8) & 0xff;// 高字节
    buf[i++] = data;
    xor     ^= data;

    // * Flow value:            3    (mL/Min)
    trans_32bits_data.data = adc_convert_data.flow_2_val;
    data     = trans_32bits_data.byte[0];//(uint8_t)(adc_convert_data.flow_2_val & 0xff); // 低字节
    buf[i++] = data;
    xor     ^= data;
    data     = trans_32bits_data.byte[1];//(uint8_t)(adc_convert_data.flow_2_val>>8) & 0xff;
    buf[i++] = data;
    xor     ^= data;
    data     = trans_32bits_data.byte[2];//(uint8_t)(adc_convert_data.flow_2_val>>16) & 0xff; // 高字节
    buf[i++] = data;
    xor     ^= data;

    // * O2 Sensor Value:       2    (mV)
    trans_16bits_data.data = adc_voltage.adc_oxygen;
    // 低字节
    buf[i++] = trans_16bits_data.byte[0];
    xor     ^= trans_16bits_data.byte[0];
    // 高字节
    buf[i++] = trans_16bits_data.byte[1];
    xor     ^= trans_16bits_data.byte[1];
    // * O2 value:              1    (0.1%)
    trans_16bits_data.data = adc_convert_data.O2_concent_2_val;
    // 低字节
    buf[i++] = trans_16bits_data.byte[0];
    xor     ^= trans_16bits_data.byte[0];
    // 高字节
    buf[i++] = trans_16bits_data.byte[1];
    xor     ^= trans_16bits_data.byte[1];

    // * Press Sensor Value:    2    (mV)
    trans_16bits_data.data = adc_voltage.adc_press_exhale;
    // 低字节
    buf[i++] = trans_16bits_data.byte[0];
    xor     ^= trans_16bits_data.byte[0];
    // 高字节
    buf[i++] = trans_16bits_data.byte[1];
    xor     ^= trans_16bits_data.byte[1];

    // * Press Value:           2    (0.1cmH2O)
    trans_16bits_data.data = adc_convert_data.pressure_4_val;
    // 低字节
    buf[i++] = trans_16bits_data.byte[0];
    xor     ^= trans_16bits_data.byte[0];
    // 高字节
    buf[i++] = trans_16bits_data.byte[1];
    xor     ^= trans_16bits_data.byte[1];

    // * Blower Rate:           2   (rpm)
    trans_16bits_data.data = blower_status.speed;
    // 低字节
    buf[i++] = trans_16bits_data.byte[0];
    xor     ^= trans_16bits_data.byte[0];
    // 高字节
    buf[i++] = trans_16bits_data.byte[1];
    xor     ^= trans_16bits_data.byte[1];

    // Oxygen Flow
    trans_32bits_data.data = (uint32_t)get_oxygen_flow();
    // 低字节
    buf[i++] = trans_32bits_data.byte[0];
    xor     ^= trans_32bits_data.byte[0];
    // 中间字节
    buf[i++] = trans_32bits_data.byte[1];
    xor     ^= trans_32bits_data.byte[1];
    // 高字节
    buf[i++] = trans_32bits_data.byte[2];
    xor     ^= trans_32bits_data.byte[2];

    buf[i++] = xor;
    packed_commnication_data((uint8_t *)buf, i);

}
#endif

void trans_upload_sensor_oxygen_data(void)
{
    uint8_t i;

    i   = 0;
    xor = 0;
    buf[i++] = BASE_PRESS_SENSOR_DATA_MSG;

    trans_eeprom_oxygen_data_from_real();

    //字节1-2：20.9% 的氧浓度传感器值
    trans_16bits_data.data = (uint16_t)eeprom_oxygen_data.data.base;
    buf[i++] = trans_16bits_data.byte[0]; // 低字节
    buf[i++] = trans_16bits_data.byte[1]; // 高字节

    //字节3-4：100%的氧浓度传感器的值
    trans_32bits_data.data = (uint16_t)eeprom_oxygen_data.data.P_step;
    buf[i++] = trans_32bits_data.byte[0]; // 低字节
    buf[i++] = trans_32bits_data.byte[1]; // 高字节
    buf[i++] = trans_32bits_data.byte[2]; // 高字节

    for(i = 0; i < 6; i++)
    {
        xor ^= buf[i];
    }
    buf[i++] = xor;
    packed_commnication_data((uint8_t *)buf, i);
}

void trans_calibration_press_sensor_upload_data(void)
{
    uint8_t i;

    if(st_ui_calibration_ack_msg.press_sensor_data_type > EM_SENSOR_PRESS_OTHERS)
    {
        set_upload_sensor_press_data_type(0);
        return;
    }
    ui_get_work_press_data(st_ui_calibration_ack_msg.press_sensor_data_type);
    i   = 0;
    xor = 0;

    buf[i++] = BASE_PRESS_SENSOR_DATA_MSG;

    // 字节1：    传感器位置、
    buf[i++] = st_ui_calibration_ack_msg.press_sensor_data_type;
    //字节2-3：0点位置
    trans_16bits_data.data = (uint16_t)eeprom_press_data.data.base;
    buf[i++] = trans_16bits_data.byte[0]; // 低字节
    buf[i++] = trans_16bits_data.byte[1]; // 高字节

    //字节4-6：正压时每一bit变化时的增量（1/1‘000’000）
    trans_32bits_data.data = eeprom_press_data.data.P_step;
    buf[i++] = trans_32bits_data.byte[0];// 低字节
    buf[i++] = trans_32bits_data.byte[1];
    buf[i++] = trans_32bits_data.byte[2];// 高字节

    //字节7-9：负压时每一bit变化时的增量（1/1‘000’000）
    trans_32bits_data.data = eeprom_press_data.data.N_step;
    buf[i++] = trans_32bits_data.byte[0];// 低字节
    buf[i++] = trans_32bits_data.byte[1];
    buf[i++] = trans_32bits_data.byte[2];// 高字节


    for(i = 0; i < 10; i++)
    {
        xor ^= buf[i];
    }
    buf[i++] = xor;
    packed_commnication_data((uint8_t *)buf, i);
}
/* ***************************************************
   Blower Control: 2byte
   Oxygen Valve Control: 2 byte
 * ***************************************************
 */
void trans_Oxygen_Mixer_ctrol(void)
{
    uint8_t i;
    i   = 0;
    xor = 0;

    buf[i++] = BASE_OXYGEN_MIXER_CONTROL;
    //Blower Control: 2byte
    buf[i++] = (uint8_t) (blower_main.control_data & 0xff);
    buf[i++] = (uint8_t) ((blower_main.control_data >> 8) & 0xff);

    // Oxygen Valve Control: 2 byte
    buf[i++] = (uint8_t) (oxygen_mixer_ctrol.control_data & 0xff);
    buf[i++] = (uint8_t) ((oxygen_mixer_ctrol.control_data >> 8) & 0xff);

    for(i = 0; i < 5; i++)
    {
        xor ^= buf[i];
    }
    buf[i++] = xor;
    packed_commnication_data((uint8_t *)buf, i);
}

void trans_Oxygen_data(void)
{
    buf[0] = (uint8_t)BASE_BREATH_OXYGEN_MSG;
    xor    = buf[0];
    // * O2 value:              1    (0.1%)
    trans_16bits_data.data = adc_convert_data.O2_concent_2_val;
    // 低字节
    buf[1] = trans_16bits_data.byte[0];
    xor   ^= trans_16bits_data.byte[0];
    // 高字节
    buf[2] = trans_16bits_data.byte[1];
    xor   ^= trans_16bits_data.byte[1];
    buf[3] = xor;

    packed_commnication_data((uint8_t *)buf, 4);
}

void trans_calibration_press_sensor_set_ack_data(void)
{
    uint8_t i;

    i = 0;

    buf[i++] = BASE_SET_PRESS_SENSOR_ACK_MSG;
    xor      = BASE_SET_PRESS_SENSOR_ACK_MSG;
    buf[i++] = st_ui_calibration_ack_msg.press_sensor_ack_status;
    xor     ^= st_ui_calibration_ack_msg.press_sensor_ack_status;

    buf[i++] = xor;
    packed_commnication_data((uint8_t *)buf, i);
}

void trans_calibration_oxygen_sensor_set_ack_data(void)
{
    uint8_t i;

    i = 0;

    buf[i++] = BASE_SET_OXYGEN_ACK_MSG;
    xor      = BASE_SET_OXYGEN_ACK_MSG;
    buf[i++] = st_ui_calibration_ack_msg.oxygen_sensor_ack_status;
    xor     ^= st_ui_calibration_ack_msg.oxygen_sensor_ack_status;

    buf[i++] = xor;
    packed_commnication_data((uint8_t *)buf, i);
}


void trans_valve_status(void)
{
#if UART_TEST_VALVE
    uint8_t i;

    i = 0;
    get_valve_status();
    buf[i++] = BASE_VALVE_INFO_MSG;
    xor      = BASE_VALVE_INFO_MSG;
    buf[i++] = st_Valve_status.base;
    xor     ^= st_Valve_status.base;
    buf[i++] = st_Valve_status.GroupA;
    xor     ^= st_Valve_status.GroupA;
    buf[i++] = st_Valve_status.GroupB;
    xor     ^= st_Valve_status.GroupB;
    buf[i++] = xor;
    packed_commnication_data((uint8_t *)buf, i);
#endif
}

int32_t get_display_flow(void);
int32_t get_display_press(void);

/*
 * PFV
 * byte 1:	0x10  // head
 * byte 2-3：压力，单位0.1cmH2O,
 *              范围 -100到+100
 *              有效值范围 0xFC18-0x03E8"
 * byte 4-6：流量  单位 1mL/Min，
 *                 范围-4,00,000mL/Min 到4,000,000mL/Min
 *                 有效值范围  0xF9E580-0x061A80"
 * byte 1-8: 潮气量 单位 1mL
 *        范围 0-2000 mL
 *				有效值范围 0x0000 - 0x07D0"
 * byte 9： 校验，数值的xor
 */
void trans_ui_wave_data(void)
{
    uint8_t i;

    i = 0;
    //head // command
    if(display_count_data.trigger_flag)
    {
        buf[i++] = BASE_BREATH_TRIGGER_I_MSG;
    }
    else
    {
        buf[i++] = BASE_PFV_MSG;
    }
    // press
    trans_16bits_data.data = (uint16_t)get_display_press();//display_count_data.Press;
    buf[i++] = trans_16bits_data.byte[0]; // 低字节
    buf[i++] = trans_16bits_data.byte[1]; // 高字节

    // flow
    trans_32bits_data.data = get_display_flow();//display_count_data.Flow;
    buf[i++] = trans_32bits_data.byte[0];// 低字节
    buf[i++] = trans_32bits_data.byte[1];
    buf[i++] = trans_32bits_data.byte[2];// 高字节

    // volume
    trans_16bits_data.data = (uint16_t)display_count_data.volume;
    buf[i++] = trans_16bits_data.byte[0]; // 低字节
    buf[i++] = trans_16bits_data.byte[1]; // 高字节

    xor = 0;

    for(i = 0; i < 8; i++)
    {
        xor ^= buf[i];
    }
    buf[8] = xor;
    packed_commnication_data((uint8_t *)buf, 9);
}


void trans_breath_second_vt_data(void)
{
    uint8_t i;

    i = 0;
    //head // command
    if(cough_control_data.cough_display_data_ready == EM_ACTIVED)
    {
        buf[i++] = BASE_COUGH_SECOND_VT_MSG;
        cough_control_data.cough_display_data_ready = EM_INACTIVED;
    }
    else
    {
        buf[i++] = BASE_BREATH_SECOND_VT_MSG;
    }
    trans_16bits_data.data = (uint16_t)display_count_data.vt_sec[0];
    buf[i++] = trans_16bits_data.byte[0];
    buf[i++] = trans_16bits_data.byte[1];

    trans_16bits_data.data = (uint16_t)display_count_data.vt_sec[1];
    buf[i++] = trans_16bits_data.byte[0];
    buf[i++] = trans_16bits_data.byte[1];

    trans_16bits_data.data = (uint16_t)display_count_data.vt_sec[2];
    buf[i++] = trans_16bits_data.byte[0];
    buf[i++] = trans_16bits_data.byte[1];

    trans_16bits_data.data = (uint16_t)display_count_data.vt_sec[3];
    buf[i++] = trans_16bits_data.byte[0];
    buf[i++] = trans_16bits_data.byte[1];

    xor = 0;
    for(i = 0; i < 9; i++)
    {
        xor ^= buf[i];
    }
    buf[9] = xor;
    packed_commnication_data((uint8_t *)buf, 10);
}


/* ********************************************
 * 字节1-2：	  吸气压力传感器的值	         （0-4095）
 * 字节3-4：	 呼气压力传感器的值	 （0-4095）
 * 字节5-6：	 真空泵压力传感器的值（0-4095）
 * 字节7-8：	 呼气流量传感器的值	 （0-4095）
 * 字节9-10：	高呼气流量传感器值 （0-4095）
 * 字节11-12：混氧流量传感器的值	         （0-4095）
 * 字节13-14：氧浓度传感器的值            （0-4095）
 * 字节15-16：TSI传感器的模拟量值   （0-4095）
 * *******************************************
 */
void trans_sensor_data(void)
{
    uint8_t i;

    i = 0;
    //head // command
    buf[i++] = UI_REAL_SENSOR_DATA_COM;
    //* 字节1-2：	  吸气压力传感器的值	         （0-4095）
    trans_16bits_data.data = (uint16_t)adc_voltage.adc_press_inhale;
    buf[i++] = trans_16bits_data.byte[0];
    buf[i++] = trans_16bits_data.byte[1];

    //* 字节3-4：	 呼气压力传感器的值	 （0-4095）
    trans_16bits_data.data = (uint16_t)adc_voltage.adc_press_exhale;
    buf[i++] = trans_16bits_data.byte[0];
    buf[i++] = trans_16bits_data.byte[1];
    //* 字节5-6：	 真空泵压力传感器的值（0-4095）
    trans_16bits_data.data = (uint16_t)adc_voltage.adc_press_vacuum_val;
    buf[i++] = trans_16bits_data.byte[0];
    buf[i++] = trans_16bits_data.byte[1];
    //* 字节7-8：	 呼气流量传感器的值	 （0-4095）
    trans_16bits_data.data = (uint16_t)adc_voltage.adc_flow_exhale_low;
    buf[i++] = trans_16bits_data.byte[0];
    buf[i++] = trans_16bits_data.byte[1];
    //* 字节9-10：	高呼气流量传感器值 （0-4095）
    trans_16bits_data.data = (uint16_t)adc_voltage.adc_flow_exhale_high;
    buf[i++] = trans_16bits_data.byte[0];
    buf[i++] = trans_16bits_data.byte[1];
    //* 字节11-12：混氧流量传感器的值	         （0-4095）
    trans_16bits_data.data = (uint16_t)adc_voltage.adc_flow_oxygen;
    buf[i++] = trans_16bits_data.byte[0];
    buf[i++] = trans_16bits_data.byte[1];
    //* 字节13-14：氧浓度传感器的值           （0-4095）
    trans_16bits_data.data = (uint16_t)adc_voltage.adc_oxygen;
    buf[i++] = trans_16bits_data.byte[0];
    buf[i++] = trans_16bits_data.byte[1];
    //* 字节15-16：TSI传感器的模拟量值    （0-4095）
    trans_16bits_data.data = (uint16_t)adc_voltage.adc_tsi_V;
    buf[i++] = trans_16bits_data.byte[0];
    buf[i++] = trans_16bits_data.byte[1];


    xor = 0;
    for(i = 0; i < 17; i++)
    {
        xor ^= buf[i];
    }
    buf[17] = xor;

    packed_commnication_data((uint8_t *)buf, 18);
}

/* ******************************************************
* 字节1-2：Ppeak 0-1000 （0.1cmH2O）
* 字节3-4：Pmean 0-1000（0.1cmH2O）
* 字节5-6：Vt  0-3000 (mL)
* 字节7-8：MV 0-1000  （0.1L/min）
* 字节9-10：FREQ 0-2000  ( 0.1 BPM)
* 字节11-13：Peak_Flow_i  0-200000 (mL/min)
* 字节14-16：Peak_Flow_e 0-200000(mL/min)
* 字节17-18:Ttotal: 0-60000 (ms)
* 字节19-20:Ti 0-30000 (ms)
* 字节21:Breath_Cycles (1-100)
* 字节22:Trigger_Cycles(0-100)
* 字节23-25:Leak Flow (0-200000)(mL/min)
* 字节26-27: R(0-2000)   {(0.1 x) cmH2O/(L/s)}
* 字节28-29: C(0-1000) {(0.1 x) mL/cmH2O }
* ******************************************************/
void trans_breath_one_cycle_data(void)
{
    uint8_t i;

    i = 0;
    //head // command
    buf[i++] = BASE_BREATH_DATA_MSG;
    trans_16bits_data.data = (uint16_t)display_count_data.P_peak;
    buf[i++] = trans_16bits_data.byte[0];
    buf[i++] = trans_16bits_data.byte[1];

    trans_16bits_data.data = (uint16_t)display_count_data.P_mean;
    buf[i++] = trans_16bits_data.byte[0];
    buf[i++] = trans_16bits_data.byte[1];

    trans_16bits_data.data = (uint16_t)display_count_data.vt;
    buf[i++] = trans_16bits_data.byte[0];
    buf[i++] = trans_16bits_data.byte[1];

    trans_16bits_data.data = (uint16_t)(display_count_data.MV / 100); // 转换成 0.1L/Min
    buf[i++] = trans_16bits_data.byte[0];
    buf[i++] = trans_16bits_data.byte[1];

    trans_16bits_data.data = (uint16_t)display_count_data.freq;
    buf[i++] = trans_16bits_data.byte[0];
    buf[i++] = trans_16bits_data.byte[1];


    trans_32bits_data.data = (uint32_t)display_count_data.inspire_peak_flow;
    buf[i++] = trans_32bits_data.byte[0];
    buf[i++] = trans_32bits_data.byte[1];
    buf[i++] = trans_32bits_data.byte[2];

    trans_32bits_data.data = (uint32_t)display_count_data.expire_peak_flow;
    buf[i++] = trans_32bits_data.byte[0];
    buf[i++] = trans_32bits_data.byte[1];
    buf[i++] = trans_32bits_data.byte[2];

    if(display_count_data.Ttotal > 60000)
    {
        trans_16bits_data.data = 60000;
    }
    else
    {
        trans_16bits_data.data = (uint16_t)display_count_data.Ttotal;
    }
    buf[i++] = trans_16bits_data.byte[0];
    buf[i++] = trans_16bits_data.byte[1];

    if(display_count_data.Ti > 30000)
    {
        trans_16bits_data.data = 30000;
    }
    else
    {
        trans_16bits_data.data = (uint16_t)display_count_data.Ti;
    }
    buf[i++] = trans_16bits_data.byte[0];
    buf[i++] = trans_16bits_data.byte[1];

    buf[i++] = display_count_data.total_breath_cycles;
    buf[i++] = display_count_data.total_tirgger_cycles;

    trans_32bits_data.data = (uint32_t)display_count_data.flow_leak;
    buf[i++] = trans_32bits_data.byte[0];
    buf[i++] = trans_32bits_data.byte[1];
    buf[i++] = trans_32bits_data.byte[2];

    // R
    buf[i++] = 0;
    buf[i++] = 0;

    //C
    buf[i++] = 0;
    buf[i++] = 0;

    xor = 0;
    for(i = 0; i < 30; i++)
    {
        xor ^= buf[i];
    }
    buf[30] = xor;
    packed_commnication_data((uint8_t *)buf, 31);
}


/* *****************************************************
 *	字节1-2:     Pi 吸气口压力(-100.0 - +100.0) cmH2O
 *	字节3-4-5: Fi 吸气流速(0-300,000)ml/Min
 *	字节6-7:     Vi 吸气口气体容积积分值（一周期）
 *	字节8-9:     Pe 呼气口压力(-100.0 - +100.0) cmH2O
 *	字节10-12: Fe 呼气流速(0-300,000)ml/Min
 *	字节13-14: Ve 呼气口气体容积积分值（一周期
 * *****************************************************
 */
void trans_PPFFVV_data(void)
{
#if USER_TEST_PPFFVV
    uint8_t i;

    xor      = 0;
    i        = 0;
    //head // command
    buf[i++] = BASE_PPFFVV_MSG;
    // press
    buf[i++] = (uint8_t)(adc_convert_data.pressure_4_val & 0xff); // 低字节
    buf[i++] = (uint8_t)(adc_convert_data.pressure_4_val >> 8) & 0xff; // 高字节

    // flow
    trans_32bits_data.data = get_inspire_flow();
    buf[i++] = trans_32bits_data.byte[0];// 低字节
    buf[i++] = trans_32bits_data.byte[1];
    buf[i++] = trans_32bits_data.byte[2];// 高字节
    // volume
    trans_16bits_data.data = (uint16_t) (display_temp_data.v_i / 60000);
    buf[i++] = trans_16bits_data.byte[0]; // 低字节
    buf[i++] = trans_16bits_data.byte[1]; // 高字节

    // press
    trans_16bits_data.data = (uint16_t)adc_convert_data.pressure_1_val;
    buf[i++] = trans_16bits_data.byte[0]; // 低字节
    buf[i++] = trans_16bits_data.byte[1]; // 高字节

    // flow
    trans_32bits_data.data = get_expire_flow();
    buf[i++] = trans_32bits_data.byte[0];// 低字节
    buf[i++] = trans_32bits_data.byte[1];
    buf[i++] = trans_32bits_data.byte[2];// 高字节
    // volume
    trans_16bits_data.data = (uint16_t) (display_temp_data.v_e / 60000);
    buf[i++] = trans_16bits_data.byte[0]; // 低字节
    buf[i++] = trans_16bits_data.byte[1]; // 高字节


    for(i = 0; i < 15; i++)
    {
        xor ^= buf[i];
    }
    buf[15] = xor;
    packed_commnication_data((uint8_t *)buf, 16);
#endif
}

void packed_pid_info_msg(void)
{
    int16_t tmp;
    uint8_t i;

    if(base_info_flags.oxygen_mixer_ctrol_pid == 0) return;
    set_base_info_pid(0);
    i = 0;
    xor = 0;
    //head // command
    buf[i++] = BASE_PID_ADJUST_INFO_MSG;
    // type
    buf[i++] = 0; // 混氧PID调整数据
    tmp      = (int16_t)(oxygen_mixer_ctrol.adjust_mul * 1000.0);
    trans_16bits_data.data = (uint16_t)tmp;
    buf[i++] = trans_16bits_data.byte[0]; // 低字节
    buf[i++] = trans_16bits_data.byte[1]; // 高字节
    tmp      = (int16_t)oxygen_mixer_ctrol.adjust_add;
    trans_16bits_data.data = (uint16_t)tmp;
    buf[i++] = trans_16bits_data.byte[0]; // 低字节
    buf[i++] = trans_16bits_data.byte[1]; // 高字节

    for(i = 0; i < 6; i++)
    {
        xor ^= buf[i];
    }
    buf[6] = xor;

    packed_commnication_data((uint8_t *)buf, 7);
}

void packed_aic_information(void)
{
#if 0
    uint8_t i;

    i        = 0;
    buf[i++] = BASE_AIC_INFORMATION_MSG;
    xor      = BASE_AIC_INFORMATION_MSG;
    buf[i++] = breath_AI_control_data.current_breath_count;
    xor     ^= breath_AI_control_data.current_breath_count;
    buf[i++] = breath_AI_control_data.current_cycles_count;
    xor     ^= breath_AI_control_data.current_cycles_count;
    buf[i++] = xor;
    packed_commnication_data((uint8_t *)buf, i);
#endif
}


//张志新：这里只是简单实现了功能，最终这个函数是需要优化的
void packed_commnication_data(uint8_t *ptr, int len)
{
    int i;

    uart3_put_char(UI_PROTOCOL_HEAD);
    for(i = 0; i < len; i++)
    {
        switch(*ptr)
        {
        case UI_PROTOCOL_HEAD:
            uart3_put_char(UI_PROTOCOL_TRANSFER_CODE);
            uart3_put_char(0x0);
            break;
        case UI_PROTOCOL_END:
            uart3_put_char(UI_PROTOCOL_TRANSFER_CODE);
            uart3_put_char(0x1);
            break;
        case UI_PROTOCOL_TRANSFER_CODE:
            uart3_put_char(UI_PROTOCOL_TRANSFER_CODE);
            uart3_put_char(0x2);
            break;

        default:
            uart3_put_char(*ptr);
            break;
        }
        ptr++;
    }
    uart3_put_char(UI_PROTOCOL_END);
}

void reset_ui_command_buf(void)
{
    int i;
    for(i = 0; i < 64; i++)
    {
        ui_command_buf[i] = 0;
    }
    ui_command_buf_ptr    = 0;
    ui_convert_flag       = 0;
    ui_command_xor_data   = 0;
}

void decode_ui_command(void)
{
    switch(ui_command_buf[0])
    {
    case UI_HANDSHAKE_COM:         // = 0x0
        break;
    case UI_STANDBY_MODE_COM:      // = 0x20
        set_changemode_actived_mode(EM_VENTLATOR_STANDBY_MODE);
        break;

    case UI_STANDBY_SET_INVASIVE_COM://  = 0x21,	//	插管状态		0x21	字节1：	 invasive_support;			插管状态：      =0 无创通气			   =1：有创通气
        break;

    case UI_STANDBY_SET_AIR_TYPE_COM://  = 0x22,    //  气体标准		0x22	字节1：	 air_type;					气体标准：      =0：ATP           =1：BTPS
        break;

    case UI_STANDBY_SET_PATIENT_COM://   = 0x23,	//	患者类型		0x23	字节1：	 patient_types;       患者类型：      =0：Adult				 =1：Children/=2：Infant
        break;

    case UI_STANDBY_SET_PIPE_TYPE_COM:// = 0x24,	//  管路连接形式		0x24	字节1：	 pipe_connect_type;     管路连接形式： =0：双管				  =1：单管
        break;

    case UI_STANDBY_SET_PIPE_C_COM://    = 0x25,	//  路顺应性      0x25	字节1：	 breathing_circuit_C;	管路顺应性：	  0.1cmH2O
        break;

    case UI_STANDBY_SET_DIESAESE_COM://  = 0x26,    //	症状		    0x26	字节1：	 body_disease;				症状					 =0: (正常)Normal    =1:(特殊)ARDS	 /=2:(特殊)COPD/etc.
        break;

    case UI_REAL_SENSOR_DATA_COM://      = 0x27,    //  读取传感器数据	              0x27	无 "此指令用于启动传感器上传，如果主控板接收这个指令。会持续上传传感器数据,直到UI发出任意模式控制指令（比如STANDBY指令0x20）才会停止"
        set_sensor_data_active_flag();
        break;

    case UI_SET_PRESS_SENSOR_DATA_COM:// = 0x28,    //	设置压力传感器 校准值           0x28
        ui_set_press_sensor_data(ui_command_buf);
        break;

    case UI_GET_PRESS_SENSOR_DATA_COM:// = 0x29,    //	读取压力传感器校准数据		        0x29
        // set upload data type
        set_upload_sensor_press_data_type(ui_command_buf[1]);
        // set flag
        set_sensor_press_data_upload_flag();
        break;

    case UI_SET_FLOW_SENSOR_DATA_COM://  = 0x2A,	//	设置流量传感器校准值		        0x2A
        break;

    case UI_GET_FLOW_SENSOR_DATA_COM://  = 0x2B,	//	读取流量传感器校准值		        0x2B
        break;

    case UI_SET_LIST_UNIT_DATA_COM://    = 0x2C,	//	设置可控制单元的校准值		        0x2C
        break;

    case UI_GET_LIST_UNIT_DATA_COM://    = 0x2D,	//	读取可控制单元的校准值		        0x2D
        break;

    case UI_SET_OXYGEN_SENSOR_DATA_COM://= 0x2E,	//	设置氧传感器校准值               0x2E
        ui_set_oxygen_sensor_data(ui_command_buf);

        break;

    case UI_GET_OXYGEN_SENSOR_DATA_COM://= 0x2F,	//	读取氧传感器校准值               0x2F
        set_oxygen_data_upload_flag();
        break;

    case UI_CPAP_MODE_COM:         // = 0x31
        ui_set_data.epap         = (uint16_t)ui_command_buf[1] * 10;
        ui_set_data.ipap         = (uint16_t)ui_set_data.epap;
#if CFLEX_ENABLE_FOR_CPAP
        ui_set_data.c_flex_level = (uint16_t)ui_command_buf[2] * 10;
#endif
        set_changemode_actived_mode(EM_VENTLATOR_CPAP_MODE);
        break;

    case UI_COUGH_START_COM:       // = 0x32
        /* ******************************************************
           字节1：Pcough_i     吸气的目标压力（15-60）cmH2O
           字节2：Pcough_e     咳痰初始压力（0--->  -90）cmH2O
           字节3：Vtcough      初始潮气量 （30-150）x 10ml
           字节4：CoughTi      咳痰吸气时间（0.5-3.0）秒
           字节5：Hold_Time 屏气时间（0.1-2.0S）
           字节6：stop_mode 咳痰结束检测条件 （0-3）
           字节7：stop_value检测阈值

           检测条件及阈值定义：
           stop_mode	             stop_value   范围     单位(uint)
           潮气量阈值(绝对值)          = 1          0-150      ml
           潮气量百分比(50-100)        = 2          50-100     %
           呼气流量阈值              = 3          0-100      L/min
           时间阈值	                 = 4          1-20       0.1秒
        * ******************************************************/
        ui_set_data.cough_prepare_press       = (uint16_t)ui_command_buf[1] *  10;
        ui_set_data.cough_start_press         = (uint16_t)ui_command_buf[2] *  10;
        ui_set_data.cough_start_vt            = (uint16_t)ui_command_buf[3] *  10;
        ui_set_data.cough_press_increase_time = (uint16_t)ui_command_buf[4] * 100;
        ui_set_data.cough_hold_time           = (uint16_t)ui_command_buf[5] * 100;
        ui_set_data.cough_finish_types        = (uint8_t) ui_command_buf[6];
        ui_set_data.cough_finish_val          = (uint8_t) ui_command_buf[7];
        // 设置咳痰指令
        force_start_vacuum_pump();
        set_changemode_actived_mode(EM_VENTLATOR_COUGH_MODE);
        break;

    case UI_COUGH_REPEAT_COM:      // = 0x33
        set_changemode_actived_mode(EM_VENTLATOR_COUGH_REPEAT_MODE);
        break;

    case UI_COUGH_STOP_COM:        // = 0x34
        stop_cough_blower_work();

        cough_back_to_prev_work();
        break;

    case UI_ST_MODE_COM:           // = 0x42
        /* ****************************************************
           字节1：EPAP（0，4-20）
           字节2：IPAP（4-20）
           字节3：FREQ     频率（控制模式）
           字节4：Ti       吸气时间（控制模式）1-10s
           字节5：Tslope压力上升时间（0.1-2.0S）（控制模式）
           字节6：C-flex压力释放强度C-Flex（0-4）
        * ****************************************************/
        ui_set_data.epap           = (uint16_t)ui_command_buf[1] *  10;
        ui_set_data.ipap           = (uint16_t)ui_command_buf[2] *  10;
        ui_set_data.T_freq         = (uint16_t)ui_command_buf[3];
        ui_set_data.T_inspire_time = (uint32_t)ui_command_buf[4] * 100;
        ui_set_data.T_rising_time  = (uint32_t)ui_command_buf[5] * 100;
        ui_set_data.c_flex_level   = (uint32_t)ui_command_buf[6] *  10;

        set_changemode_actived_mode(EM_VENTLATOR_BIPAP_ST_MODE);
        break;

    case UI_PCV_MODE_COM:    // = 0x43
        /* ****************************************************
           字节1：EPAP（0，4-20）
           字节2：IPAP（4-20）
           字节3：FREQ频率（控制模式）
           字节4：Ti吸气时间（控制模式）1-10s
           字节5：压力上升时间（0.1-2.0S）（控制模式）
        * ****************************************************/
        ui_set_data.epap       = (uint16_t)ui_command_buf[1] *  10;
        ui_set_data.ipap       = (uint16_t)ui_command_buf[2] *  10;
        ui_set_data.T_freq       = (uint16_t)ui_command_buf[3];
        ui_set_data.T_inspire_time = (uint32_t)ui_command_buf[4] * 100;
        ui_set_data.T_rising_time  = (uint32_t)ui_command_buf[5] * 100;
        set_changemode_actived_mode(EM_VENTLATOR_BIPAP_PCV_MODE);
        break;
    /*
       PEEP(EPAP)    0x60	 字节1：EPAP（0，4-20）
       FREQ		     0x61	 字节1：FREQ频率（控制模式）
       Ptarget(IPAP)	 0x62	 字节1：IPAP（4-20）
     */
    case UI_SET_EPAP_COM:          // = 0x60,
        ui_set_data.epap = (uint16_t)ui_command_buf[1] *  10;
        set_ui_change_parameters();
        break;
    case UI_SET_FREQ_COM:      // = 0x61,
        ui_set_data.ipap = (uint16_t)ui_command_buf[1] *  10;
        set_ui_change_parameters();
        break;
    case UI_SET_IPAP_COM:        // = 0x62,
        ui_set_data.T_freq = (uint16_t)ui_command_buf[1];
        set_ui_change_parameters();
        break;
    case UI_START_RAP_COM:         // = 0x78
        breath_rap_control_data.freq              = (uint8_t )ui_command_buf[1];
        breath_rap_control_data.press             = (uint16_t)ui_command_buf[2] *  10;
        breath_rap_control_data.inspire_work_time = (uint32_t)ui_command_buf[3] * 100;
        breath_rap_control_data.expire_work_time  = (uint32_t)ui_command_buf[4] * 100;
        refresh_rap_control_data(breath_rap_control_data.press, breath_rap_control_data.freq, 0);
        set_breath_rap_enable();
        break;
    case UI_STOP_RAP_COM:          // = 0x79
        set_breath_rap_disable();
        break;
    case UI_START_NEBULIZER_COM:   // = 0x80,
        set_nebulizer_work_status(EM_ON);
        break;
    case UI_STOP_NEBULIZER_COM:    // = 0x81,
        set_nebulizer_work_status(EM_OFF);
        break;
    case UI_STOP_COMMUNICATION_COM: // = 0xD0,
        //ui_trans_actived_flag = EM_INACTIVED;
        break;
    case UI_START_COMMUNICATION_COM: // = 0xD1,
        //ui_trans_actived_flag = EM_ACTIVED;
        break;
    case UI_STOP_O2_MIXER_COM:   // = 0xD2,
        oxygen_stop();
        break;
    case UI_START_O2_MIXTER_COM: // = 0xD3,
        oxygen_start();
        trans_16bits_data.byte[0] = ui_command_buf[1];
        trans_16bits_data.byte[1] = ui_command_buf[2];
        set_blower_speed(trans_16bits_data.data);
        set_concentration_value(ui_command_buf[3]);
        break;
    case UI_CHANGE_BLOWER_COM:   // = 0xD4,
        trans_16bits_data.byte[0] = ui_command_buf[1];
        trans_16bits_data.byte[1] = ui_command_buf[2];
        set_blower_speed(trans_16bits_data.data);
        break;
    case UI_CHANGE_OXYGEN_COM:   // = 0xD5,
        set_concentration_value(ui_command_buf[1]);
        break;
    case UI_CHANGE_OVERFLOW_VALVE_COM: // = 0xD6
        trans_16bits_data.byte[0] = ui_command_buf[1];
        trans_16bits_data.byte[1] = ui_command_buf[2];
        set_overflow_valve(trans_16bits_data.data);
        break;
    case UI_CHANGE_VALVE_COM:  // = 0xDA
        set_valve_base(ui_command_buf[1]);
        set_valve_groupA(ui_command_buf[2]);
        set_valve_groupB(ui_command_buf[3]);
        break;
    case UI_CHANGE_PEEP_VALVE_COM: // = 0xDC
        trans_16bits_data.byte[0] = ui_command_buf[1];
        trans_16bits_data.byte[1] = ui_command_buf[2];
        set_peep_valve_control_val(trans_16bits_data.data);
        break;
    default:
        break;
    }
}


void inc_ui_com_ptr(void)
{
    ui_command_buf_ptr++;
    if(ui_command_buf_ptr > 60) // err
    {
        ui_command_buf_ptr  = 0;
        ui_command_xor_data = 0;
    }
}


void decode_communication_data(void)
{
    char ch;

    while(get_ui_byte(&ch))
    {
        if(ui_convert_flag)
        {
            ui_convert_flag = 0;
            switch(ch)
            {
            case 0:
                ui_command_buf[ui_command_buf_ptr] = UI_PROTOCOL_HEAD;
                ui_command_xor_data               ^= UI_PROTOCOL_HEAD;
                inc_ui_com_ptr();
                break;
            case 1:
                ui_command_buf[ui_command_buf_ptr] = UI_PROTOCOL_END;
                ui_command_xor_data               ^= UI_PROTOCOL_END;
                inc_ui_com_ptr();
                break;
            case 2:
                ui_command_buf[ui_command_buf_ptr] = UI_PROTOCOL_TRANSFER_CODE;
                ui_command_xor_data               ^= UI_PROTOCOL_TRANSFER_CODE;
                inc_ui_com_ptr();
                break;
            default:
                break;
            }
        }
        else
        {
            // now 1 byte data is com
            switch(ch)
            {
            case UI_PROTOCOL_HEAD:
                // new command arrived
                ui_command_buf_ptr  = 0;
                ui_command_xor_data = 0;
                break;
            case UI_PROTOCOL_END:
                // Now data is ready
                // decode data
                if(ui_command_xor_data == 0)
                {
                    decode_ui_command();
                }
                // waiting for next command
                ui_command_buf_ptr  = 0;
                ui_command_xor_data = 0;
                break;
            case UI_PROTOCOL_TRANSFER_CODE:
                ui_convert_flag = 1;
                break;

            default:
                ui_command_buf[ui_command_buf_ptr] = ch;
                ui_command_xor_data               ^= ch;
                inc_ui_com_ptr();
                break;
            }
        }

    }
}

void do_UI_500ms_communication_actions(void)
{
    static uint16_t cycles = 0;

    if((cycles++ & 0xf) == 0)
    {
        trans_Oxygen_data();
    }
    if((cycles & 1) == 1)
    {
        trans_base_board_status();
    }
}


void do_UI_1ms_communication_actions(uint16_t counter)
{
    if((counter & 0x7) == 1) // 8ms once
    {
        trans_ui_wave_data();
    }

    if((counter & 0x7f) == 10) // 128 ms once
    {
        //trans_Oxygen_Mixer_data();
        packed_pid_info_msg();
    }

    if(is_one_breath_cycle_completed())
    {
        reset_one_cycle_complete_flag();
        trans_breath_one_cycle_data();
        trans_breath_second_vt_data();
    }

    if((counter & 0x1) == 0)
    {
        //trans_Oxygen_Mixer_ctrol();
        //		上传 传感器数据
        if(is_calibration_message_ready())
        {

            if(is_sensor_data_active_send())
            {
                // send sensor data
                trans_sensor_data();
            }

            //		设置压力传感器校准值的 ACK
            if(is_calibration_press_sensor_data_set_complete())
            {
                trans_calibration_press_sensor_set_ack_data();
                // need to clear this flag
                reset_sensor_press_set_data_ack_flag();
            }

            //		上传压力传感器校准数据
            if(is_calibration_upload_sensor_press_data())
            {
                trans_calibration_press_sensor_upload_data();
                reset_sensor_press_data_upload_flag();
            }
            //		设置流量传感器校准值的ACK
            //		上传流量传感器校准值
            //		设置可控制单元的校准值的ACK
            //		上传可控制单元的校准值
            //		设置氧传感器校准值的ACK
            if(is_calibration_oxygen_sensor_data_set_complete())
            {
                trans_calibration_oxygen_sensor_set_ack_data();
                // need to clear this flag
                reset_sensor_oxygen_set_data_ack_flag();
            }
            //      读取氧传感器校准值
            if(is_calibration_upload_sensor_oxygen_data())
            {
                trans_upload_sensor_oxygen_data();
                reset_oxygen_data_upload_flag();
            }
        }

        //return;
    }
    else
    {
        trans_PPFFVV_data();
    }
}


// end of file: UI_data.c
