// File:flow_sensor.h
typedef struct __TSI_FLOW_SENSOR_DATA
{
    uint16_t tsi_flow_sensor_counter; // 用于1分钟计数， 1分钟约30000次
    uint8_t T_is_actived; // 如果连续接收了1分钟，则置1，初步的想法是休息1分钟
    float32_t Tover; // 最终要使用的补偿数据
    int32_t T;     // 真实的温度
    float32_t Vt;    // adc的电压值
    float32_t Vf;    // adc的电压值
    float32_t Vstd;  // 中间的关键参考值
    float32_t Q;     // 最后的流速
} TSI_FLOW_SENSOR_DATA;

void reset_flow_sensor_data(void);
void init_tsi_peripheral(void);

