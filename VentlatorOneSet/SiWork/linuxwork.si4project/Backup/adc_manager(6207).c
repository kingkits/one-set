/* **********************************************************
File: adc_manager.c
 * **********************************************************/
/* Includes ------------------------------------------------------------------*/
//#include "main.h"
#include "stm32f4xx_hal.h"
#include "cmsis_os.h"
#include "../global.h"

#include "arm_math.h"
#include "../lib/insert_data.h"
#include "../lib/MovingAverage.h"

#include "Calibration.h"

extern ADC_HandleTypeDef hadc1;
extern ADC_HandleTypeDef hadc3;

extern uint32_t expire_flow_val_data[];
extern uint32_t expire_flow_sensor_data[];
extern ST_INSERT_DATA  expire_low_flow_data;

extern uint32_t expire_flow_high_idx_data[];
extern uint32_t expire_flow_high_data[];
extern ST_INSERT_DATA  expire_high_flow_data;

extern uint32_t Oxygen_flow_val_data[];
extern uint32_t Oxygen_flow_sensor_data[];
extern ST_INSERT_DATA  Oxygen_flow_data;

#define MOVING_AVERAGE_FLOW_BUF_LEN 18
// Moving Average 移动平均窗
ST_MOVING_AVERAGE_DATA_U32 MA_flow_inspire;
uint32_t MA_flow_inspire_buf[MOVING_AVERAGE_FLOW_BUF_LEN];

ST_MOVING_AVERAGE_DATA_U32 MA_flow_expire;
uint32_t MA_flow_expire_buf[MOVING_AVERAGE_FLOW_BUF_LEN];

#define extern
#include "adc_manager.h"

#define USE_ADC_RAM_DATA 1


DMARAM ST_ADC_DMA_BUFFER_DEF adc_dma_data[MAX_ADC_DMA_DATA];
#define ADC3_DMA_DATA_LEN 24
DMARAM uint16_t adc3_dma_data[ADC3_DMA_DATA_LEN];

void init_adc_moving_average_data(void)
{
    init_moving_average_data_u32(&MA_flow_expire,  MOVING_AVERAGE_FLOW_BUF_LEN, MA_flow_expire_buf);
    init_moving_average_data_u32(&MA_flow_inspire, MOVING_AVERAGE_FLOW_BUF_LEN, MA_flow_inspire_buf);
}

void init_expire_low_flow_data(void)
{
    init_insert_data(&expire_low_flow_data,  18, expire_flow_sensor_data,   expire_flow_val_data, ENUM_DATA_IS_INCREASE);
    init_insert_data(&expire_high_flow_data, 37, expire_flow_high_idx_data, expire_flow_high_data, ENUM_DATA_IS_INCREASE);
}

int32_t trans_expire_flow(uint16_t val, uint16_t val2)
{
    uint32_t current_flow;
    if(val < 3500)
    {
        current_flow = get_insert_inc_data(val,  &expire_low_flow_data);
    }
    else
    {
        current_flow = get_insert_inc_data(val2, &expire_high_flow_data);
    }

    moving_average_data_append_u32(&MA_flow_expire, current_flow);
    return current_flow;
}

void init_oxygen_flow_data(void)
{
    init_insert_data(&Oxygen_flow_data, 16, Oxygen_flow_sensor_data, Oxygen_flow_val_data, ENUM_DATA_IS_INCREASE);
}

static void clear_adc_dma_buf(void)
{
    int i;
    for(i = 0; i < MAX_ADC_DMA_DATA; i++)
    {
        adc_dma_data[i].adc_data1  = 0;
        adc_dma_data[i].adc_data2  = 0;
        adc_dma_data[i].adc_data3  = 0;
        adc_dma_data[i].adc_data4  = 0;
        adc_dma_data[i].adc_data5  = 0;
        adc_dma_data[i].adc_press_inhale  = 0;
        adc_dma_data[i].adc_data7  = 0;
        adc_dma_data[i].adc_data8  = 0;
        adc_dma_data[i].adc_data9  = 0;
        adc_dma_data[i].adc_data10 = 0;
        adc_dma_data[i].adc_press_vacuum_val = 0;
        adc_dma_data[i].adc_data12 = 0;
    }
}


