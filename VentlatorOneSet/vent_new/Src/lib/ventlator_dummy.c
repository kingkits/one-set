// file: ventlator_dummy.c
#include"insert_data.h"
__weak unsigned char init_insert_data(ST_INSERT_DATA *data_ptr, unsigned char len, unsigned int *data_idx, unsigned int *data_tar, EM_DATA_LOGIC data_type)
{
    return 0;
}
__weak unsigned int  get_insert_inc_data(unsigned int data_insert, ST_INSERT_DATA *data_ptr)
{
    return 0;
}
__weak unsigned int  get_insert_dec_data(unsigned int data_insert, ST_INSERT_DATA *data_ptr)
{
    return 0;
}
__weak void          flow_leak_refresh_data(short int p, int Fleak_mean) {}
__weak int           flow_leak_compensation(short int p)
{
    return 0;
}
__weak void          init_flow_leak_compensation_data (void) {}
__weak void          init_st_real_flow_press_compensation_data(void) {}
__weak void          display_refresh_compensation_press_flow(void) {}
__weak short int     get_real_press(void)
{
    return 0;
}
__weak short int     get_ins_press(void)
{
    return 0;
}
__weak short int     get_ex_press(void)
{
    return 0;
}
__weak int           get_real_flow(void)
{
    return 0;
}
__weak int           get_ins_flow   (void)
{
    return 0;
}
__weak int           get_ex_flow(void)
{
    return 0;
}
__weak int           get_flow_leak(void)
{
    return 0;
}
__weak void          set_refresh_compensation_press_flow(short int p_ins, short int p_ex, int f_ins, int f_ex) {}

