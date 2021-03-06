/////////////////////////////////////
//   File: Adc_data_solution.c
/////////////////////////////////////

#include "stm32f4xx_hal.h"
#include "cmsis_os.h"
#include "../global.h"

#include "../lib/insert_data.h"

#include "Calibration.h"
#include "Adc_data_solution.h"

void set_sensor_press_ack_status(void);
void reset_sensor_press_ack_status(void);
void set_sensor_press_set_data_ack_flag(void);
void set_sensor_oxygen_ack_status(void);
void reset_sensor_oxygen_ack_status(void);
void set_sensor_oxygen_set_data_ack_flag(void);
void reset_sensor_oxygen_set_data_ack_flag(void);

void FM25V05_WRITE(uint32_t WAddr, uint8_t *pBuf, uint32_t num);
void FM25V05_READ(uint32_t RAddr, uint8_t *pBuf, uint32_t num);

EEPROM_LIST_DATA eeprom_list_data;

EEPROM_LIST_WORK_DATA_DEFINES eeprom_flow_ex_low_data;
EEPROM_LIST_WORK_DATA_DEFINES eeprom_flow_ex_high_data;
EEPROM_LIST_WORK_DATA_DEFINES eeprom_flow_oxygen_data;
EEPROM_LIST_WORK_DATA_DEFINES eeprom_blower_press_data;
EEPROM_LIST_WORK_DATA_DEFINES eeprom_blower_speed_data;
EEPROM_LIST_WORK_DATA_DEFINES eeprom_blower_flow0_data;
EEPROM_LIST_WORK_DATA_DEFINES eeprom_blower_vacuum_data;
EEPROM_LIST_WORK_DATA_DEFINES eeprom_valve_peep_data;
EEPROM_LIST_WORK_DATA_DEFINES eeprom_valve_oxygen_data;
EEPROM_LIST_WORK_DATA_DEFINES eeprom_valve_backflow_data;
ST_INSERT_DATA expire_low_flow_data;
ST_INSERT_DATA expire_high_flow_data;
ST_INSERT_DATA Oxygen_flow_data;
ST_INSERT_DATA blower_flow_0_control_data; // get_blower_speed_for_press (flow = 0)
ST_INSERT_DATA blower_press_control_data;  // get_blower_speed_for_press_and_flow
ST_INSERT_DATA blower_speed_control_data;            // set speed
ST_INSERT_DATA oxygen_valve_control_data;
ST_INSERT_DATA peep_valve_control_data;
ST_INSERT_DATA vacuum_pump_press_control_data;
ST_INSERT_DATA back_flow_control_data;

#define EEPROM_TEMP_BUF_LENTH           512
#define EEPROM_OXYGEN_C_DATA_HEAD       0x55550000
#define EEPROM_PRESS_DATA_HEAD          0x55551111
#define EEPROM_LIST_DATA_HEAD           0x55552222

#define EEPROM_STATUS_DATA_READY        0xA
#define EEPROM_STATUS_DATA_FALSE        0x0
#define EEPROM_PRESS_INHALE_ADDRESS     0x10
#define EEPROM_PRESS_EXHALE_ADDRESS     0x30
#define EEPROM_PRESS_VACUUM_ADDRESS     0x50
#define EEPROM_O2_CONCENTRATION_ADDRESS 0x100
#define EEPROM_FLOW_EXHALE_LOW_ADDRESS  0x0200
#define EEPROM_FLOW_EXHALE_HIGH_ADDRESS 0x0400
#define EEPROM_FLOW_OXYGEN_ADDRESS      0x0600
#define EEPROM_BLOWER_SPEED_ADDRESS     0x0800
#define EEPROM_BLOWER_FLOW0_ADDRESS     0x0A00
#define EEPROM_BLOWER_PRESS_ADDRESS     0x0C00
#define EEPROM_BLOWER_VACUUM_ADDRESS    0x0E00
#define EEPROM_VALVE_PEEP_ADDRESS       0x1000
#define EEPROM_VALVE_OXYGEN_ADDRESS     0x1200
#define EEPROM_VALVE_BACKFLOW_ADDRESS   0x1400



#define EEPROM_PRESS_DATA_MUL_FACT  0.000001f
#define EEPROM_PRESS_DATA_FACT_BASE 1000000
UNION_EEPROM_PRESS_DATA eeprom_press_data;
UNION_EEPROM_OXYGEN_DATA eeprom_oxygen_data;

uint8_t eeprom_temp_buf[EEPROM_TEMP_BUF_LENTH];
static uint16_t eeprom_temp_buf_lenth;
#define set_eeprom_temp_buf_lenth(len)  (eeprom_temp_buf_lenth = len)

uint16_t get_eeprom_temp_buf_lenth(void)
{
    return eeprom_temp_buf_lenth;
}

static void clear_eeprom_temp_buf(void)
{
    int i;
    for(i = 0; i < EEPROM_TEMP_BUF_LENTH; i++)
    {
        eeprom_temp_buf[i] = 0;
    }
}
/////////////////////////////////////////////////
// Oxygen concentration
/////////////////////////////////////////////////
uint8_t check_eeprom_buf(uint8_t *src_ptr, uint8_t *tar_ptr, int len)
{
    uint8_t xor;
    int i;
    xor = 0;
    for(i = 0; i < len; i++)
    {
        if(*src_ptr != *tar_ptr)
            return EEPROM_OPERATION_FALSE;
        xor = *src_ptr;

        // !!!这里需要确认是否能做指针的增值操作，由于指针是传输参数，直接操作可能有误
        src_ptr++;
        tar_ptr++;
    }
    if(xor != 0)
        return EEPROM_OPERATION_FALSE;
    else
        return EEPROM_OPERATION_SUCCEED;
}

void clear_eeprom_list_data(void)
{
    uint8_t *ptr;
    int i;
    ptr = (uint8_t *) &eeprom_list_data;
    for(i = 0; i < EEPROM_LIST_DATA_LENTH; i++)
    {
        *(ptr++) = 0;
    }
}

void trans_eeprom_oxygen_data_to_real(void)
{
    sensor_data.O2_base = eeprom_oxygen_data.data.base;
    sensor_data.O2_step = (float) eeprom_oxygen_data.data.P_step * EEPROM_PRESS_DATA_MUL_FACT;
}

void trans_eeprom_oxygen_data_from_real(void)
{
    float ftmp;
    eeprom_oxygen_data.data.head = EEPROM_OXYGEN_C_DATA_HEAD;
    eeprom_oxygen_data.data.status = EEPROM_STATUS_DATA_READY;

    eeprom_oxygen_data.data.base = sensor_data.O2_base;
    ftmp = sensor_data.O2_step * EEPROM_PRESS_DATA_FACT_BASE;
    eeprom_oxygen_data.data.P_step = (uint32_t) ftmp;
}

void clear_eeprom_oxygen_data(void)
{
    int i;
    for(i = 0; i < ST_EEPROM_OXYGEN_DATA_LEN; i++)
    {
        eeprom_oxygen_data.bytes[i] = 0;
    }
}

void append_eeprom_oxygen_data_xor_byte(void)
{
    int i;
    uint8_t xor;
    xor = 0;
    eeprom_oxygen_data.data.xor_byte = 0;
    for(i = 0; i < ST_EEPROM_OXYGEN_DATA_LEN; i++)
    {
        xor ^= eeprom_oxygen_data.bytes[i];
    }
    eeprom_oxygen_data.data.xor_byte = xor;
}


uint8_t check_eeprom_oxygen_data(void)
{
    int i;
    uint8_t xor;
    xor = 0;

    if(eeprom_oxygen_data.data.head != EEPROM_OXYGEN_C_DATA_HEAD)
        return EEPROM_OPERATION_FALSE;

    for(i = 0; i < ST_EEPROM_OXYGEN_DATA_LEN; i++)
    {
        xor ^= eeprom_oxygen_data.bytes[i];
    }
    //eeprom_oxygen_data.data.xor_byte = xor;

    if(xor == 0)
        return EEPROM_OPERATION_SUCCEED;
    else
        return EEPROM_OPERATION_FALSE;
}