// clear adc struct data (include convert counter)
static void adc_clear_struct_data(void)
{
    adc_convert_data.flow_1_val        = 0;
    adc_convert_data.flow_2_val        = 0;
    adc_convert_data.flow_3_val        = 0;
    adc_convert_data.adc_flow_oxygen_val        = 0;
    adc_convert_data.O2_concent_1_val  = 0;
    adc_convert_data.O2_concent_2_val  = 0;
    adc_convert_data.pressure_1_val    = 0;
    adc_convert_data.pressure_2_val    = 0;
    adc_convert_data.pressure_3_val    = 0;
    adc_convert_data.pressure_4_val    = 0;
    adc_convert_data.temperature_1_val = 0;
    adc_convert_data.temperature_2_val = 0;
    adc_convert_data.adc_press_high    = 0;
    adc_convert_data.flow_air_o2       = 0;
}


static void adc_clear_data(void)
{
    clear_adc_dma_buf();
    adc_clear_struct_data();
}
void init_flow_sensor(void);
void reset_inspire_flow_buffer(void);

static void adc_initialize(void)
{
#if USE_ADC_RAM_DATA
    load_default_calibration_data();
#else
    init_expire_low_flow_data();
    init_oxygen_flow_data();
#endif
    init_adc_moving_average_data();

    reset_inspire_flow_buffer();

    init_flow_sensor();
    // datad initialize
    adc_clear_data();
#if 1
    // try to start adc1
    HAL_ADC_Start(&hadc1);

    //try to satrt DMA for adc
    HAL_ADC_Start_DMA(&hadc1, (uint32_t *)adc_dma_data, MAX_ADC_DMA_DATA * EM_ADC_TOTAL_NUM);     //MAX_ADC_DMA_DATA * EM_ADC_TOTAL_NUM
#endif
    // try to start adc3
    HAL_ADC_Start(&hadc3);

    //try to satrt DMA for adc
    HAL_ADC_Start_DMA(&hadc3, (uint32_t *)adc3_dma_data, ADC3_DMA_DATA_LEN);
}

static uint16_t get_adc3_dma_data(int select)
{
    int i;
    uint16_t result, tmp;

    result = 0;
    if(select >= 3)return 0;
    for(i = 0; i < 8; i++)
    {
        tmp = adc3_dma_data[i * 3 + select];
        result += tmp & 0xfff;
    }
    result >>= 3;
    return result;
}

#if 0
static uint16_t convert_adc_o2_concent(uint16_t val)
{
    return val;
}
#endif

uint16_t get_oxygen_concentration(uint16_t adc_val);
void     Tsi840205_Get_Flow(void);
void     insert_inspire_flow_data(int32_t val);

static void adc3_trans_data(void)
{
    uint16_t adc_val;
    // 这里的氧浓度与流量传感器的计算周期应该不同，需要区分出来！！！！！！
    adc_voltage.adc_tsi_V = get_adc3_dma_data(EM_ADC_O2_CONCENT_1); // Vf
    adc_voltage.adc_tsi_T = get_adc3_dma_data(EM_ADC_FLOW_AIR_O2); // T

    Tsi840205_Get_Flow();

    //insert_inspire_flow_data(adc_convert_data.flow_1_val);

    moving_average_data_append_u32(&MA_flow_inspire, adc_convert_data.flow_1_val);

    adc_val                           = get_adc3_dma_data(EM_ADC_O2_CONCENT_2);
    adc_voltage.adc_oxygen            = adc_val;
    // trans temp and save data
#if USE_ADC_RAM_DATA
    adc_convert_data.O2_concent_2_val = trans_adc_oxygen(adc_val);
#else
    adc_convert_data.O2_concent_2_val = get_oxygen_concentration(adc_val);
#endif
}
static uint16_t get_adc_dma_data(int select)
{
    int i;
    uint16_t  result, tmp;
    uint16_t *ptr;

    result = 0;
    if(select >= EM_ADC_TOTAL_NUM) return 0;
    for(i = 0; i < MAX_ADC_DMA_DATA; i++)
    {
        ptr = (uint16_t *) & (adc_dma_data[i]);
        tmp = ptr[select];
        result += tmp & 0xfff;
    }
    result >>= 4; // /16
    return result;
}

