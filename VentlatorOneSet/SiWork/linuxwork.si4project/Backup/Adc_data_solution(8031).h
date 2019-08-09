// File:Adc_data_solution.h
#ifndef ADC_DATA_SOLUTION_H
#define ADC_DATA_SOLUTION_H

#define EEPROM_OPERATION_SUCCEED 0
#define EEPROM_OPERATION_FALSE   1

#define MAX_EEPROM_LIST_DATA_NUM 50

#define ST_EEPROM_PRESS_DATA_LEN sizeof(ST_EEPROM_PRESS_DATA)
#define ST_EEPROM_OXYGEN_DATA_LEN sizeof(ST_EEPROM_OXYGEN_DATA)

#define EEPROM_LIST_DATA_LENTH   sizeof(EEPROM_LIST_DATA)
#define null ((unsigned char *) 0)

#define DEFAULT_EXHALE_LOW_FLOW_DATA_LENTH  18
#define DEFAULT_EXHALE_HIGH_FLOW_DATA_LENTH 37
#define DEFAULT_OXYGEN_FLOW_DATA_LENTH      16
#define DEFAULT_BLOWER_PRESS_DATA_LENTH     20
#define DEFAULT_BLOWER_SPEED_DATA_LENTH     34
#define DEFAULT_BLOWER_FLOW0_DATA_LENTH     34
#define DEFAULT_BLOWER_VACUUM_DATA_LENTH    13
#define DEFAULT_VALVE_PEEP_DATA_LENTH       25
#define DEFAULT_VALVE_OXYGEN_DATA_LENTH     30

typedef enum
{
    // PRESS
    EM_CALIBRATION_FLOW_DATA = 0,
    EM_CALIBRATION_LIST_DATA = 1,
    EM_CALIBRATION_OTHER_DATA = 2
} __ENUM_EEPROM_LIST_DATA_OR_FLOW_DATA;


typedef enum
{
    // PRESS
    EM_SENSOR_PRESS_INHALE,
    EM_SENSOR_PRESS_EXHALE,
    EM_SENSOR_PRESS_VACUUM,
    EM_SENSOR_PRESS_OTHERS
} ENUM_EEPROM_sensor_press_DATA_TYPES;

typedef enum
{
    EM_LIST_DATA_FLOW_EXHALE_LOW_DATA  = 0x0,
    EM_LIST_DATA_FLOW_EXHALE_HIGH_DATA = 0x1,
    EM_LIST_DATA_FLOW_OXYGEN_DATA      = 0x2,
    EM_LIST_DATA_BLOWER_PRESS_DATA     = 0x10,
    EM_LIST_DATA_BLOWER_SPEED_DATA     = 0x11,
    EM_LIST_DATA_BLOWER_FLOW0_DATA     = 0x12,
    EM_LIST_DATA_BLOWER_VACUUM_DATA    = 0x13,
    EM_LIST_DATA_VALVE_PEEP_DATA       = 0x14,
    EM_LIST_DATA_VALVE_OXYGEN_DATA     = 0x15,
    EM_LIST_DATA_VALVE_BACKFLOW_DATA   = 0x16
} ENUM_EEPROM_LIST_DATA_TYPES;
typedef struct __ST_EEPROM_OXYGEN_DATA
{
    //            name        lenth   locate
    unsigned int head;         //4      0-3
    unsigned int P_step;       //4      4-7
    // base
    unsigned short int base;   //2      8-9
    unsigned char status;      //1       10
    // XOR
    unsigned char xor_byte;    //1       11
} ST_EEPROM_OXYGEN_DATA;