uint8_t eeprom_save_oxygen_concentration_data(void)
{
    //	int i;
    //	uint8_t xor;
    // append xor
    append_eeprom_oxygen_data_xor_byte();
    FM25V05_WRITE(EEPROM_O2_CONCENTRATION_ADDRESS, (uint8_t *)eeprom_oxygen_data.bytes, ST_EEPROM_OXYGEN_DATA_LEN);

    // read
    FM25V05_READ(EEPROM_O2_CONCENTRATION_ADDRESS - 1, (uint8_t *)eeprom_temp_buf, ST_EEPROM_OXYGEN_DATA_LEN);

    // check
    //	xor = 0
    //	for(i=0;i<ST_EEPROM_OXYGEN_DATA_LEN;i++)
    //	{
    //		if(eeprom_oxygen_data.bytes[i] != eeprom_temp_buf[i])
    //			return EEPROM_OPERATION_FALSE;
    //		xor ^= eeprom_temp_buf[i];
    //	}

    //	if(xor != 0) return EEPROM_OPERATION_FALSE;
    //	else         return EEPROM_OPERATION_SUCCEED;
    return check_eeprom_buf((uint8_t *)eeprom_oxygen_data.bytes, (uint8_t *)eeprom_temp_buf, ST_EEPROM_OXYGEN_DATA_LEN);
}


void clear_eeprom_press_data(void)
{
    int i;
    for(i = 0; i < ST_EEPROM_PRESS_DATA_LEN; i++)
    {
        eeprom_press_data.bytes[i] = 0;
    }
}

uint8_t eeprom_check_press_xor(UNION_EEPROM_PRESS_DATA *press_data_ptr)
{
    int i;
    uint8_t x_dat;

    x_dat = 0;
    for(i = 0; i < ST_EEPROM_PRESS_DATA_LEN; i++)
    {
        x_dat ^= press_data_ptr->bytes[i];
    }

    if ((press_data_ptr->data.head == EEPROM_PRESS_DATA_HEAD) &&
            (press_data_ptr->data.status == EEPROM_STATUS_DATA_READY))
    {
        if(x_dat)
            return EEPROM_OPERATION_FALSE;
        else
            return EEPROM_OPERATION_SUCCEED;
    }

    return EEPROM_OPERATION_FALSE; // error
}

void eeprom_append_press_xor(UNION_EEPROM_PRESS_DATA *press_data_ptr)
{
    int i;
    uint8_t x_dat;

    x_dat = 0;
    for(i = 0; i < ST_EEPROM_PRESS_DATA_LEN - 1; i++)
    {
        x_dat ^= press_data_ptr->bytes[i];
    }

    press_data_ptr->data.xor_byte = x_dat;
}

uint8_t eeprom_load_press_data(uint8_t type)
{
    switch(type)
    {
    case EM_SENSOR_PRESS_INHALE:
        FM25V05_READ(EEPROM_PRESS_INHALE_ADDRESS - 1, (uint8_t *)eeprom_press_data.bytes, ST_EEPROM_PRESS_DATA_LEN);
        //FM25V05_READ(EEPROM_PRESS_INHALE_ADDRESS-1, (uint8_t *)eeprom_temp_buf, ST_EEPROM_PRESS_DATA_LEN+1);
        if(eeprom_check_press_xor((UNION_EEPROM_PRESS_DATA *) &eeprom_press_data) == EEPROM_OPERATION_FALSE)
            return EEPROM_OPERATION_FALSE; // data error need load default data
        sensor_data.press_i_base   = eeprom_press_data.data.base;
        sensor_data.press_i_N_step = (float)eeprom_press_data.data.N_step * EEPROM_PRESS_DATA_MUL_FACT;
        sensor_data.press_i_P_step = (float)eeprom_press_data.data.P_step * EEPROM_PRESS_DATA_MUL_FACT;
        return EEPROM_OPERATION_SUCCEED;

    case EM_SENSOR_PRESS_EXHALE:
        FM25V05_READ(EEPROM_PRESS_EXHALE_ADDRESS - 1, (uint8_t *)eeprom_press_data.bytes, ST_EEPROM_PRESS_DATA_LEN);
        if(eeprom_check_press_xor((UNION_EEPROM_PRESS_DATA *) &eeprom_press_data) == EEPROM_OPERATION_FALSE)
            return EEPROM_OPERATION_FALSE; // data error need load default data
        sensor_data.press_e_base   = eeprom_press_data.data.base;
        sensor_data.press_e_N_step = (float)eeprom_press_data.data.N_step * EEPROM_PRESS_DATA_MUL_FACT;
        sensor_data.press_e_P_step = (float)eeprom_press_data.data.P_step * EEPROM_PRESS_DATA_MUL_FACT;
        return EEPROM_OPERATION_SUCCEED;

    case EM_SENSOR_PRESS_VACUUM:
        FM25V05_READ(EEPROM_PRESS_VACUUM_ADDRESS - 1, (uint8_t *)eeprom_press_data.bytes, ST_EEPROM_PRESS_DATA_LEN);
        if(eeprom_check_press_xor((UNION_EEPROM_PRESS_DATA *) &eeprom_press_data) == EEPROM_OPERATION_FALSE)
            return EEPROM_OPERATION_FALSE; // data error need load default data
        sensor_data.press_c_base   = eeprom_press_data.data.base;
        sensor_data.press_c_N_step = (float)eeprom_press_data.data.N_step * EEPROM_PRESS_DATA_MUL_FACT;
        sensor_data.press_c_P_step = (float)eeprom_press_data.data.P_step * EEPROM_PRESS_DATA_MUL_FACT;
        return EEPROM_OPERATION_SUCCEED;

    default:
        // 不做处理，直接返回操作正确
        break;
    }
    return EEPROM_OPERATION_SUCCEED;
}



uint8_t eeprom_save_press_data(uint8_t type)
{
    switch(type)
    {
    case EM_SENSOR_PRESS_INHALE:
        eeprom_press_data.data.head   = EEPROM_PRESS_DATA_HEAD;
        eeprom_press_data.data.status = EEPROM_STATUS_DATA_READY;
        eeprom_press_data.data.base   = sensor_data.press_i_base;
        eeprom_press_data.data.N_step = (uint32_t)(sensor_data.press_i_N_step * EEPROM_PRESS_DATA_FACT_BASE);
        eeprom_press_data.data.P_step = (uint32_t)(sensor_data.press_i_P_step * EEPROM_PRESS_DATA_FACT_BASE);
        eeprom_append_press_xor((UNION_EEPROM_PRESS_DATA *) &eeprom_press_data);
        FM25V05_WRITE(EEPROM_PRESS_INHALE_ADDRESS, (uint8_t *)eeprom_press_data.bytes, ST_EEPROM_PRESS_DATA_LEN);
        // check
#if 0
        FM25V05_READ(EEPROM_PRESS_INHALE_ADDRESS - 1, (uint8_t *)eeprom_press_data.bytes, ST_EEPROM_PRESS_DATA_LEN);
        return eeprom_check_press_xor((UNION_EEPROM_PRESS_DATA *) &eeprom_press_data);
#else
        FM25V05_READ(EEPROM_PRESS_INHALE_ADDRESS - 1, (uint8_t *)eeprom_temp_buf, ST_EEPROM_PRESS_DATA_LEN);
        return check_eeprom_buf((uint8_t *)eeprom_press_data.bytes, eeprom_temp_buf, ST_EEPROM_PRESS_DATA_LEN);
#endif
    case EM_SENSOR_PRESS_EXHALE:
        eeprom_press_data.data.head   = EEPROM_PRESS_DATA_HEAD;
        eeprom_press_data.data.status = EEPROM_STATUS_DATA_READY;
        eeprom_press_data.data.base   = sensor_data.press_e_base;
        eeprom_press_data.data.N_step = (uint32_t)(sensor_data.press_e_N_step * EEPROM_PRESS_DATA_FACT_BASE);
        eeprom_press_data.data.P_step = (uint32_t)(sensor_data.press_e_P_step * EEPROM_PRESS_DATA_FACT_BASE);
        eeprom_append_press_xor((UNION_EEPROM_PRESS_DATA *) &eeprom_press_data);
        FM25V05_WRITE(EEPROM_PRESS_EXHALE_ADDRESS, (uint8_t *)eeprom_press_data.bytes, ST_EEPROM_PRESS_DATA_LEN);
        // check
#if 0
        FM25V05_READ(EEPROM_PRESS_EXHALE_ADDRESS - 1, (uint8_t *)eeprom_press_data.bytes, ST_EEPROM_PRESS_DATA_LEN);
        return eeprom_check_press_xor((UNION_EEPROM_PRESS_DATA *) &eeprom_press_data);
#else
        FM25V05_READ(EEPROM_PRESS_INHALE_ADDRESS - 1, (uint8_t *)eeprom_temp_buf, ST_EEPROM_PRESS_DATA_LEN);
        return check_eeprom_buf((uint8_t *)eeprom_press_data.bytes, eeprom_temp_buf, ST_EEPROM_PRESS_DATA_LEN);
#endif

    case EM_SENSOR_PRESS_VACUUM:
        eeprom_press_data.data.head   = EEPROM_PRESS_DATA_HEAD;
        eeprom_press_data.data.status = EEPROM_STATUS_DATA_READY;
        eeprom_press_data.data.base   = sensor_data.press_c_base;
        eeprom_press_data.data.N_step = (uint32_t)(sensor_data.press_c_N_step * EEPROM_PRESS_DATA_FACT_BASE);
        eeprom_press_data.data.P_step = (uint32_t)(sensor_data.press_c_P_step * EEPROM_PRESS_DATA_FACT_BASE);
        eeprom_append_press_xor((UNION_EEPROM_PRESS_DATA *) &eeprom_press_data);
        FM25V05_WRITE(EEPROM_PRESS_VACUUM_ADDRESS, (uint8_t *)eeprom_press_data.bytes, ST_EEPROM_PRESS_DATA_LEN);
        // check
#if 0
        FM25V05_READ(EEPROM_PRESS_VACUUM_ADDRESS - 1, (uint8_t *)eeprom_press_data.bytes, ST_EEPROM_PRESS_DATA_LEN);
        return eeprom_check_press_xor((UNION_EEPROM_PRESS_DATA *) &eeprom_press_data);
#else
        FM25V05_READ(EEPROM_PRESS_INHALE_ADDRESS - 1, (uint8_t *)eeprom_temp_buf, ST_EEPROM_PRESS_DATA_LEN);
        return check_eeprom_buf((uint8_t *)eeprom_press_data.bytes, eeprom_temp_buf, ST_EEPROM_PRESS_DATA_LEN);
#endif

    default:
        break;
    }
    return EEPROM_OPERATION_SUCCEED;
}