static int16_t convert_adc_temprature(uint16_t val)
{
    int result;
    // user convertion code here
    result = (int) trans_adc1_data_to_volt(val);
    // VOUT = (10 mV/°C) (Temperature °C) + 500 mV
    result -= 500;
    if(result < -400)result = -400;

    return result;
}

static uint16_t convert_adc_press_high(uint16_t val)
{
    uint16_t result;
    // user convertion code here

    result = val;
    return result;
}

uint32_t get_inspire_flow_data(void);

__weak uint32_t get_inspire_flow(void)
{
#if USE_MA_TRANSLATOR
    return fast_get_moving_average_data_u32(&MA_flow_inspire);
#else
    return adc_convert_data.flow_1_val;
#endif
}

__weak uint32_t get_expire_flow(void)
{
#if USE_MA_TRANSLATOR
    return fast_get_moving_average_data_u32(&MA_flow_expire);
#else
    return adc_convert_data.flow_2_val;
#endif
}

__weak uint16_t get_inspire_oxygen_concent(void)
{
    return adc_convert_data.O2_concent_2_val;
}

__weak int16_t get_current_press(void)
{
    // 需要确认负数在移位时不会变成正数！
    return (adc_convert_data.pressure_1_val + adc_convert_data.pressure_4_val) >> 1;
}

__weak int16_t get_inspire_press(void)
{
    return adc_convert_data.pressure_1_val;
}

__weak int16_t get_expire_press(void)
{
    return adc_convert_data.pressure_4_val;
}

// !!! 注意 这个压力是取反了的，与实际压力相差一个负号
__weak int16_t get_vacuum_pump_pressure(void)
{
    return adc_convert_data.pressure_2_val;
}

int32_t trans_adc1_data_to_volt(uint16_t data)
{
    float32_t result;
    result  = (float32_t) data;
    result *= adc_convert_data.referent_val;

    return (int32_t) result;
}


#define PRESS1_STEP_VAL 0.51150895140664961636828644501279
#define PRESS1_BASE 0x81c
// 吸气端的压力在咳痰时也有可能是负值，需要谨慎处理
int16_t  trans_data_to_press(uint16_t data)
{
    float val;
    val = (float) (data - PRESS1_BASE);
    val *= (float)PRESS1_STEP_VAL;

    return  (int16_t) val;
}


#define PRESS2_STEP_VAL1 0.61458333333333333333333333333333
#define PRESS2_STEP_VAL2 0.61780738946093276801938219261054
#define PRESS2_BASE 2059
int16_t  trans_data_to_press2(uint16_t data)
{
    int16_t result;
    float val;
    if(data > PRESS2_BASE)
    {
        val = (float) (data - PRESS2_BASE);
        val *= (float)PRESS2_STEP_VAL1;
        result = (int16_t) val;
        return result;
    }
    else
    {
        val = (float) (data - PRESS2_BASE);
        val *= (float)PRESS2_STEP_VAL2;
        result = (int16_t) val;
        return result;
    }
}

#define COUGH_PRESS_STEP_VAL 0.630952380952381
#define COUGH_PRESS_BASE 2035
int32_t trans_vacuum_pump_pressure(uint16_t data)
{
    float val;
    val = (float) (COUGH_PRESS_BASE - data);
    val *= (float)COUGH_PRESS_STEP_VAL;

    return (int32_t) val;
}



float trans_adc3_data_to_volt(uint16_t data)
{
    float32_t result;
    result = (float32_t) data;
    //if(result < 0.0) result = 0.0;
    result *= adc_convert_data.referent_val;
    return (float32_t) result;
}