typedef union
{
    ST_EEPROM_OXYGEN_DATA data;
    unsigned char bytes[ST_EEPROM_OXYGEN_DATA_LEN];
} UNION_EEPROM_OXYGEN_DATA;
extern UNION_EEPROM_OXYGEN_DATA eeprom_oxygen_data;
typedef struct __ST_EEPROM_PRESS_DATA
{
    //       name        lenth locate
    unsigned int head;   //4      0-3
    // P_step
    unsigned int P_step; //4      4-7
    // N_step
    unsigned int N_step; //4      8-11
    // base
    unsigned short int base; //2      12-13
    unsigned char status; //1      14
    // XOR
    unsigned char xor_byte; //1      15
} ST_EEPROM_PRESS_DATA;

typedef union
{
    ST_EEPROM_PRESS_DATA data;
    unsigned char bytes[ST_EEPROM_PRESS_DATA_LEN];
} UNION_EEPROM_PRESS_DATA;
extern UNION_EEPROM_PRESS_DATA eeprom_press_data;

typedef struct __EEPROM_LIST_POINT
{
    unsigned int real_val;
    unsigned short int cpu_managed_val; // sensor/valve data
} EEPROM_LIST_POINT;

typedef struct __EEPROM_LIST_DATA
{
    //       name        lenth locate
    unsigned int head;    //4      0-3
    EEPROM_LIST_POINT data[MAX_EEPROM_LIST_DATA_NUM]; //300 bytes = 6 * 50
    unsigned int sum_dat; //4
    unsigned short int lenth; //2
    unsigned char unused; //1
    unsigned char xor_dat; //1
} EEPROM_LIST_DATA; // total 4+300+4+2+1+1
extern EEPROM_LIST_DATA eeprom_list_data;