// copy sensor data or valve data to eeprom_list_data
void eeprom_copy_real_to_work_data(uint8_t data_type) // to eeprom_list_data
{
    EEPROM_LIST_WORK_DATA_DEFINES *data_ptr;
    int i;
    // set source ptr
    switch(data_type)
    {
    case EM_EEPROM_LIST_FLOW_EX_LOW:
        data_ptr = (EEPROM_LIST_WORK_DATA_DEFINES *) &eeprom_flow_ex_low_data;
        break;

    case EM_EEPROM_LIST_FLOW_EX_HIGH:
        data_ptr = (EEPROM_LIST_WORK_DATA_DEFINES *) &eeprom_flow_ex_high_data;
        break;

    case EM_EEPROM_LIST_FLOW_OXYGEN:
        data_ptr = (EEPROM_LIST_WORK_DATA_DEFINES *) &eeprom_flow_oxygen_data;
        break;

    case EM_EEPROM_LIST_BLOWER_PRESS:
        data_ptr = (EEPROM_LIST_WORK_DATA_DEFINES *) &eeprom_blower_press_data;
        break;

    case EM_EEPROM_LIST_BLOWER_SPEED:
        data_ptr = (EEPROM_LIST_WORK_DATA_DEFINES *) &eeprom_blower_speed_data;
        break;

    case EM_EEPROM_LIST_BLOWER_FLOW0:
        data_ptr = (EEPROM_LIST_WORK_DATA_DEFINES *) &eeprom_blower_flow0_data;
        break;

    case EM_EEPROM_LIST_VALVE_PEEP:
        data_ptr = (EEPROM_LIST_WORK_DATA_DEFINES *) &eeprom_valve_peep_data;
        break;

    case EM_EEPROM_LIST_BLOWER_VACUUM:
        data_ptr = (EEPROM_LIST_WORK_DATA_DEFINES *) &eeprom_blower_vacuum_data;
        break;

    case EM_EEPROM_LIST_VALVE_FLOWBACK:
        data_ptr = (EEPROM_LIST_WORK_DATA_DEFINES *) &eeprom_valve_backflow_data;
        break;

    case EM_EEPROM_LIST_VALVE_OXYGEN:
        data_ptr = (EEPROM_LIST_WORK_DATA_DEFINES *) &eeprom_valve_oxygen_data;
        break;

    default:
    case EM_EEPROM_LIST_OTHERS:
        return;
    }

    // begin
    eeprom_list_data.head  = EEPROM_LIST_DATA_HEAD;
    eeprom_list_data.lenth = data_ptr->lenth;

    for(i = 0; i < eeprom_list_data.lenth; i++)
    {
        eeprom_list_data.data[i].real_val = data_ptr->real_val[i];
        eeprom_list_data.data[i].cpu_managed_val = data_ptr->cpu_managed_val[i];
    }
}

// copy eeprom_list_data to sensor data or valve data
void eeprom_copy_work_to_real_data(uint8_t data_type) // copy from eeprom_list_data
{
    EEPROM_LIST_WORK_DATA_DEFINES *data_ptr;
    int i;
    // set target ptr
    switch(data_type)
    {
    case EM_EEPROM_LIST_FLOW_EX_LOW:
        data_ptr = (EEPROM_LIST_WORK_DATA_DEFINES *) &eeprom_flow_ex_low_data;
        break;

    case EM_EEPROM_LIST_FLOW_EX_HIGH:
        data_ptr = (EEPROM_LIST_WORK_DATA_DEFINES *) &eeprom_flow_ex_high_data;
        break;

    case EM_EEPROM_LIST_FLOW_OXYGEN:
        data_ptr = (EEPROM_LIST_WORK_DATA_DEFINES *) &eeprom_flow_oxygen_data;
        break;

    case EM_EEPROM_LIST_BLOWER_PRESS:
        data_ptr = (EEPROM_LIST_WORK_DATA_DEFINES *) &eeprom_blower_press_data;
        break;

    case EM_EEPROM_LIST_BLOWER_SPEED:
        data_ptr = (EEPROM_LIST_WORK_DATA_DEFINES *) &eeprom_blower_speed_data;
        break;

    case EM_EEPROM_LIST_BLOWER_FLOW0:
        data_ptr = (EEPROM_LIST_WORK_DATA_DEFINES *) &eeprom_blower_flow0_data;
        break;

    case EM_EEPROM_LIST_VALVE_PEEP:
        data_ptr = (EEPROM_LIST_WORK_DATA_DEFINES *) &eeprom_valve_peep_data;
        break;

    case EM_EEPROM_LIST_BLOWER_VACUUM:
        data_ptr = (EEPROM_LIST_WORK_DATA_DEFINES *) &eeprom_blower_vacuum_data;
        break;

    case EM_EEPROM_LIST_VALVE_FLOWBACK:
        data_ptr = (EEPROM_LIST_WORK_DATA_DEFINES *) &eeprom_valve_backflow_data;
        break;

    case EM_EEPROM_LIST_VALVE_OXYGEN:
        data_ptr = (EEPROM_LIST_WORK_DATA_DEFINES *) &eeprom_valve_oxygen_data;
        break;

    default:
    case EM_EEPROM_LIST_OTHERS:
        return;
    }

    // begin
    data_ptr->lenth = eeprom_list_data.lenth;

    for(i = 0; i < eeprom_list_data.lenth; i++)
    {
        data_ptr->real_val[i]        = eeprom_list_data.data[i].real_val;
        data_ptr->cpu_managed_val[i] = eeprom_list_data.data[i].cpu_managed_val;
    }
}


