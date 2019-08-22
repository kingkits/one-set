/* File: adc_manager.h

 */

// code begin
#ifndef __ADC_MANAGER_H
#define __ADC_MANAGER_H


#define MAX_ADC_DMA_DATA 16

#if 0
typedef enum _ENUM_ADC_REFRASH_FLAG_TYPES
{
    // temperature             ba9876543210
    EM_FLAG_ADC_TEMPERATURE_1   = 0b000000000001,
    EM_FLAG_ADC_TEMPERATURE_2   = 0b000000000010,
    // Pressure
    EM_FLAG_ADC_PRESSURE_HIGH   = 0b000000000100,
    EM_FLAG_ADC_PRESSURE_1      = 0b000000001000,
    EM_FLAG_ADC_PRESSURE_2      = 0b000000010000,
    EM_FLAG_ADC_PRESSURE_3      = 0b000000100000,
    EM_FLAG_ADC_PRESSURE_4      = 0b000001000000,
    // Flow
    EM_FLAG_ADC_FLOW_1_HIGH     = 0b000010000000,
    EM_FLAG_ADC_FLOW_1_LOW      = 0b000100000000,
    EM_FLAG_ADC_FLOW_2_HIGH     = 0b001000000000,
    EM_FLAG_ADC_FLOW_2_LOW      = 0b010000000000,
    // others
    EM_FLAG_ADC_OTHERS          = 0b100000000000,
    EM_FLAG_ADC_ALL_OK          = 0b111111111111
} ENUM_ADC_REFRASH_FLAG_TYPES;
#endif
extern unsigned short adc_refresh_flags;
typedef enum _ENUM_ADC3_CHANEL_DATA_TYPES
{
    EM_ADC_O2_CONCENT_1,
    EM_ADC_FLOW_AIR_O2,
    EM_ADC_O2_CONCENT_2,
    EM_ADC3_TOTAL_NUM
} ENUM_ADC3_CHANEL_DATA_TYPES;

// for adc1 chanels
typedef enum _ENUM_ADC1_CHANEL_DATA_TYPES
{
    // temperature
    EM_ADC_TEMPERATURE_1 = 0, // IN0
    //EM_ADC_REVERSE_1,       // IN1
    EM_ADC_TEMPERATURE_2   ,  // IN2
    EM_ADC1_REFERENT       ,  // IN3
    //EM_ADC_REVERSE_3,       // IN4
    //EM_ADC_REVERSE_4,       // IN5
    //EM_ADC_REVERSE_5,       // IN6
    // Pressure
    EM_ADC_PRESSURE_HIGH   ,  // IN7
    EM_ADC_PRESSURE_1      ,  // IN8
    EM_ADC_PRESSURE_4      ,  // IN9
    // Flow
    EM_ADC_FLOW_1_HIGH     ,  // IN10
    EM_ADC_FLOW_1_LOW      ,  // IN11
    EM_ADC_FLOW_2_HIGH     ,  // IN12
    EM_ADC_FLOW_2_LOW      ,  // IN13

    // pressure
    EM_ADC_PRESSURE_2      ,  // IN14
    EM_ADC_PRESSURE_3      ,  // IN15
    EM_ADC_TOTAL_NUM
} ENUM_ADC1_CHANEL_DATA_TYPES;

typedef struct _ST_ADC_DATA_TYPES_DEFINED
{
    unsigned short int O2_concent_1_val;  // oxygen concentration;
    unsigned short int O2_concent_2_val;  // oxygen concentration;
    signed short int   temperature_1_val; // degree(C) 2 decimal places
    signed short int   temperature_2_val; // degree(C) 2 decimal places
    short int          pressure_1_val;    // (cmH2O) 2 decimal places
    short int          pressure_2_val;    // (cmH2O) 2 decimal places
    short int          pressure_3_val;    // (cmH2O) 2 decimal places
    short int          pressure_4_val;    // (cmH2O) 2 decimal places
    long               flow_1_val;        // (ml/min)1 decimal places
    long               flow_2_val;        // (ml/min)1 decimal places
    long               flow_3_val;        // (ml/min)1 decimal places
    long               adc_flow_oxygen_val;        // (ml/min)1 decimal places
    long               flow_air_o2;       // (ml/min)1 decimal places
    unsigned short int adc_press_high;    // (cmH2O)
    float referent_val;
}  ST_ADC_DATA_TYPES_DEFINED;
extern ST_ADC_DATA_TYPES_DEFINED adc_convert_data;

typedef struct _ST_ADC_VOLTAGE_DEF
{
    unsigned short int adc_oxygen;
    unsigned short int adc_press_exhale; // airway
    unsigned short int adc_flow_exhale_low;  // airway
    unsigned short int adc_tsi_T;
    unsigned short int adc_tsi_V;
    unsigned short int adc_press_inhale;
    unsigned short int adc_flow_exhale_high;
    unsigned short int adc_data8;
    unsigned short int adc_data9;
    unsigned short int adc_data10;
    unsigned short int adc_press_vacuum_val;
    unsigned short int adc_data12;
    unsigned short int adc_flow_oxygen;
} ST_ADC_VOLTAGE_DEF;
extern ST_ADC_VOLTAGE_DEF adc_voltage;

typedef struct _ST_ADC_DMA_BUFFER_DEF
{
    unsigned short int adc_data1;
    unsigned short int adc_data2;
    unsigned short int adc_data3;
    unsigned short int adc_data4;
    unsigned short int adc_data5;
    unsigned short int adc_press_inhale;
    unsigned short int adc_data7;
    unsigned short int adc_data8;
    unsigned short int adc_data9;
    unsigned short int adc_data10;
    unsigned short int adc_press_vacuum_val;
    unsigned short int adc_data12;
} ST_ADC_DMA_BUFFER_DEF;



int32_t  trans_adc1_data_to_volt(uint16_t data);
float    trans_adc3_data_to_volt(uint16_t data);
uint32_t get_inhale_flow(void);
int16_t  get_current_press(void);
int16_t  get_inhale_press(void);
int16_t  get_exhale_press(void);
int32_t  get_exhale_flow(void);
uint16_t get_inhale_oxygen_concent(void);
int16_t  get_vacuum_pump_pressure(void);

#endif
//-------------File adc_manager.h end

