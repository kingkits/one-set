// File: MovingAverage.c
#include "MovingAverage.h"
void init_moving_average_data_u32(
    ST_MOVING_AVERAGE_DATA_U32 *base,
    unsigned char               len,
    unsigned int               *buf_ptr)
{
    base->len = len;
    if(len <= 4) base->average_data = 2;
    else base->average_data = len - 2;

    base->data = buf_ptr;
    base->data_ptr = 0;
}

// for
void moving_average_data_append_u32(ST_MOVING_AVERAGE_DATA_U32 *base, unsigned int dat)
{
    *(base->data + base->data_ptr) = dat;
    (base->data_ptr) ++;
    if(base->data_ptr >= base->len) base->data_ptr = 0;
}

unsigned int get_moving_average_data_u32(ST_MOVING_AVERAGE_DATA_U32 *base)
{
    int i;
    unsigned int dat_sum, dat_max, dat_min, data;
    dat_sum = dat_max = dat_min = *(base->data);
    for(i = 1; i < base->len; i++)
    {
        data = *(base->data + i);
        dat_sum += data;
        if(dat_max < data)dat_max = data;
        if(dat_min > data)dat_min = data;
    }
    dat_sum -= dat_max; // 不需要判断
    dat_sum -= dat_min;
    return dat_sum / base->average_data;
}


// only for base->len = 18
unsigned int fast_get_moving_average_data_u32(ST_MOVING_AVERAGE_DATA_U32 *base)
{
    int i;
    unsigned int dat_sum, dat_max, dat_min, data;
    dat_sum = dat_max = dat_min = *(base->data);
    for(i = 1; i < base->len; i++)
    {
        data = *(base->data + i);
        dat_sum += data;
        if(dat_max < data)dat_max = data;
        if(dat_min > data)dat_min = data;
    }
    dat_sum -= dat_max; // 不需要判断
    dat_sum -= dat_min;
    return dat_sum >> 4; // 16
}

// 用于浮点计算
void init_moving_average_data_f64(ST_MOVING_AVERAGE_DATA_F64 *base,
                                  unsigned char                          len,
                                  double                                *buf_ptr)
{
    base->len = len;
    if(len < 4) base->average_data = 2.0;
    else        base->average_data = (double) len - 2.0;
    base->data     = buf_ptr;
    base->data_ptr = 0;
}

// for
void moving_average_data_append_f64(ST_MOVING_AVERAGE_DATA_F64 *base, double dat)
{
    *(base->data + base->data_ptr) = dat;
    (base->data_ptr) ++;
    if(base->data_ptr >= base->len) base->data_ptr = 0;
}
double get_moving_average_data_f64(ST_MOVING_AVERAGE_DATA_F64 *base)
{
    int i;
    double dat_sum, dat_max, dat_min, data;
    dat_sum = dat_max = dat_min = *(base->data);
    for(i = 1; i < base->len; i++)
    {
        data = *(base->data + i);
        dat_sum += data;
        if(dat_max < data)dat_max = data;
        if(dat_min > data)dat_min = data;
    }
    dat_sum -= dat_max; // 不需要判断
    dat_sum -= dat_min;

    return dat_sum / base->average_data;
}

/* *********************************
 * for sum average
 * *********************************/
void reset_sum_average(ST_SUM_AVERAGE_DATA_DEFINES *base)
{
    base->count = 0;
    base->sum = 0;
}
void add_sum_average_data(ST_SUM_AVERAGE_DATA_DEFINES *base, unsigned int data)
{
    (base->count) ++;
    base->sum += data;
}
unsigned int get_sum_average_data(ST_SUM_AVERAGE_DATA_DEFINES *base)
{
    if(base->count == 0)return 0;
    return (base->sum) / (base->count);
}


void init_moving_average_data_s32(
    ST_MOVING_AVERAGE_DATA_S32 *base,
    unsigned char                     len,
    int                   *buf_ptr)
{
    base->len = len;
    if(len <= 4) base->average_data = 2;
    else base->average_data = len - 2;

    base->data = buf_ptr;
    base->data_ptr = 0;
}

// for
void moving_average_data_append_s32(ST_MOVING_AVERAGE_DATA_S32 *base, int dat)
{
    *(base->data + base->data_ptr) = dat;
    (base->data_ptr) ++;
    if(base->data_ptr >= base->len) base->data_ptr = 0;
}

int get_moving_average_data_s32(ST_MOVING_AVERAGE_DATA_S32 *base)
{
    int i;
    int dat_sum, dat_max, dat_min, data;
    dat_sum = dat_max = dat_min = *(base->data);
    for(i = 1; i < base->len; i++)
    {
        data = *(base->data + i);
        dat_sum += data;
        if(dat_max < data)dat_max = data;
        if(dat_min > data)dat_min = data;
    }
    dat_sum -= dat_max; // 不需要判断
    dat_sum -= dat_min;
    return dat_sum / base->average_data;
}

unsigned char is_Move_Average_big_than_gate(ST_MOVING_AVERAGE_DATA_S32 *ma_ptr, int gate_val, unsigned char gate_num)
{
    int i;
    int count;
    count = 0;
    i = ma_ptr->data_ptr;
    if(i == 0) i = ma_ptr->len - 1;
    else i--;
    while(i != ma_ptr->data_ptr)
    {
        if((ma_ptr->data)[i] > gate_val)
        {
            count++;
            if(count >= gate_num) return 1;
        }
        if(i == 0) i = ma_ptr->len - 1;
        else i--;
    }
    return 0;
}

unsigned char is_Move_Average_small_than_gate(ST_MOVING_AVERAGE_DATA_S32 *ma_ptr, int gate_val, unsigned char gate_num)
{
    int i;
    int count;
    count = 0;
    i = ma_ptr->data_ptr;
    if(i == 0) i = ma_ptr->len - 1;
    else i--;
    while(i != ma_ptr->data_ptr)
    {
        if((ma_ptr->data)[i] < gate_val)
        {
            count++;
            if(count >= gate_num) return 1;
        }
        if(i == 0) i = ma_ptr->len - 1;
        else i--;
    }
    return 0;
}