// copy sensor data or valve data to eeprom_list_data
void copy_default_data_to_eeprom_list_data(uint8_t data_type) // to eeprom_list_data
{
    int i;

    clear_eeprom_list_data();
    eeprom_list_data.head = EEPROM_LIST_DATA_HEAD;
    switch(data_type)
    {
    case EM_EEPROM_LIST_FLOW_EX_LOW:
        //init_insert_data(&expire_low_flow_data,  18, expire_flow_sensor_data,   expire_flow_val_data, ENUM_DATA_IS_INCREASE);
        eeprom_list_data.lenth = DEFAULT_EXHALE_LOW_FLOW_DATA_LENTH;
        for(i = 0; i < eeprom_list_data.lenth; i++)
        {
            eeprom_list_data.data[i].cpu_managed_val = expire_flow_sensor_data[i] & 0xffff;
            eeprom_list_data.data[i].real_val = expire_flow_val_data[i];
        }
        break;

    case EM_EEPROM_LIST_FLOW_EX_HIGH:
        //init_insert_data(&expire_high_flow_data, 37, expire_flow_high_idx_data, expire_flow_high_data, ENUM_DATA_IS_INCREASE);
        eeprom_list_data.lenth = DEFAULT_EXHALE_HIGH_FLOW_DATA_LENTH;
        for(i = 0; i < eeprom_list_data.lenth; i++)
        {
            eeprom_list_data.data[i].cpu_managed_val = expire_flow_high_idx_data[i] & 0xffff;
            eeprom_list_data.data[i].real_val = expire_flow_high_data[i];
        }
        break;

    case EM_EEPROM_LIST_FLOW_OXYGEN:
        //init_insert_data(&Oxygen_flow_data, 16, Oxygen_flow_sensor_data, Oxygen_flow_val_data, ENUM_DATA_IS_INCREASE);
        eeprom_list_data.lenth = DEFAULT_OXYGEN_FLOW_DATA_LENTH;
        for(i = 0; i < eeprom_list_data.lenth; i++)
        {
            eeprom_list_data.data[i].cpu_managed_val = Oxygen_flow_sensor_data[i] & 0xffff;
            eeprom_list_data.data[i].real_val = Oxygen_flow_val_data[i];
        }
        break;

    case EM_EEPROM_LIST_BLOWER_PRESS:
        //init_insert_data(&blower_press_control_data, 20, PEEP_VALVE_IDX, PEEP_BLOWER_DATA, ENUM_DATA_IS_INCREASE);
        eeprom_list_data.lenth = DEFAULT_BLOWER_PRESS_DATA_LENTH;
        for(i = 0; i < eeprom_list_data.lenth; i++)
        {
            eeprom_list_data.data[i].cpu_managed_val =  PEEP_VALVE_IDX[i] & 0xffff;
            eeprom_list_data.data[i].real_val = PEEP_BLOWER_DATA[i];
        }
        break;

    case EM_EEPROM_LIST_BLOWER_SPEED:
        //	init_insert_data(&blower_speed_control_data, 34, blower_control_idx, blower_control_data, ENUM_DATA_IS_INCREASE);
        eeprom_list_data.lenth = DEFAULT_BLOWER_SPEED_DATA_LENTH;
        for(i = 0; i < eeprom_list_data.lenth; i++)
        {
            eeprom_list_data.data[i].cpu_managed_val = blower_control_idx[i] & 0xffff;
            eeprom_list_data.data[i].real_val = blower_control_data[i];
        }
        break;

    case EM_EEPROM_LIST_BLOWER_FLOW0:
        //	init_insert_data(&blower_flow_0_control_data, 34, blower_control_press_flow_0, blower_control_idx, ENUM_DATA_IS_INCREASE);
        eeprom_list_data.lenth = DEFAULT_BLOWER_FLOW0_DATA_LENTH;
        for(i = 0; i < eeprom_list_data.lenth; i++)
        {
            eeprom_list_data.data[i].cpu_managed_val = blower_control_press_flow_0[i] & 0xffff;
            eeprom_list_data.data[i].real_val = blower_control_idx[i];
        }
        break;

    case EM_EEPROM_LIST_VALVE_PEEP:
        //init_insert_data(&peep_valve_control_data, 25, PEEP_VALVE_IDX, PEEP_VALVE_DATA, ENUM_DATA_IS_DECREASE);
        eeprom_list_data.lenth = DEFAULT_VALVE_PEEP_DATA_LENTH;
        for(i = 0; i < eeprom_list_data.lenth; i++)
        {
            eeprom_list_data.data[i].cpu_managed_val = PEEP_VALVE_IDX[i] & 0xffff;
            eeprom_list_data.data[i].real_val = PEEP_VALVE_DATA[i];
        }
        break;

    case EM_EEPROM_LIST_BLOWER_VACUUM:
        //	init_insert_data(&vacuum_pump_press_control_data, 13, vacuum_pump_press_index, vacuum_pump_press_data, ENUM_DATA_IS_INCREASE);
        eeprom_list_data.lenth = DEFAULT_BLOWER_VACUUM_DATA_LENTH;
        for(i = 0; i < eeprom_list_data.lenth; i++)
        {
            eeprom_list_data.data[i].cpu_managed_val = vacuum_pump_press_index[i] & 0xffff;
            eeprom_list_data.data[i].real_val = vacuum_pump_press_data[i];
        }
        break;

    case EM_EEPROM_LIST_VALVE_FLOWBACK:

        return;

    case EM_EEPROM_LIST_VALVE_OXYGEN:
        //init_insert_data(&oxygen_valve_control_data, 30, Oxgen_Valve_Flow_data, Oxgen_Valve_Control_data, ENUM_DATA_IS_INCREASE);
        eeprom_list_data.lenth = DEFAULT_VALVE_OXYGEN_DATA_LENTH;
        for(i = 0; i < eeprom_list_data.lenth; i++)
        {
            eeprom_list_data.data[i].cpu_managed_val = Oxgen_Valve_Flow_data[i] & 0xffff;
            eeprom_list_data.data[i].real_val = Oxgen_Valve_Control_data[i];
        }
        break;

    //case EM_EEPROM_LIST_OTHERS:
    default:
        return;
    }
}

// 给eeprom_list_data的内容添加校验值
// 会改变如下变量：
//   eeprom_list_data.sum_dat
//   eeprom_list_data.xor_dat
void eeprom_add_xordata_to_list_data(void)
{
    uint8_t xor_data;
    uint32_t sum_data;
    UN_TRANS_U32_2_BYTE u32b_data;
    UN_TRANS_U16_2_BYTE u16b_data;
    int i;

    sum_data = 0;
    sum_data = eeprom_list_data.head + eeprom_list_data.lenth;
    u16b_data.data = eeprom_list_data.lenth;

    xor_data = 0; // 这里默认 eeprom_list_data.head 的异或值为0！！
    xor_data ^= u16b_data.byte[0];
    xor_data ^= u16b_data.byte[1];

    for(i = 0; i < MAX_EEPROM_LIST_DATA_NUM; i++)
    {
        sum_data += eeprom_list_data.data[i].real_val + eeprom_list_data.data[i].cpu_managed_val;
        u32b_data.data = eeprom_list_data.data[i].real_val;
        u16b_data.data = eeprom_list_data.data[i].cpu_managed_val;

        xor_data ^= u32b_data.byte[0];
        xor_data ^= u32b_data.byte[1];
        xor_data ^= u32b_data.byte[2];
        xor_data ^= u32b_data.byte[3];

        xor_data ^= u16b_data.byte[0];
        xor_data ^= u16b_data.byte[1];
    }

    eeprom_list_data.sum_dat = sum_data;

    u32b_data.data = sum_data;
    xor_data ^= u32b_data.byte[0];
    xor_data ^= u32b_data.byte[1];
    xor_data ^= u32b_data.byte[2];
    xor_data ^= u32b_data.byte[3];
    eeprom_list_data.xor_dat = xor_data;
}

