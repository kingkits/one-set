#include "stm32f4xx_hal.h"
#include "arm_math.h"
#include "cmsis_os.h"
#include "../adc/adc_manager.h"
#include "../lib/insert_data.h"
#include "flow_sensor.h"
extern I2C_HandleTypeDef hi2c2;
//extern DMA_HandleTypeDef hdma_i2c2_rx;
//extern DMA_HandleTypeDef hdma_i2c2_tx;

#define FLOW_SENSOR_BASE_LENGTH 60
#define FLOW_SENSOR_DATA_LENGTH 64
//static uint8_t TSI_data_is_ready = 0;
typedef struct __STFLOW_SENSOR_BASE
{
    uint16_t  crc;//2 byte
    uint8_t   sn[6];//serial_number
    uint8_t   mn[4];// model number
    uint8_t   revision;
    uint16_t  year;
    uint8_t   month;
    uint8_t   day;
    float32_t Tcal; //Tcal;
    float32_t S;
    float32_t Z;
    float32_t S2;
    float32_t Z2;
    float32_t Tcorr;
    uint8_t   cal_gas_air;
    uint16_t  air_locate;
    uint8_t   cal_oxygen;
    uint16_t  oxygen_locate;
    uint8_t   cal_gas_heliox;
    uint16_t  heliox_locate;
    uint8_t   cal_gas_4;
    uint16_t  gas_4_locate;
} ST_FLOW_SENSOR_BASE;
union __UNION_FLOW_SENSOR_BASE
{
    uint8_t bytes[FLOW_SENSOR_BASE_LENGTH];
    ST_FLOW_SENSOR_BASE base;
} flow_sensor_base;


typedef struct __ST_FLOW_SENSOR_DATA
{
    uint8_t   offset;
    float32_t K;
    uint8_t   reserved1[12];
    float32_t L;
    uint8_t   reserved2[12];
    float32_t vi;
    float32_t Ai;
    float32_t Bi;
    float32_t Ci;
} ST_FLOW_SENSOR_DATA;

union __UNION_FLOW_SENSOR_DATA
{
    uint8_t bytes[FLOW_SENSOR_DATA_LENGTH];
    ST_FLOW_SENSOR_DATA base;
} flow_sensor_data;

ST_FLOW_SENSOR_BASE *flow_sensor_base_ptr;
ST_FLOW_SENSOR_DATA *flow_sensor_data_ptr;
uint8_t flow_sensor_actived_flag;


extern uint32_t TSI_T_data[];
extern uint32_t TSI_T_idx[];
ST_INSERT_DATA tsi_flow_sensor_T_data;

#define FLOW_DATA_BUFFER_LEN 18
int32_t inspire_flow_val_buf[FLOW_DATA_BUFFER_LEN];
int inspire_flow_buffer_ptr;
void reset_inspire_flow_buffer(void)
{
    int i;
    for(i = 0; i < FLOW_DATA_BUFFER_LEN; i++)
    {
        inspire_flow_val_buf[i] = 0;
        inspire_flow_buffer_ptr = 0;
    }
}
void insert_inspire_flow_data(int32_t val)
{
    inspire_flow_val_buf[inspire_flow_buffer_ptr] = val;
    inspire_flow_buffer_ptr ++;
    if(inspire_flow_buffer_ptr >= FLOW_DATA_BUFFER_LEN)
    {
        inspire_flow_buffer_ptr = 0;
    }
}

uint32_t get_inspire_flow_data(void)
{
    int i;
    int32_t fmax, fmin;
    int32_t ftotal;
    fmax   = inspire_flow_val_buf[0];
    fmin   = inspire_flow_val_buf[0];
    ftotal = inspire_flow_val_buf[0];
    for(i = 1; i < FLOW_DATA_BUFFER_LEN; i++)
    {
        if(fmax < inspire_flow_val_buf[i]) fmax = inspire_flow_val_buf[i];
        if(fmin > inspire_flow_val_buf[i]) fmin = inspire_flow_val_buf[i];
        ftotal += inspire_flow_val_buf[i];
    }
    ftotal = ftotal - fmax - fmin;
    ftotal >>= 4; //
    return ftotal;
}
void reset_tsi_flow_sensor_T_data(void)
{
    init_insert_data(&tsi_flow_sensor_T_data, 91, TSI_T_idx, TSI_T_data, ENUM_DATA_IS_INCREASE);
}