#if 0
{
    //       name        lenth locate
    unsigned int head    : 32;//4      0-3
    unsigned int lenth   : 8;//1        4

    //       name        lenth locate  Data num 1-10
    unsigned int index1  : 24;//3     05-07 // 1
    unsigned int data1   : 16;//2     08-09
    unsigned int index2  : 24;//3     10-12 // 2
    unsigned int data2   : 16;//2     13-14
    unsigned int index3  : 24;//3     15-17 // 3
    unsigned int data3   : 16;//2     18-19
    unsigned int index4  : 24;//3     20-22 // 4
    unsigned int data4   : 16;//2	 23-24
    unsigned int index5  : 24;//3     25-27 // 5
    unsigned int data5   : 16;//2     28-29
    unsigned int index6  : 24;//3     30-32 // 6
    unsigned int data6   : 16;//2     33-34
    unsigned int index7  : 24;//3     35-37 // 7
    unsigned int data7   : 16;//2     38-39
    unsigned int index8  : 24;//3     40-42 // 8
    unsigned int data8   : 16;//2     43-44
    unsigned int index9  : 24;//3     45-47 // 9
    unsigned int data9   : 16;//2     48-49
    unsigned int index10  : 24;//3     50-52 // 0
    unsigned int data10   : 16;//2     53-54
    //       name        lenth locate  Data num 11-20
    unsigned int index11  : 24;//3     55-57 // 1
    unsigned int data11   : 16;//2     58-59
    unsigned int index12  : 24;//3     60-62 // 2
    unsigned int data12   : 16;//2     63-64
    unsigned int index13  : 24;//3     65-67 // 3
    unsigned int data13   : 16;//2     68-69
    unsigned int index14  : 24;//3     70-72 // 4
    unsigned int data14   : 16;//2	 73-74
    unsigned int index15  : 24;//3     75-77 // 5
    unsigned int data15   : 16;//2     78-79
    unsigned int index16  : 24;//3     80-82 // 6
    unsigned int data16   : 16;//2     83-84
    unsigned int index17  : 24;//3     85-87 // 7
    unsigned int data17   : 16;//2     88-89
    unsigned int index18  : 24;//3     90-92 // 8
    unsigned int data18   : 16;//2     93-94
    unsigned int index19  : 24;//3     95-97 // 9
    unsigned int data19   : 16;//2     98-99
    unsigned int index20  : 24;//3     00-02 // 0   (100+)
    unsigned int data21   : 16;//2     03-04

    //       name        lenth locate  Data num 21-30 (100+)
    unsigned int index21  : 24;//3     05-07 // 1
    unsigned int data21   : 16;//2     08-09
    unsigned int index22  : 24;//3     10-12 // 2
    unsigned int data22   : 16;//2     13-14
    unsigned int index23  : 24;//3     15-17 // 3
    unsigned int data23   : 16;//2     18-19
    unsigned int index24  : 24;//3     20-22 // 4
    unsigned int data24   : 16;//2	 23-24
    unsigned int index25  : 24;//3     25-27 // 5
    unsigned int data25   : 16;//2     28-29
    unsigned int index26  : 24;//3     30-32 // 6
    unsigned int data26   : 16;//2     33-34
    unsigned int index27  : 24;//3     35-37 // 7
    unsigned int data27   : 16;//2     38-39
    unsigned int index28  : 24;//3     40-42 // 8
    unsigned int data28   : 16;//2     43-44
    unsigned int index29  : 24;//3     45-47 // 9
    unsigned int data29   : 16;//2     48-49
    unsigned int index30  : 24;//3     50-52 // 0
    unsigned int data30   : 16;//2     53-54

    //       name        lenth locate  Data num 31-40
    unsigned int index31  : 24;//3     55-57 // 1
    unsigned int data31   : 16;//2     58-59
    unsigned int index32  : 24;//3     60-62 // 2
    unsigned int data32   : 16;//2     63-64
    unsigned int index33  : 24;//3     65-67 // 3
    unsigned int data33   : 16;//2     68-69
    unsigned int index34  : 24;//3     70-72 // 4
    unsigned int data34   : 16;//2	 73-74
    unsigned int index35  : 24;//3     75-77 // 5
    unsigned int data35   : 16;//2     78-79
    unsigned int index36  : 24;//3     80-82 // 6
    unsigned int data36   : 16;//2     83-84
    unsigned int index37  : 24;//3     85-87 // 7
    unsigned int data37   : 16;//2     88-89
    unsigned int index38  : 24;//3     90-92 // 8
    unsigned int data38   : 16;//2     93-94
    unsigned int index39  : 24;//3     95-97 // 9
    unsigned int data39   : 16;//2     98-99
    unsigned int index40  : 24;//3     00-02 // 0  (200+)
    unsigned int data40   : 16;//2     03-04

    //       name        lenth locate  Data num 41-50 (200+)
    unsigned int index41  : 24;//3     05-07 // 1
    unsigned int data41   : 16;//2     08-09
    unsigned int index42  : 24;//3     10-12 // 2
    unsigned int data42   : 16;//2     13-14
    unsigned int index43  : 24;//3     15-17 // 3
    unsigned int data43   : 16;//2     18-19
    unsigned int index44  : 24;//3     20-22 // 4
    unsigned int data44   : 16;//2	  23-24
    unsigned int index45  : 24;//3     25-27 // 5
    unsigned int data45   : 16;//2     28-29
    unsigned int index46  : 24;//3     30-32 // 6
    unsigned int data46   : 16;//2     33-34
    unsigned int index47  : 24;//3     35-37 // 7
    unsigned int data47   : 16;//2     38-39
    unsigned int index48  : 24;//3     40-42 // 8
    unsigned int data48   : 16;//2     43-44
    unsigned int index49  : 24;//3     45-47 // 9
    unsigned int data49   : 16;//2     48-49
    unsigned int index50  : 24;//3     50-52 // 0
    unsigned int data50   : 16;//2     53-54

    unsigned int xor_byte : 8; //1     255
}
#endif

typedef struct __EEPROM_LIST_WORK_DATA_DEFINES
{
    unsigned char lenth;
    unsigned int real_val[MAX_EEPROM_LIST_DATA_NUM];
    unsigned int cpu_managed_val[MAX_EEPROM_LIST_DATA_NUM];
} EEPROM_LIST_WORK_DATA_DEFINES;