// 检查eeprom_list_data的校验值，但不改变eeprom_list_data的内容
uint8_t eeprom_check_list_data(void)
{
    uint8_t xor_data;
    uint32_t sum_data;
    UN_TRANS_U32_2_BYTE u32b_data;
    UN_TRANS_U16_2_BYTE u16b_data;
    int i;

    // head
    if(eeprom_list_data.head != EEPROM_LIST_DATA_HEAD)
        return EEPROM_OPERATION_FALSE;

    sum_data = 0;
    sum_data = eeprom_list_data.head + eeprom_list_data.lenth;
    u16b_data.data = eeprom_list_data.lenth;

    xor_data = 0;
    xor_data ^= u16b_data.byte[0];
    xor_data ^= u16b_data.byte[1];

    // count xor_data locate
    for(i = 0; i < MAX_EEPROM_LIST_DATA_NUM; i++)
    {
        sum_data += eeprom_list_data.data[i].real_val + eeprom_list_data.data[i].cpu_managed_val;
        u32b_data.data = eeprom_list_data.data[i].real_val;
        u16b_data.data = eeprom_list_data.data[i].cpu_managed_val;

        xor_data ^= u32b_data.byte[0];
        xor_data ^= u32b_data.byte[1];
        xor_data ^= u32b_data.byte[2];
        xor_data ^= u32b_data.byte[3];

        xor_data ^= u16b_data.byte[0];
        xor_data ^= u16b_data.byte[1];
    }

    if(eeprom_list_data.sum_dat != sum_data)
        return EEPROM_OPERATION_FALSE;

    u32b_data.data = eeprom_list_data.sum_dat;
    xor_data ^= u32b_data.byte[0];
    xor_data ^= u32b_data.byte[1];
    xor_data ^= u32b_data.byte[2];
    xor_data ^= u32b_data.byte[3];

    xor_data ^= eeprom_list_data.xor_dat;

    if(xor_data)
        return EEPROM_OPERATION_FALSE;
    else
        return EEPROM_OPERATION_SUCCEED;
}


uint8_t eeprom_load_list_data(uint8_t data_type)
{
    uint8_t *byte_ptr;
    byte_ptr = (uint8_t *) &eeprom_list_data;

    // load data from EEPROM
    switch(data_type)
    {
    case EM_EEPROM_LIST_FLOW_EX_LOW:
        FM25V05_READ(EEPROM_FLOW_EXHALE_LOW_ADDRESS - 1, byte_ptr, EEPROM_LIST_DATA_LENTH);
        break;

    case EM_EEPROM_LIST_FLOW_EX_HIGH:
        FM25V05_READ(EEPROM_FLOW_EXHALE_HIGH_ADDRESS - 1, byte_ptr, EEPROM_LIST_DATA_LENTH);
        break;

    case EM_EEPROM_LIST_FLOW_OXYGEN:
        FM25V05_READ(EEPROM_FLOW_OXYGEN_ADDRESS - 1, byte_ptr, EEPROM_LIST_DATA_LENTH);
        break;

    case EM_EEPROM_LIST_BLOWER_PRESS:
        FM25V05_READ(EEPROM_BLOWER_PRESS_ADDRESS - 1, byte_ptr, EEPROM_LIST_DATA_LENTH);
        break;

    case EM_EEPROM_LIST_BLOWER_SPEED:
        FM25V05_READ(EEPROM_BLOWER_SPEED_ADDRESS - 1, byte_ptr, EEPROM_LIST_DATA_LENTH);
        break;

    case EM_EEPROM_LIST_BLOWER_FLOW0:
        FM25V05_READ(EEPROM_BLOWER_FLOW0_ADDRESS - 1, byte_ptr, EEPROM_LIST_DATA_LENTH);
        break;

    case EM_EEPROM_LIST_VALVE_PEEP:
        FM25V05_READ(EEPROM_VALVE_PEEP_ADDRESS - 1, byte_ptr, EEPROM_LIST_DATA_LENTH);
        break;

    case EM_EEPROM_LIST_BLOWER_VACUUM:
        FM25V05_READ(EEPROM_BLOWER_VACUUM_ADDRESS - 1, byte_ptr, EEPROM_LIST_DATA_LENTH);
        break;

    case EM_EEPROM_LIST_VALVE_FLOWBACK:
        FM25V05_READ(EEPROM_VALVE_BACKFLOW_ADDRESS - 1, byte_ptr, EEPROM_LIST_DATA_LENTH);
        break;

    case EM_EEPROM_LIST_VALVE_OXYGEN:
        FM25V05_READ(EEPROM_VALVE_OXYGEN_ADDRESS - 1, byte_ptr, EEPROM_LIST_DATA_LENTH);
        break;

    default:
    case EM_EEPROM_LIST_OTHERS:
        return EEPROM_OPERATION_FALSE;
    }

    if(eeprom_check_list_data()) return EEPROM_OPERATION_FALSE;

    eeprom_copy_work_to_real_data(data_type);

    return EEPROM_OPERATION_SUCCEED;
}


// 调用之前需要把 eeprom_list_data 准备好
void save_eeprom_list_data(uint8_t data_type)
{
    uint8_t *byte_ptr;
    byte_ptr = (uint8_t *) &eeprom_list_data;

    // add sum/xor data
    eeprom_add_xordata_to_list_data();

    // save data to EEPROM
    switch(data_type)
    {
    case EM_EEPROM_LIST_FLOW_EX_LOW:
        FM25V05_WRITE(EEPROM_FLOW_EXHALE_LOW_ADDRESS, byte_ptr, EEPROM_LIST_DATA_LENTH);
        break;

    case EM_EEPROM_LIST_FLOW_EX_HIGH:
        FM25V05_WRITE(EEPROM_FLOW_EXHALE_HIGH_ADDRESS, byte_ptr, EEPROM_LIST_DATA_LENTH);
        break;

    case EM_EEPROM_LIST_FLOW_OXYGEN:
        FM25V05_WRITE(EEPROM_FLOW_OXYGEN_ADDRESS, byte_ptr, EEPROM_LIST_DATA_LENTH);
        break;

    case EM_EEPROM_LIST_BLOWER_PRESS:
        FM25V05_WRITE(EEPROM_BLOWER_PRESS_ADDRESS, byte_ptr, EEPROM_LIST_DATA_LENTH);
        break;

    case EM_EEPROM_LIST_BLOWER_SPEED:
        FM25V05_WRITE(EEPROM_BLOWER_SPEED_ADDRESS, byte_ptr, EEPROM_LIST_DATA_LENTH);
        break;

    case EM_EEPROM_LIST_BLOWER_FLOW0:
        FM25V05_WRITE(EEPROM_BLOWER_FLOW0_ADDRESS, byte_ptr, EEPROM_LIST_DATA_LENTH);
        break;

    case EM_EEPROM_LIST_VALVE_PEEP:
        FM25V05_WRITE(EEPROM_VALVE_PEEP_ADDRESS, byte_ptr, EEPROM_LIST_DATA_LENTH);
        break;

    case EM_EEPROM_LIST_BLOWER_VACUUM:
        FM25V05_WRITE(EEPROM_BLOWER_VACUUM_ADDRESS, byte_ptr, EEPROM_LIST_DATA_LENTH);
        break;

    case EM_EEPROM_LIST_VALVE_FLOWBACK:
        FM25V05_WRITE(EEPROM_VALVE_BACKFLOW_ADDRESS, byte_ptr, EEPROM_LIST_DATA_LENTH);
        break;

    case EM_EEPROM_LIST_VALVE_OXYGEN:
        FM25V05_WRITE(EEPROM_VALVE_OXYGEN_ADDRESS, byte_ptr, EEPROM_LIST_DATA_LENTH);
        break;

    default:
    case EM_EEPROM_LIST_OTHERS:
        return;
    }
}