#define FLOW_SENSOR_WP_HIGH() HAL_GPIO_WritePin(F1_EEPROM_WP_GPIO_Port, F1_EEPROM_WP_Pin, GPIO_PIN_SET)
#define FLOW_SENSOR_WP_LOW() HAL_GPIO_WritePin(F1_EEPROM_WP_GPIO_Port, F1_EEPROM_WP_Pin, GPIO_PIN_RESET)

void reset_flow_sensor_data(void)
{
    int i;
    flow_sensor_base_ptr = (ST_FLOW_SENSOR_BASE *) & (flow_sensor_base.base);
    flow_sensor_data_ptr = (ST_FLOW_SENSOR_DATA *) & (flow_sensor_data.base);
    for(i = 0; i < FLOW_SENSOR_BASE_LENGTH; i++)
    {
        flow_sensor_base.bytes[i] = 0;
    }
    for(i = 0; i < FLOW_SENSOR_DATA_LENGTH; i++)
    {
        flow_sensor_data.bytes[i] = 0;
    }
    //FLOW_SENSOR_WP_HIGH();
}

// 初始化tsi所需的io等外围设备
void init_tsi_peripheral(void)
{
    // I2C2 + EEPROM_WP_HIGH
    reset_flow_sensor_data();
}

float32_t reverse_float(float32_t f)
{
    char ch;
    union
    {
        float32_t f;
        char buf[4];
    } f_data;

    f_data.f      = f;
    ch            = f_data.buf[0];
    f_data.buf[0] = f_data.buf[3];
    f_data.buf[3] = ch;
    ch            = f_data.buf[1];
    f_data.buf[1] = f_data.buf[2];
    f_data.buf[2] = ch;

    return f_data.f;
}


void load_tsi_eeprom_data(void)
{
#if 0
    union
    {
        float32_t f;
        uint32_t d;
    } d2f_data;
    //float32_t Tcal=0x41A8E148; //Tcal;
    d2f_data.d = 0x41A8E148;
    flow_sensor_base.base.Tcal = d2f_data.f;
    //    float32_t S=0x3F5B3CD5;
    d2f_data.d = 0x3F5B3CD5;
    flow_sensor_base.base.S = d2f_data.f;
    //    float32_t Z=0x3F09B478;
    d2f_data.d = 0x3F09B478;
    flow_sensor_base.base.Z = d2f_data.f;
    //    float32_t S2=0x3FA37E89;
    d2f_data.d = 0x3FA37E89;
    flow_sensor_base.base.S2 = d2f_data.f;
    //    float32_t Z2=0x3FF00000;
    d2f_data.d = 0x3FF00000;
    flow_sensor_base.base.Z2 = d2f_data.f;
    //    float32_t Tcorr=0xBEEE4480;
    d2f_data.d = 0xBEEE4480;
    flow_sensor_base.base.Tcorr = d2f_data.f;

    //float32_t K=0x43300000;
    d2f_data.d = 0x43300000;
    flow_sensor_data.base.K = d2f_data.f;
    //float32_t L=0x43440000;
    d2f_data.d = 0x43440000;
    flow_sensor_data.base.L = d2f_data.f;
    //float32_t vi=0x3E82C423;
    d2f_data.d = 0x3E82C423;
    flow_sensor_data.base.vi = d2f_data.f;
    //float32_t Ai=0xBEE028E6;
    d2f_data.d = 0xBEE028E6;
    flow_sensor_data.base.Ai = d2f_data.f;
    //float32_t Bi=0x40C2755E;
    d2f_data.d = 0x40C2755E;
    flow_sensor_data.base.Bi = d2f_data.f;
    //float32_t Ci=0x42187235;
    d2f_data.d = 0x42187235;
    flow_sensor_data.base.Ci = d2f_data.f;

    // 修正Tcorr
    flow_sensor_base.base.Tcorr *= 1000.0;

    TSI_data_is_ready = 1;
#endif
}

// 计算量比较大，不可以调用太频繁
// 每2毫秒调用一次，一秒钟调用500次，一分钟调用30000次


TSI_FLOW_SENSOR_DATA tsi_flow_sensor_tmp;

