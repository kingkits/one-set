// File: Calibration.h
#ifndef __CALIBRATION_H
#define __CALIBRATION_H
typedef enum
{
    // PRESS
    EM_EEPROM_LIST_FLOW_EX_LOW,
    EM_EEPROM_LIST_FLOW_EX_HIGH,
    EM_EEPROM_LIST_FLOW_OXYGEN,
    EM_EEPROM_LIST_BLOWER_PRESS,
    EM_EEPROM_LIST_BLOWER_SPEED,
    EM_EEPROM_LIST_BLOWER_FLOW0,
    EM_EEPROM_LIST_VALVE_PEEP,
    EM_EEPROM_LIST_BLOWER_VACUUM,
    EM_EEPROM_LIST_VALVE_FLOWBACK,
    EM_EEPROM_LIST_VALVE_OXYGEN,
    EM_EEPROM_LIST_OTHERS
} __ENUM_LIST_TYPES;

typedef enum
{
    EM_SENSOR_DEFULT, // Has not beed calibrated
    EM_SENSOR_DATA_IS_IN_CALIBRATING,
    EM_SENSOR_DATA_IS_OK
} __ENUM_SENSOR_CALIBRATION_DATA_STATUS;


// 要区分校准和使用时的数据，不要放在一个结构中
typedef struct __ST_CALIBRATION_DATA
{
    // Press
    // inhale press sensor
    uint8_t   inhale_press_flag;
    uint16_t  inhale_press_base;
    uint16_t inhale_press_step;

    uint8_t   exhale_press_flag;
    uint16_t  exhale_press_base;
    uint16_t exhale_press_P; // 正压
    uint16_t exhale_press_N_step; // 负压

    uint8_t  vacuum_press_flag;
    uint16_t vacuum_press_base;
    uint16_t vacuum_press_step;
    // Flow

    // Valve
} ST_CALIBRATION_DATA;

typedef struct __ST_SENSOR_BASE_DEFINED
{
    // Press
    // inhale press sensor
    uint16_t  inhale_press_base;
    float32_t inhale_press_step;

    uint16_t  exhale_press_base;
    float32_t exhale_press_P_step; // 正压
    float32_t exhale_press_N_step; // 负压

    uint16_t  vacuum_press_base;
    uint16_t  vacuum_press_step;
} ST_SENSOR_BASE_DEFINED;


typedef struct __SENSOR_PTR_DEFINE
{
    ///////////////////////////////// press
    uint16_t press_i_base;         // inhale
    float    press_i_P_step;       // inhale
    float    press_i_N_step;       // inhale
    uint16_t press_e_base;         // exhale
    float    press_e_P_step;       // exhale
    float    press_e_N_step;       // exhale
    uint16_t press_c_base;         // cough
    float    press_c_P_step;       // cough
    float    press_c_N_step;       // cough

    ///////////////////////////////// flow
    uint8_t  e_l_flow_data_len;      // exhale flow (Low)
    uint32_t *e_l_flow_real_val_ptr;      // exhale flow (Low)
    uint32_t *e_l_flow_sensor_ptr;   // exhale flow (Low)
    uint8_t  e_h_flow_data_len;    // exhale flow (High)
    uint32_t *e_h_flow_real_val_ptr;    // exhale flow (High)
    uint32_t *e_h_flow_sensor_ptr; // exhale flow (High)
    uint8_t  O2_flow_data_len;     // O2 flow
    uint32_t *O2_flow_real_val_ptr;     // O2 flow
    uint32_t *O2_flow_sensor_ptr;  // O2 flow

    ///////////////////////////////// Other
    uint16_t O2_base;
    float    O2_step;
} SENSOR_PTR_DEFINE;
extern SENSOR_PTR_DEFINE sensor_data;


extern ST_INSERT_DATA  blower_flow_0_control_data;// get_blower_speed_for_press (flow = 0)
extern ST_INSERT_DATA  blower_press_control_data; // get_blower_speed_for_press_and_flow
extern ST_INSERT_DATA  blower_speed_control_data;           // set speed
extern ST_INSERT_DATA  oxygen_valve_control_data;
extern ST_INSERT_DATA  peep_valve_control_data;
extern ST_INSERT_DATA  vacuum_pump_press_control_data;

typedef struct __VALVE_PTR_DEFINE
{
    uint8_t  blower_speed_data_len;
    uint32_t *blower_speed_real_val_ptr;
    uint32_t *blower_speed_cpu_managed_val_ptr;

    uint8_t  blower_flow_0_data_len;
    uint32_t *blower_flow_0_real_val_ptr;
    uint32_t *blower_flow_0_cpu_managed_val_ptr;

    uint8_t  blower_press_data_len;
    uint32_t *blower_press_real_val_ptr;
    uint32_t *blower_press_cpu_managed_val_ptr;


    uint8_t  back_flow_valve_data_len;
    uint32_t *back_flow_valve_real_val_ptr;
    uint32_t *back_flow_valve_cpu_managed_val_ptr;

    uint8_t  peep_valve_data_len;
    uint32_t *peep_valve_real_val_ptr;
    uint32_t *peep_valve_cpu_managed_val_ptr;

    uint8_t  oxygen_valve_control_data_len;
    uint32_t *oxygen_valve_real_val_ptr;
    uint32_t *oxygen_valve_cpu_managed_val_ptr;

    uint8_t  vacuum_pump_data_len;
    uint32_t *vacuum_pump_real_val_ptr;
    uint32_t *vacuum_pump_cpu_managed_val_ptr;
} VALVE_PTR_DEFINE;

extern VALVE_PTR_DEFINE valve_data;


void     load_default_calibration_data(void);
int16_t  trans_adc_press_c(uint16_t data);
int16_t  trans_adc_press_e(uint16_t data);
int16_t  trans_adc_press_i(uint16_t data);
int16_t trans_adc_oxygen(uint16_t data);

#endif