void active_eeprom_list_data(uint8_t data_type)
{
    switch(data_type)
    {
    case EM_EEPROM_LIST_FLOW_EX_LOW:
        // set sensor_data pointer
        sensor_data.e_l_flow_data_len     = eeprom_flow_ex_low_data.lenth;
        sensor_data.e_l_flow_real_val_ptr = (uint32_t *) & (eeprom_flow_ex_low_data.real_val);
        sensor_data.e_l_sensor_flow_ptr   = (uint32_t *) & (eeprom_flow_ex_low_data.cpu_managed_val);

        // set adc data
        init_insert_data(&expire_low_flow_data,
                         eeprom_flow_ex_low_data.lenth,
                         eeprom_flow_ex_low_data.cpu_managed_val,
                         eeprom_flow_ex_low_data.real_val,
                         ENUM_DATA_IS_INCREASE);
        break;

    case EM_EEPROM_LIST_FLOW_EX_HIGH:
        // set sensor_data pointer
        sensor_data.e_h_flow_data_len     = eeprom_flow_ex_high_data.lenth;
        sensor_data.e_h_flow_real_val_ptr = (uint32_t *) & (eeprom_flow_ex_high_data.real_val);
        sensor_data.e_h_sensor_flow_ptr   = (uint32_t *) & (eeprom_flow_ex_high_data.cpu_managed_val);

        // set adc data
        init_insert_data(&expire_high_flow_data,
                         eeprom_flow_ex_high_data.lenth,
                         eeprom_flow_ex_high_data.cpu_managed_val,
                         eeprom_flow_ex_high_data.real_val,
                         ENUM_DATA_IS_INCREASE);
        break;

    case EM_EEPROM_LIST_FLOW_OXYGEN:
        sensor_data.O2_flow_data_len     = eeprom_flow_oxygen_data.lenth;
        sensor_data.O2_flow_real_val_ptr = (uint32_t *) & (eeprom_flow_oxygen_data.real_val);
        sensor_data.O2_sensor_flow_ptr   = (uint32_t *) & (eeprom_flow_oxygen_data.cpu_managed_val);

        // set adc data
        init_insert_data(&Oxygen_flow_data,
                         eeprom_flow_oxygen_data.lenth,
                         eeprom_flow_oxygen_data.cpu_managed_val,
                         eeprom_flow_oxygen_data.real_val,
                         ENUM_DATA_IS_INCREASE);
        break;

    case EM_EEPROM_LIST_BLOWER_PRESS:
        valve_data.blower_press_data_len            = eeprom_blower_press_data.lenth;
        valve_data.blower_press_real_val_ptr        = (uint32_t *) & (eeprom_blower_press_data.real_val);
        valve_data.blower_press_cpu_managed_val_ptr = (uint32_t *) & (eeprom_blower_press_data.cpu_managed_val);

        // set adc data
        init_insert_data(&blower_press_control_data,
                         eeprom_blower_press_data.lenth,
                         eeprom_blower_press_data.cpu_managed_val,
                         eeprom_blower_press_data.real_val,
                         ENUM_DATA_IS_INCREASE);
        break;


    case EM_EEPROM_LIST_BLOWER_SPEED:
        valve_data.blower_speed_data_len            = eeprom_blower_speed_data.lenth;
        valve_data.blower_speed_real_val_ptr        = (uint32_t *) & (eeprom_blower_speed_data.real_val);
        valve_data.blower_speed_cpu_managed_val_ptr = (uint32_t *) & (eeprom_blower_speed_data.cpu_managed_val);

        // set adc data
        init_insert_data(&blower_speed_control_data,
                         eeprom_blower_speed_data.lenth,
                         eeprom_blower_speed_data.cpu_managed_val,
                         eeprom_blower_speed_data.real_val,
                         ENUM_DATA_IS_INCREASE);
        break;

    case EM_EEPROM_LIST_BLOWER_FLOW0:
        valve_data.blower_flow_0_data_len            = eeprom_blower_flow0_data.lenth;
        valve_data.blower_flow_0_real_val_ptr        = (uint32_t *) & (eeprom_blower_flow0_data.real_val);
        valve_data.blower_flow_0_cpu_managed_val_ptr = (uint32_t *) & (eeprom_blower_flow0_data.cpu_managed_val);

        // set adc data
        init_insert_data(&blower_flow_0_control_data,
                         eeprom_blower_flow0_data.lenth,
                         eeprom_blower_flow0_data.cpu_managed_val,
                         eeprom_blower_flow0_data.real_val,
                         ENUM_DATA_IS_INCREASE);
        break;

    case EM_EEPROM_LIST_VALVE_PEEP:
        valve_data.peep_valve_data_len            = eeprom_valve_peep_data.lenth;
        valve_data.peep_valve_real_val_ptr        = (uint32_t *) & (eeprom_valve_peep_data.real_val);
        valve_data.peep_valve_cpu_managed_val_ptr = (uint32_t *) & (eeprom_valve_peep_data.cpu_managed_val);

        // set adc data
        init_insert_data(&peep_valve_control_data,
                         eeprom_valve_peep_data.lenth,
                         eeprom_valve_peep_data.cpu_managed_val,
                         eeprom_valve_peep_data.real_val,
                         ENUM_DATA_IS_DECREASE);
        break;

    case EM_EEPROM_LIST_BLOWER_VACUUM:
        valve_data.vacuum_pump_data_len            = eeprom_blower_vacuum_data.lenth;
        valve_data.vacuum_pump_real_val_ptr        = (uint32_t *) & (eeprom_blower_vacuum_data.real_val);
        valve_data.vacuum_pump_cpu_managed_val_ptr = (uint32_t *) & (eeprom_blower_vacuum_data.cpu_managed_val);

        // set adc data
        init_insert_data(&vacuum_pump_press_control_data,
                         eeprom_blower_vacuum_data.lenth,
                         eeprom_blower_vacuum_data.cpu_managed_val,
                         eeprom_blower_vacuum_data.real_val,
                         ENUM_DATA_IS_INCREASE);
        break;

    case EM_EEPROM_LIST_VALVE_FLOWBACK:
        valve_data.back_flow_valve_data_len            = eeprom_valve_backflow_data.lenth;
        valve_data.back_flow_valve_real_val_ptr        = (uint32_t *) & (eeprom_valve_backflow_data.real_val);
        valve_data.back_flow_valve_cpu_managed_val_ptr = (uint32_t *) & (eeprom_valve_backflow_data.cpu_managed_val);

        // set adc data
        init_insert_data(&back_flow_control_data,
                         eeprom_valve_backflow_data.lenth,
                         eeprom_valve_backflow_data.cpu_managed_val,
                         eeprom_valve_backflow_data.real_val,
                         ENUM_DATA_IS_INCREASE);
        break;

    case EM_EEPROM_LIST_VALVE_OXYGEN:
        valve_data.oxygen_valve_control_data_len    = eeprom_valve_oxygen_data.lenth;
        valve_data.oxygen_valve_real_val_ptr        = (uint32_t *) & (eeprom_valve_oxygen_data.real_val);
        valve_data.oxygen_valve_cpu_managed_val_ptr = (uint32_t *) & (eeprom_valve_oxygen_data.cpu_managed_val);

        // set adc data
        init_insert_data(&back_flow_control_data,
                         eeprom_valve_oxygen_data.lenth,
                         eeprom_valve_oxygen_data.cpu_managed_val,
                         eeprom_valve_oxygen_data.real_val,
                         ENUM_DATA_IS_INCREASE);
        break;

    //case EM_EEPROM_LIST_OTHERS:
    default:
        return;
    }

}