// //////////////////////////////
// calibration list data
// //////////////////////////////
extern EEPROM_LIST_WORK_DATA_DEFINES eeprom_flow_ex_low_data;
extern EEPROM_LIST_WORK_DATA_DEFINES eeprom_flow_ex_high_data;
extern EEPROM_LIST_WORK_DATA_DEFINES eeprom_flow_oxygen_data;
extern EEPROM_LIST_WORK_DATA_DEFINES eeprom_blower_press_data;
extern EEPROM_LIST_WORK_DATA_DEFINES eeprom_blower_speed_data;
extern EEPROM_LIST_WORK_DATA_DEFINES eeprom_blower_vacuum_data;
extern EEPROM_LIST_WORK_DATA_DEFINES eeprom_valve_peep_data;
extern EEPROM_LIST_WORK_DATA_DEFINES eeprom_valve_oxygen_data;
extern EEPROM_LIST_WORK_DATA_DEFINES eeprom_valve_backflow_data;

// //////////////////////////////
// default calibration data src
// //////////////////////////////
extern unsigned int blower_control_idx[],        blower_control_data[];
extern unsigned int expire_flow_val_data[],      expire_flow_sensor_data[];
extern unsigned int expire_flow_high_idx_data[], expire_flow_high_data[];
extern unsigned int Oxygen_flow_val_data[],      Oxygen_flow_sensor_data[];
extern unsigned int vacuum_pump_press_data[],    vacuum_pump_press_index[];
extern unsigned int blower_control_idx[],        blower_control_press_flow_0[];
extern unsigned int Oxgen_Valve_Flow_data[],     Oxgen_Valve_Control_data[];
extern unsigned int PEEP_VALVE_DATA[],           PEEP_VALVE_IDX[],               PEEP_BLOWER_DATA[];


// ******** press ******************************************************
unsigned char eeprom_load_press_data(unsigned char type); //__ENUM_SENSOR_PRESS_TYPES type
unsigned char eeprom_save_press_data(unsigned char type); //__ENUM_SENSOR_PRESS_TYPES type
unsigned char eeprom_check_press_xor(UNION_EEPROM_PRESS_DATA *press_data_ptr);
void          eeprom_append_press_xor(UNION_EEPROM_PRESS_DATA *press_data_ptr);
void          ui_get_work_press_data(uint8_t type);
void          ui_set_sensor_press_data(uint8_t *bufptr);

// **********List data ************************************************
// copy sensor data or valve data to eeprom_list_data
void          eeprom_copy_real_to_work_data(unsigned char data_type); // to eeprom_list_data
// copy eeprom_list_data to sensor data or valve data
void          eeprom_copy_work_to_real_data(unsigned char data_type); // copy from eeprom_list_data
unsigned char eeprom_check_list_data(void);
unsigned char eeprom_load_list_data(unsigned char data_type);
void          save_eeprom_list_data(unsigned char data_type);
void          active_eeprom_list_data(unsigned char data_type);
void          active_default_eeprom_list_data(unsigned char data_type);
void          set_list_data_from_buf(unsigned char *buf, unsigned char data_type);

//************Oxygen *********************************************
void          eeprom_add_xordata_to_list_data(void);
void          ui_set_sensor_oxygen_data(uint8_t *bufptr);
unsigned char *ui_get_work_flow_data(uint8_t flow_type);
unsigned char *ui_get_work_list_data(uint8_t list_type);
void          trans_eeprom_oxygen_data_from_real(void);
void          trans_eeprom_oxygen_data_to_real(void);
unsigned char eeprom_load_oxygen_concentration_data(void);

// ***********Others *********************************************
unsigned short int get_eeprom_temp_buf_lenth(void);
void               load_all_valve_default_calibration_data(void);

#endif//#ifndef ADC_DATA_SOLUTION_H