void init_tsi_tmp_data(void)
{
    tsi_flow_sensor_tmp.Q                       = 0.0;
    tsi_flow_sensor_tmp.T                       = 0;
    tsi_flow_sensor_tmp.Tover                   = 0;
    tsi_flow_sensor_tmp.tsi_flow_sensor_counter = 0;
    tsi_flow_sensor_tmp.T_is_actived            = 0;
    tsi_flow_sensor_tmp.Vf                      = 0;
    tsi_flow_sensor_tmp.Vstd                    = 0;
}


void Tsi840205_Load_Param(void);

void init_flow_sensor(void)
{
    init_tsi_peripheral();
    //load_tsi_eeprom_data(); // 装载TSI EEPROM数据
    Tsi840205_Load_Param();
    reset_tsi_flow_sensor_T_data(); // 温度插值表
    init_tsi_tmp_data(); // 过程数据
}
float trans_adc3_data_to_volt(uint16_t data);

void trans_flow_sensor_data(void)
{
#if 0
    // 变量定义
    float32_t vv, vvvvv;
    float32_t ftmp1, ftmp2, ftmp3;
    uint32_t utmp;
    if(TSI_data_is_ready == 0)
    {
        // tsi is not ready
        return ;
    }
    /* *******************************
     * 温度修正1分钟执行一次即可
     * *******************************/
    // 读取温度电压值（mv）
    tsi_flow_sensor_tmp.Vt = trans_adc3_data_to_volt(adc_voltage.adc_tsi_T);
    utmp = (uint32_t) tsi_flow_sensor_tmp.Vt;
    // 转换成温度 °C
    tsi_flow_sensor_tmp.T = (int32_t)get_insert_dec_data(utmp, &tsi_flow_sensor_T_data);
    // 由Tcorr 修正T=T+Tcorr
    tsi_flow_sensor_tmp.T += flow_sensor_base.base.Tcorr;
    // 比较21.11度
    if(tsi_flow_sensor_tmp.T >= 41110) // >21.11°C
    {
        // tover = K
        tsi_flow_sensor_tmp.Tover = flow_sensor_data.base.K;
    }
    else
    {
        // tover = L
        tsi_flow_sensor_tmp.Tover = flow_sensor_data.base.L;
    }

    /* ***************************
     * 流量值需要每次都修正
     * ***************************/
    // 得到流量电压值 Vf： flow voltage （mV）
    tsi_flow_sensor_tmp.Vf = trans_adc3_data_to_volt(adc_voltage.adc_tsi_V);
    // 计算标准参考值
    // Vfstd = (Vf + Z) * sqrt((Tover-21.11)/(Tover - T)) - Z
    ftmp1 = tsi_flow_sensor_tmp.Tover - 21.11;
    //(Tover - T)
    ftmp3 = tsi_flow_sensor_tmp.T - 20000.0;
    ftmp3 /= 1000.0; // 先转换到真实温度
    ftmp2 = tsi_flow_sensor_tmp.Tover - ftmp3;
    ftmp3 = ftmp1 / ftmp2;
    arm_sqrt_f32(ftmp3, &ftmp3);
    ftmp1 = tsi_flow_sensor_tmp.Vf / 1000.0 + flow_sensor_base.base.Z;
    tsi_flow_sensor_tmp.Vstd = ftmp1 * ftmp3 - flow_sensor_base.base.Z;


    // 由标准参考值计算流量
    // Q = A + B * V * V + C * V * V * V * V * V
    //            (2次方)        <--- (5次方)---->
    vv = tsi_flow_sensor_tmp.Vstd * tsi_flow_sensor_tmp.Vstd;
    vvvvv = vv * vv;
    vvvvv = vvvvv * tsi_flow_sensor_tmp.Vstd;
    tsi_flow_sensor_tmp.Q = flow_sensor_data.base.Ai;
    tsi_flow_sensor_tmp.Q += flow_sensor_data.base.Bi * vv;
    tsi_flow_sensor_tmp.Q += flow_sensor_data.base.Ci * vvvvv;
    // 转换成整形（ml/min）并返回
    //tsi_flow_sensor_tmp.Q = tsi_flow_sensor_tmp.Q * 1000; // 由L/Min --> mL/Min

    tsi_flow_sensor_tmp.Q *= 19.622;//only for test
    adc_convert_data.flow_1_val = (long)tsi_flow_sensor_tmp.Q;
#endif
}