void active_default_eeprom_list_data(uint8_t data_type)
{
    switch(data_type)
    {
    case EM_EEPROM_LIST_FLOW_EX_LOW:

        //init_insert_data(&expire_low_flow_data,  18, expire_flow_sensor_data,   expire_flow_val_data, ENUM_DATA_IS_INCREASE);
        // set sensor_data pointer
        sensor_data.e_l_flow_data_len   = DEFAULT_EXHALE_LOW_FLOW_DATA_LENTH;
        sensor_data.e_l_flow_real_val_ptr = (uint32_t *)&expire_flow_val_data;
        sensor_data.e_l_sensor_flow_ptr   = (uint32_t *)&expire_flow_sensor_data;

        // set adc data
        init_insert_data(&expire_low_flow_data,
                         sensor_data.e_l_flow_data_len,
                         sensor_data.e_l_sensor_flow_ptr,
                         sensor_data.e_l_flow_real_val_ptr,
                         ENUM_DATA_IS_INCREASE);
        break;

    case EM_EEPROM_LIST_FLOW_EX_HIGH:
        //init_insert_data(&expire_high_flow_data, 37, expire_flow_high_idx_data, expire_flow_high_data, ENUM_DATA_IS_INCREASE);
        // set sensor_data pointer
        sensor_data.e_h_flow_data_len   = DEFAULT_EXHALE_HIGH_FLOW_DATA_LENTH;
        sensor_data.e_h_flow_real_val_ptr = (uint32_t *)&expire_flow_high_data;
        sensor_data.e_h_sensor_flow_ptr   = (uint32_t *)&expire_flow_high_idx_data;

        // set adc data
        init_insert_data(&expire_high_flow_data,
                         sensor_data.e_h_flow_data_len,
                         sensor_data.e_h_flow_real_val_ptr,
                         sensor_data.e_h_sensor_flow_ptr,
                         ENUM_DATA_IS_INCREASE);
        break;

    case EM_EEPROM_LIST_FLOW_OXYGEN:
        //init_insert_data(&Oxygen_flow_data, 16, Oxygen_flow_sensor_data, Oxygen_flow_val_data, ENUM_DATA_IS_INCREASE);
        sensor_data.O2_flow_data_len     = DEFAULT_OXYGEN_FLOW_DATA_LENTH;
        sensor_data.O2_flow_real_val_ptr = (uint32_t *)&Oxygen_flow_val_data;
        sensor_data.O2_sensor_flow_ptr   = (uint32_t *)&Oxygen_flow_sensor_data;

        // set adc data
        init_insert_data(&Oxygen_flow_data,
                         sensor_data.O2_flow_data_len,
                         sensor_data.O2_sensor_flow_ptr,
                         sensor_data.O2_flow_real_val_ptr,
                         ENUM_DATA_IS_INCREASE);
        break;
    case EM_EEPROM_LIST_BLOWER_PRESS:
        //init_insert_data(&blower_press_control_data, 20, PEEP_VALVE_IDX, PEEP_BLOWER_DATA, ENUM_DATA_IS_INCREASE);
        valve_data.blower_press_data_len            = DEFAULT_BLOWER_PRESS_DATA_LENTH;
        valve_data.blower_press_real_val_ptr        = (uint32_t *)&PEEP_BLOWER_DATA;
        valve_data.blower_press_cpu_managed_val_ptr = (uint32_t *)&PEEP_VALVE_IDX;

        // set adc data
        init_insert_data(&blower_press_control_data,
                         valve_data.blower_press_data_len,
                         valve_data.blower_press_cpu_managed_val_ptr,
                         valve_data.blower_press_real_val_ptr,
                         ENUM_DATA_IS_INCREASE);
        break;


    case EM_EEPROM_LIST_BLOWER_SPEED:
        //	init_insert_data(&blower_speed_control_data, 34, blower_control_idx, blower_control_data, ENUM_DATA_IS_INCREASE);
        valve_data.blower_speed_data_len            = DEFAULT_BLOWER_SPEED_DATA_LENTH;
        valve_data.blower_speed_real_val_ptr        = (uint32_t *)&blower_control_data;
        valve_data.blower_speed_cpu_managed_val_ptr = (uint32_t *)&blower_control_idx;

        // set adc data
        init_insert_data(&blower_speed_control_data,
                         valve_data.blower_speed_data_len,
                         valve_data.blower_speed_cpu_managed_val_ptr,
                         valve_data.blower_speed_real_val_ptr,
                         ENUM_DATA_IS_INCREASE);
        break;

    case EM_EEPROM_LIST_BLOWER_FLOW0:
        //	init_insert_data(&blower_flow_0_control_data, 34, blower_control_press_flow_0, blower_control_idx, ENUM_DATA_IS_INCREASE);
        valve_data.blower_flow_0_data_len            = DEFAULT_BLOWER_FLOW0_DATA_LENTH;
        valve_data.blower_flow_0_real_val_ptr        = blower_control_idx;
        valve_data.blower_flow_0_cpu_managed_val_ptr = blower_control_press_flow_0;

        // set adc data
        init_insert_data(&blower_flow_0_control_data,
                         valve_data.blower_flow_0_data_len,
                         valve_data.blower_flow_0_cpu_managed_val_ptr,
                         valve_data.blower_flow_0_real_val_ptr,
                         ENUM_DATA_IS_INCREASE);
        break;

    case EM_EEPROM_LIST_VALVE_PEEP:
        //init_insert_data(&peep_valve_control_data, 25, PEEP_VALVE_IDX, PEEP_VALVE_DATA, ENUM_DATA_IS_DECREASE);
        valve_data.peep_valve_data_len            = DEFAULT_VALVE_PEEP_DATA_LENTH;
        valve_data.peep_valve_real_val_ptr        = (uint32_t *)&PEEP_VALVE_DATA;
        valve_data.peep_valve_cpu_managed_val_ptr = (uint32_t *)&PEEP_VALVE_IDX;

        // set adc data
        init_insert_data(&peep_valve_control_data,
                         valve_data.peep_valve_data_len,
                         valve_data.peep_valve_real_val_ptr,
                         valve_data.peep_valve_cpu_managed_val_ptr,
                         ENUM_DATA_IS_DECREASE); //****
        break;

    case EM_EEPROM_LIST_BLOWER_VACUUM:
        //	init_insert_data(&vacuum_pump_press_control_data, 13, vacuum_pump_press_index, vacuum_pump_press_data, ENUM_DATA_IS_INCREASE);
        valve_data.vacuum_pump_data_len            = DEFAULT_BLOWER_VACUUM_DATA_LENTH;
        valve_data.vacuum_pump_real_val_ptr        = (uint32_t *)&vacuum_pump_press_data;
        valve_data.vacuum_pump_cpu_managed_val_ptr = (uint32_t *)&vacuum_pump_press_index;

        // set adc data
        init_insert_data(&vacuum_pump_press_control_data,
                         valve_data.vacuum_pump_data_len,
                         valve_data.vacuum_pump_cpu_managed_val_ptr,
                         valve_data.vacuum_pump_real_val_ptr,
                         ENUM_DATA_IS_INCREASE);
        break;

    case EM_EEPROM_LIST_VALVE_FLOWBACK:
        //		valve_data.back_flow_valve_data_len  = 0;
        //		valve_data.back_flow_valve_real_val_ptr = 0;
        //		valve_data.back_flow_valve_cpu_managed_val_ptr	= 0;
        //		// set adc data
        //		init_insert_data(&back_flow_control_data,
        //						  valve_data.back_flow_valve_data_len,
        //						  valve_data.back_flow_valve_cpu_managed_val_ptr,
        //						  valve_data.back_flow_valve_real_val_ptr,
        //						  ENUM_DATA_IS_INCREASE);
        break;

    case EM_EEPROM_LIST_VALVE_OXYGEN:
        //init_insert_data(&oxygen_valve_control_data, 30, Oxgen_Valve_Flow_data, Oxgen_Valve_Control_data, ENUM_DATA_IS_INCREASE);
        valve_data.oxygen_valve_control_data_len    = DEFAULT_VALVE_OXYGEN_DATA_LENTH;
        valve_data.oxygen_valve_real_val_ptr        = (uint32_t *)&Oxgen_Valve_Control_data;
        valve_data.oxygen_valve_cpu_managed_val_ptr = (uint32_t *)&Oxgen_Valve_Flow_data;

        // set adc data
        init_insert_data(&oxygen_valve_control_data,
                         valve_data.oxygen_valve_control_data_len,
                         valve_data.oxygen_valve_cpu_managed_val_ptr,
                         valve_data.oxygen_valve_real_val_ptr,
                         ENUM_DATA_IS_INCREASE);
        break;

    //case EM_EEPROM_LIST_OTHERS:
    default:
        return;
    }
}

void load_all_valve_default_data(void)
{
    int i;
    for(i = EM_EEPROM_LIST_FLOW_EX_LOW; i < EM_EEPROM_LIST_OTHERS; i++)
        active_default_eeprom_list_data(i);
}



// UI inteface
/* **********************************
   字节1-2：20.9% 的氧浓度传感器值
   字节3-4：100%的氧浓度传感器的值
 **********************************
 */