static float32_t adc_get_ref(uint16_t val)
{
    float32_t result;
    result = (float32_t)1200.0 / (float32_t)val;
    return result;
}
// 将有效的adc数据转换成可识别的数据
static void adc_trans_data(void)
{
    uint16_t adc_val, adc_val2;

    adc_val = get_adc_dma_data(EM_ADC1_REFERENT);
    // trans temp
    //save data
    adc_convert_data.referent_val = adc_get_ref(adc_val);

    adc_val = get_adc_dma_data(EM_ADC_TEMPERATURE_1);
    // trans temp
    //save data
    adc_convert_data.temperature_1_val = convert_adc_temprature(adc_val);

    adc_val = get_adc_dma_data(EM_ADC_TEMPERATURE_2);
    // trans temp
    //save data
    adc_convert_data.temperature_2_val = convert_adc_temprature(adc_val);

    adc_val = get_adc_dma_data(EM_ADC_PRESSURE_HIGH);
    // trans temp
    //save data
    adc_convert_data.adc_press_high = convert_adc_press_high(adc_val);

    adc_val = get_adc_dma_data(EM_ADC_PRESSURE_1);
    adc_voltage.adc_press_inhale = adc_val;
    // trans temp
    //save data
#if USE_ADC_RAM_DATA
    adc_convert_data.pressure_1_val = trans_adc_press_e(adc_val);
#else
    adc_convert_data.pressure_1_val = trans_data_to_press2(adc_val);
#endif

    adc_val = get_adc_dma_data(EM_ADC_PRESSURE_4);
    adc_voltage.adc_press_exhale = adc_val;
    // trans temp
    //save data
#if USE_ADC_RAM_DATA
    adc_convert_data.pressure_4_val = trans_adc_press_i(adc_val);
#else
    adc_convert_data.pressure_4_val = trans_data_to_press(adc_val);
#endif
    adc_val2 = get_adc_dma_data(EM_ADC_FLOW_1_HIGH);
    adc_val  = get_adc_dma_data(EM_ADC_FLOW_1_LOW);
    // trans temp
    adc_voltage.adc_flow_exhale_low = adc_val;
    adc_voltage.adc_flow_exhale_high = adc_val2;
    // trans temp
    //save data

    adc_convert_data.flow_2_val = trans_expire_flow(adc_val, adc_val2);

    adc_val = get_adc_dma_data(EM_ADC_FLOW_2_HIGH);

    // trans temp
    //save data
    //adc_convert_data.flow_3_val = convert_adc_flow(adc_val);



    adc_val = get_adc_dma_data(EM_ADC_FLOW_2_LOW);

    //save data
    //adc_convert_data.adc_flow_oxygen_val = convert_adc_flow(adc_val);
    adc_convert_data.adc_flow_oxygen_val = (int32_t) get_insert_inc_data(adc_val, &Oxygen_flow_data);

    adc_val = get_adc_dma_data(EM_ADC_PRESSURE_2);
    adc_voltage.adc_press_vacuum_val = adc_val;
    // trans temp
    //save data
#if USE_ADC_RAM_DATA
    adc_convert_data.pressure_2_val = trans_adc_press_c(adc_val);
#else
    adc_convert_data.pressure_2_val = trans_vacuum_pump_pressure(adc_val);//trans_adc1_data_to_volt(adc_val);
#endif

    adc_val = get_adc_dma_data(EM_ADC_PRESSURE_3);
    // trans temp
    //save data
    adc_convert_data.pressure_3_val = trans_adc1_data_to_volt(adc_val);
    // need to convert adc data to real data
}

uint16_t get_air_way_oxygen_concent(void)
{
    return adc_convert_data.O2_concent_1_val;
}

int32_t get_oxygen_flow(void)
{
    return adc_convert_data.adc_flow_oxygen_val;
}


void trans_flow_sensor_data(void);

void ADC_Task(void const *argument)
{
    /* USER CODE BEGIN ADC_Task */
    /* Initialize */
    adc_initialize();

    /* Infinite loop */
    for(;;)
    {
        adc_trans_data();
        adc3_trans_data();
        //trans_flow_sensor_data();
        osDelay(2);
    }
    /* USER CODE END ADC_Task */
}


