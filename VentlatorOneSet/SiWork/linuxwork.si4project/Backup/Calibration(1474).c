//File: Calibration.c
#include "stm32f4xx_hal.h"
#include "cmsis_os.h"
#include "../global.h"
#include "arm_math.h"
#include "../lib/insert_data.h"
#include "../lib/MovingAverage.h"
#include "Calibration.h"
#include "Adc_data_solution.h"
void FM25V05_WRITE(uint32_t WAddr, uint8_t *pBuf, uint32_t num);
void FM25V05_READ(uint32_t RAddr, uint8_t *pBuf, uint32_t num);

extern uint32_t Oxgen_Valve_Flow_data[];
extern uint32_t Oxgen_Valve_Control_data[];
extern uint32_t PEEP_VALVE_IDX[];
extern uint32_t PEEP_VALVE_DATA[];
extern uint32_t vacuum_pump_press_data[];
extern uint32_t vacuum_pump_press_index[];

extern uint32_t expire_flow_val_data[];
extern uint32_t expire_flow_sensor_data[];
extern uint32_t expire_flow_high_idx_data[];
extern uint32_t expire_flow_high_data[];
extern uint32_t Oxygen_flow_val_data[];
extern uint32_t Oxygen_flow_sensor_data[];

extern uint32_t blower_control_idx[];
extern uint32_t blower_control_press_flow_0[];
extern uint32_t blower_control_idx[], blower_control_data[];


// **************************************************************************************
// default data defined
// **************************************************************************************
#define PRESS1_STEP_VAL      0.51150895140664961636828644501279
#define PRESS1_BASE          0x81c
#define PRESS2_STEP_VAL1     0.61458333333333333333333333333333
#define PRESS2_STEP_VAL2     0.61780738946093276801938219261054
#define PRESS2_BASE          2059
#define COUGH_PRESS_STEP_VAL 0.630952380952381
#define COUGH_PRESS_BASE     2035
#define sensor_oxygen_BASE   760
#define sensor_oxygen_STEP   0.574

// ************
// global data
// ************
VALVE_PTR_DEFINE valve_data;
SENSOR_PTR_DEFINE sensor_data;


// **************************************************************************************
// Code Start
// **************************************************************************************

/**
 *
 *
 * @method load_spi_ram_data
 * @param  types             [description]
 * @return                   [description]
 */
uint8_t load_spi_ram_data(void)
{
    int data_type;

    // press (inhale/exhale/vacuum)
    for(data_type = EM_SENSOR_PRESS_INHALE; data_type < EM_SENSOR_PRESS_OTHERS; data_type++)
    {
        eeprom_load_press_data(data_type);
    }

    // oxygen
    if(eeprom_load_oxygen_concentration_data() == EEPROM_OPERATION_SUCCEED)
    {
        trans_eeprom_oxygen_data_to_real();
        return EEPROM_OPERATION_SUCCEED;
    }
    else
    {
        return EEPROM_OPERATION_FALSE;
    }

    // flow & list data
    for(data_type = EM_EEPROM_LIST_FLOW_EX_LOW; data_type < EM_EEPROM_LIST_OTHERS; data_type++)
    {
        eeprom_load_list_data(data_type);
        if(eeprom_check_list_data() == EEPROM_OPERATION_SUCCEED)
        {
            eeprom_copy_work_to_real_data(data_type);
            active_eeprom_list_data(data_type);
        }
    }
}


/**
 *
 *
 * @method load_default_calibration_data
 */
void load_default_calibration_data(void)
{
    // 先调用缺省数据
    sensor_data.press_c_base          = COUGH_PRESS_BASE;
    sensor_data.press_c_N_step        = COUGH_PRESS_STEP_VAL;
    sensor_data.press_c_P_step        = 0.0;

    sensor_data.press_e_base          = PRESS2_BASE;
    sensor_data.press_e_N_step        = PRESS2_STEP_VAL2;
    sensor_data.press_e_P_step        = PRESS2_STEP_VAL1;

    sensor_data.press_i_base          = PRESS1_BASE;
    sensor_data.press_i_N_step        = 0.0;
    sensor_data.press_i_P_step        = PRESS1_STEP_VAL;

    sensor_data.O2_base               = sensor_oxygen_BASE;
    sensor_data.O2_step               = sensor_oxygen_STEP;

#if 0
    //	sensor_data.e_l_flow_data_len     = DEFAULT_EXHALE_LOW_FLOW_DATA_LENTH;
    //	sensor_data.e_l_flow_real_val_ptr = expire_flow_val_data;
    //	sensor_data.e_l_sensor_flow_ptr   = expire_flow_sensor_data;
    //
    //	sensor_data.e_h_flow_data_len     = DEFAULT_EXHALE_HIGH_FLOW_DATA_LENTH;
    //	sensor_data.e_h_flow_real_val_ptr = expire_flow_high_idx_data;
    //	sensor_data.e_h_sensor_flow_ptr   = expire_flow_high_data;
    //
    //	sensor_data.O2_flow_data_len      = DEFAULT_OXYGEN_FLOW_DATA_LENTH;
    //	sensor_data.O2_flow_real_val_ptr  = Oxygen_flow_val_data;
    //	sensor_data.O2_sensor_flow_ptr    = Oxygen_flow_sensor_data;
#endif

    load_all_valve_default_calibration_data();

    // 读取铁电中保存的数据       // 检验是否读取成功
    // 如果成功则覆盖，否则使用缺省数据
    load_spi_ram_data();
}

/**
 *
 *
 * @method Sensor_Data_Init
 */
void Sensor_Data_Init(void)
{
}

/**
 *
 *
 * @method trans_adc_press_i
 * @param  data              [description]
 * @return                   [description]
 */
int16_t  trans_adc_press_i(uint16_t data)
{
    float val;
    val = (float) (data - sensor_data.press_i_base);
    if(data > sensor_data.press_i_base)
    {
        val *= sensor_data.press_i_P_step;
    }
    else
    {
        val *= sensor_data.press_i_N_step;
    }
    return (int16_t) val;
}

/**
 *
 *
 * @method trans_adc_press_e
 * @param  data              [description]
 * @return                   [description]
 */
int16_t  trans_adc_press_e(uint16_t data)
{
    float val;
    val = (float) (data - sensor_data.press_e_base);
    if(data > sensor_data.press_e_base)
    {
        val *= sensor_data.press_e_P_step;

    }
    else
    {
        val *= sensor_data.press_e_N_step;
    }
    return (int16_t) val;
}

/**
 *
 *
 * @method trans_adc_press_c
 * @param  data              [description]
 * @return                   [description]
 */
// 负压传感器返回值依然是正值！！
int16_t  trans_adc_press_c(uint16_t data)
{
    float val;
    val = (float) (sensor_data.press_c_base - data);
    if(data > sensor_data.press_c_base)
    {
        val = 0; //*= sensor_data.press_c_P_step;

    }
    else
    {
        val *= sensor_data.press_c_N_step;
    }
    return (int16_t) val;
}

/**
 *
 *
 * @method trans_adc_oxygen
 * @param  data             [description]
 * @return                  [description]
 */
int16_t trans_adc_oxygen(uint16_t data)
{
    double val;
    int16_t result;

    val = (float) (data - sensor_data.O2_base);
    val *= sensor_data.O2_step;
    result = (int16_t)(val + 209.0);

    if(result < 150)
        return 150;
    else
        return result;
}