void ui_set_sensor_oxygen_data(uint8_t *bufptr)
{
    UN_TRANS_U16_2_BYTE dat_16b;
    UN_TRANS_U32_2_BYTE dat_32b;

    //
    clear_eeprom_oxygen_data();

    eeprom_oxygen_data.data.head   = EEPROM_OXYGEN_C_DATA_HEAD;
    eeprom_oxygen_data.data.status = EEPROM_STATUS_DATA_READY;

    dat_16b.byte[0] = *(bufptr + 1);
    dat_16b.byte[1] = *(bufptr + 2);
    eeprom_oxygen_data.data.base = dat_16b.data;

    dat_32b.byte[0] = *(bufptr + 3);
    dat_32b.byte[1] = *(bufptr + 4);
    dat_32b.byte[2] = *(bufptr + 5);
    dat_32b.byte[3] = 0;

    eeprom_oxygen_data.data.P_step = dat_32b.data;
    append_eeprom_oxygen_data_xor_byte();

    // save to eeprom
    FM25V05_WRITE(EEPROM_O2_CONCENTRATION_ADDRESS, (uint8_t *)eeprom_oxygen_data.bytes, ST_EEPROM_OXYGEN_DATA_LEN);

    // check & load to real work
    FM25V05_READ(EEPROM_O2_CONCENTRATION_ADDRESS - 1, (uint8_t *)eeprom_temp_buf, ST_EEPROM_OXYGEN_DATA_LEN);

    if(check_eeprom_buf((uint8_t *)eeprom_oxygen_data.bytes, (uint8_t *)eeprom_temp_buf, ST_EEPROM_OXYGEN_DATA_LEN) == EEPROM_OPERATION_SUCCEED)
    {
        reset_sensor_oxygen_ack_status();

        //
        trans_eeprom_oxygen_data_to_real();
    }
    else
    {
        set_sensor_oxygen_ack_status();
    }
    set_sensor_oxygen_set_data_ack_flag();
}

/*
   字节1：传感器位置
   字节2-3：0点位置
   字节4-6：正压时每一bit变化时的增量（1/1‘000’000）
   字节7-9：负压时每一bit变化时的增量（1/1‘000’000）
 */
void ui_set_sensor_press_data(uint8_t *bufptr)
{
    UN_TRANS_U16_2_BYTE dat_16b;
    UN_TRANS_U32_2_BYTE dat_32b;
    uint32_t addr;

    clear_eeprom_press_data();

    eeprom_press_data.data.head   = EEPROM_PRESS_DATA_HEAD;
    eeprom_press_data.data.status = EEPROM_STATUS_DATA_READY;
    switch(*(bufptr + 1))
    {
    case 0: // =0 吸气压力
        addr = EEPROM_PRESS_INHALE_ADDRESS;
        break;
    case 1: // =1 呼气压力
        addr = EEPROM_PRESS_EXHALE_ADDRESS;
        break;
    case 2: // =2 真空泵
        addr = EEPROM_PRESS_VACUUM_ADDRESS;
        break;
    default:
        // error
        // must tell UI
        break;
    }
    dat_16b.byte[0] = *(bufptr + 2);
    dat_16b.byte[1] = *(bufptr + 3);
    eeprom_press_data.data.base = dat_16b.data;

    // 24 bits
    dat_32b.byte[0] = *(bufptr + 4);
    dat_32b.byte[1] = *(bufptr + 5);
    dat_32b.byte[2] = *(bufptr + 6);
    dat_32b.byte[3] = 0;
    eeprom_press_data.data.P_step = dat_32b.data;

    // 24 bits
    dat_32b.byte[0] = *(bufptr + 7);
    dat_32b.byte[1] = *(bufptr + 8);
    dat_32b.byte[2] = *(bufptr + 9);
    dat_32b.byte[3] = 0;
    eeprom_press_data.data.N_step = dat_32b.data;


    eeprom_append_press_xor((UNION_EEPROM_PRESS_DATA *) &eeprom_press_data);

    FM25V05_WRITE(addr, (uint8_t *)eeprom_press_data.bytes, ST_EEPROM_PRESS_DATA_LEN);

    // check & load to real work
    if(eeprom_load_press_data(*(bufptr + 1)) == EEPROM_OPERATION_SUCCEED)
    {
        reset_sensor_press_ack_status();
    }
    else
    {
        set_sensor_press_ack_status();
    }

    set_sensor_press_set_data_ack_flag();
}

void ui_get_work_press_data(uint8_t type)
{

    eeprom_press_data.data.head   = EEPROM_PRESS_DATA_HEAD;
    eeprom_press_data.data.status = EEPROM_STATUS_DATA_READY;

    switch(type)
    {
    case EM_SENSOR_PRESS_INHALE:
        eeprom_press_data.data.base   = sensor_data.press_i_base;
        eeprom_press_data.data.N_step = (uint32_t)(sensor_data.press_i_N_step * EEPROM_PRESS_DATA_FACT_BASE);
        eeprom_press_data.data.P_step = (uint32_t)(sensor_data.press_i_P_step * EEPROM_PRESS_DATA_FACT_BASE);
        break;

    case EM_SENSOR_PRESS_EXHALE:
        eeprom_press_data.data.base   = sensor_data.press_e_base;
        eeprom_press_data.data.N_step = (uint32_t)(sensor_data.press_e_N_step * EEPROM_PRESS_DATA_FACT_BASE);
        eeprom_press_data.data.P_step = (uint32_t)(sensor_data.press_e_P_step * EEPROM_PRESS_DATA_FACT_BASE);
        break;

    case EM_SENSOR_PRESS_VACUUM:
        eeprom_press_data.data.base   = sensor_data.press_c_base;
        eeprom_press_data.data.N_step = (uint32_t)(sensor_data.press_c_N_step * EEPROM_PRESS_DATA_FACT_BASE);
        eeprom_press_data.data.P_step = (uint32_t)(sensor_data.press_c_P_step * EEPROM_PRESS_DATA_FACT_BASE);
        break;

    default:
        break;
    }
}

// 要将flow校准数据打包
unsigned char *package_eeprom_list_data(uint8_t list_type)
{
    UN_TRANS_U16_2_BYTE dat_16b;
    UN_TRANS_U32_2_BYTE dat_32b;
    int i;

    // first
    clear_eeprom_temp_buf();

    // second
    switch(list_type)
    {
    case EM_LIST_DATA_FLOW_EXHALE_LOW_DATA:
        break;

    case EM_LIST_DATA_FLOW_EXHALE_HIGH_DATA:
        break;

    case EM_LIST_DATA_FLOW_OXYGEN_DATA:
        break;

    case EM_LIST_DATA_BLOWER_PRESS_DATA:
        break;

    case EM_LIST_DATA_BLOWER_SPEED_DATA:
        break;

    case EM_LIST_DATA_BLOWER_FLOW0_DATA:
        break;

    case EM_LIST_DATA_BLOWER_VACUUM_DATA:
        break;

    case EM_LIST_DATA_VALVE_PEEP_DATA:
        break;

    case EM_LIST_DATA_VALVE_OXYGEN_DATA:
        break;

    case EM_LIST_DATA_VALVE_BACKFLOW_DATA:
        break;

    default:
        return null;
    }

    set_eeprom_temp_buf_lenth(0);
    return (unsigned char *) &eeprom_temp_buf;
}

// 当数据错误时，返回空指针，需要判断和处理
unsigned char *ui_get_work_flow_data(uint8_t flow_type)
{
    if(flow_type > EM_LIST_DATA_FLOW_OXYGEN_DATA)
        return null;
    else
        return package_eeprom_list_data(flow_type);
}

// 这里要将标校存储在铁电RAM里的数据强行清除，并设置成缺省数据
// 先设置成缺省数据，然后才能执行
void format_eeprom_data(void)
{
    uint8_t result;
    int i;
    //
    load_default_calibration_data();

    //save list data
    for(i = EM_EEPROM_LIST_FLOW_EX_LOW; i < EM_EEPROM_LIST_OTHERS; i++)
    {
        // prepare eeprom_list_data with default list data
        copy_default_data_to_eeprom_list_data(i);

        save_eeprom_list_data(i);
    }
    // save press data
    result = eeprom_save_press_data(EM_SENSOR_PRESS_INHALE);
    if(result)
    {
        /*Do nothing now*/
    }
    result = eeprom_save_press_data(EM_SENSOR_PRESS_EXHALE);
    if(result)
    {
        /*Do nothing now*/
    }
    result = eeprom_save_press_data(EM_SENSOR_PRESS_VACUUM);
    if(result)
    {
        /*Do nothing now*/
    }

    // save oxygen data
    trans_eeprom_oxygen_data_from_real();
    append_eeprom_oxygen_data_xor_byte();
    result = eeprom_save_oxygen_concentration_data();
    if(result)
    {
        /*Do nothing now*/
    }
}

//////////////////////////////////////////////////////////////////////
// File End  /////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

