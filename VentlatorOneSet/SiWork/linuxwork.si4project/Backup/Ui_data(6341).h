#include "../global.h"

#ifndef __UI_DATA_H
#define __UI_DATA_H

typedef struct __ST_BASE_INFO_ACTIVED_FLAG
{
    // 暂时将控制信息和测试控制信息放在一块，将来有必要时在做处理
    uint16_t oxygen_mixer_ctrol_info: 1;
    uint16_t oxygen_mixer_ctrol_pid: 1;
    uint16_t reserved: 14;
} ST_BASE_INFO_ACTIVED_FLAG;
extern ST_BASE_INFO_ACTIVED_FLAG base_info_flags;

// 用于系统发送显示信息的通知标记
typedef struct __ST_UI_MESSAGE_SEND_FLAG
{
    uint16_t breath_one_cycle_completed_flag: 1;
    //	上传 传感器数据
    uint16_t sensor_data_active_flag        : 1;
    //	设置压力传感器校准值的 ACK
    uint16_t sensor_press_set_data_ack_flag : 1;
    //	上传压力传感器校准数据
    uint16_t sensor_press_data_upload_flag  : 1;
    //	设置流量传感器校准值的ACK
    uint16_t sensor_flow_set_data_ack_flag  : 1;
    //	上传流量传感器校准值
    uint16_t sensor_flow_data_upload_flag   : 1;
    //	设置可控制单元的校准值的ACK
    uint16_t list_data_set_ack_flag         : 1;
    //	上传可控制单元的校准值
    uint16_t list_data_upload_flag          : 1;
    //	设置氧传感器校准值的ACK
    uint16_t oxygen_set_data_ack_flag       : 1;
    //	读取氧传感器校准值
    uint16_t oxygen_data_upload_flag        : 1;

    uint16_t reserved: 6;
} ST_UI_MESSAGE_SEND_FLAG;
extern ST_UI_MESSAGE_SEND_FLAG st_ui_send_message_flag;

// 用于系统发送报警信息的通知标记
typedef struct __ST_UI_ALARM_SEND_FLAG
{
    uint16_t alarm_apea_flag: 1;
    uint16_t alarm_high_airway_press_flag: 1;
    uint16_t reserved: 14;
} ST_UI_ALARM_SEND_FLAG;
extern ST_UI_ALARM_SEND_FLAG st_ui_send_alarm_flag;

void reset_ui_data(void);
void trans_ui_wave_data(void);
void packed_commnication_data(uint8_t *ptr, int len);
void decode_communication_data(void);
void reset_ui_command_buf(void);
void set_one_cycle_complete_flag(void);
void reset_one_cycle_complete_flag(void);

void set_sensor_data_active_flag(void);
void reset_sensor_data_active_flag(void);


#endif


// file Ui_data.h ends
