/**
 * File:pcv_control.h
 */

#define CONST_EXHALE_FLOW 30000
typedef struct __ST_PCV_CONTROL_DAT
{
    // 基础数据
    // 基础数据-- 时间
    uint32_t breath_start_gate; // 由呼吸频率/周期决定，确定T触发时间(下一吸气开始时刻)
    uint32_t one_breath_time;    // 一个呼吸的周期
    uint32_t expire_start_gate;  // 下一呼气开始时刻
    uint32_t T_expire_3_4_gate;    // 呼气经过T呼气时间的3/4 时刻，用于检测吸气触发及PEEP计算
    // 基础数据-- 涡轮速度
    uint16_t high_speed;
    uint16_t low_speed;
    // 模式控制-- 压力上升
    float *inc_data_ptr;  //数据指针
    int inc_data_count;   //计数（）
    int max_inc_data_num; //最大计数
    // 模式控制-- 压力保持

    // 模式控制-- 压力下降（呼气）

    // 模式控制-- 触发

    // 模式控制-- 叩击
} ST_PCV_CONTROL_DAT;
extern ST_PCV_CONTROL_DAT st_pcv_control_dat;