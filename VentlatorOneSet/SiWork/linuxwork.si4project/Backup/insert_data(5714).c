// File:Insert_data.h
/*
    插值法求数据，本函数在求数据时不进行数据有效性检测，
    所有的数据都应该是正数，且时有序的序列，如果未排序，则视为无效数组，
    所有的插值都返回0
    源数据 必须是有序上升的
    目标数据需要指定是否时上升的（data_type）
 */
#include "insert_data.h"

/*
    ST_INSERT_DATA *data_ptr  用于插值计算的数据结构
    unsigned char len,   数据长度，必须大于2
    unsigned int * data_src, 源数据 必须是有序上升的
    unsigned int * data_tar, 目标数据需要指定是否时上升的（data_type）
    EM_DATA_LOGIC data_type)
*/
unsigned char init_insert_data(ST_INSERT_DATA *data_ptr,
                         unsigned char          len,
                         unsigned int        *data_idx,
                         unsigned int        *data_tar,
                         EM_DATA_LOGIC    data_type)
{
    unsigned int *current;
    unsigned char i;
    // first set data structure false
    data_ptr->data_status      = FALSE;
    // set ptr
    data_ptr->data_len         = len;
    data_ptr->src_ptr          = data_idx;
    data_ptr->tar_inc_data_ptr = data_tar;
    data_ptr->tar_dec_data_ptr = data_tar;
    data_ptr->data_type        = data_type;

    if(len <= 2) return FALSE; //error;
    // check logic
    current = data_idx;
    for(i = 0; i < len - 1; i++)
    {
        if(*current > *(current + 1)) return FALSE;
        current ++;
    }
    // 用于控制环操作
    data_ptr->prev_data = *data_idx;

    current             = data_tar;
    if(data_type == ENUM_DATA_IS_INCREASE)
    {
        for(i = 0; i < len - 1; i++)
        {
            if(*current > *(current + 1)) return FALSE;
            current ++;
        }
    }
    else
    {
        for(i = 0; i < len - 1; i++)
        {
            if(*current < * (current + 1)) return FALSE;
            current ++;
        }
    }
    data_ptr->data_status = TRUE;
    return TRUE;
}


// 本函数不处理负数！！,只处理上升序列的插值算法
// 本函数不做超出范围的插值处理，如果超出范围，按最大/最小的值处理
unsigned int get_insert_inc_data(unsigned int data_insert,         ST_INSERT_DATA *data_ptr)
{
    unsigned short int top, bot, curr;
    double val1, val2, val3;
    top = 0;
    bot = data_ptr->data_len - 1;
    curr = bot >> 1;
    // 判断是否超出范围
    if(data_insert < * (data_ptr->src_ptr + top))      return  *(data_ptr->tar_inc_data_ptr);
    if(data_insert > *(data_ptr->src_ptr + bot))       return  *(data_ptr->tar_inc_data_ptr + data_ptr->data_len - 1);

    // 二分法 定位
    while(curr > top)
    {
        if(data_insert == *(data_ptr->src_ptr + curr)) return *(data_ptr->tar_inc_data_ptr + curr);
        if(data_insert > *(data_ptr->src_ptr + curr))
        {
            top = curr;
        }
        else
        {
            bot = curr;
        }
        curr = top + bot;
        curr >>= 1;
    }

    // count
    val1  = (double)(data_insert - * (data_ptr->src_ptr + curr));
    val2  = (double)(*(data_ptr->src_ptr + curr + 1) - * (data_ptr->src_ptr + curr));
    val3  = val1 / val2;

    val1  = (double)(*(data_ptr->tar_inc_data_ptr + curr + 1) - * (data_ptr->tar_inc_data_ptr + curr));
    val2  = (double)(*(data_ptr->tar_inc_data_ptr + curr));
    val3 *= val1;
    val3 += val2;

    return (unsigned int) val3;
}

// 本函数不处理负数！！,只处理下降序列的插值算法(本函数仅指索引系列位下降的)，数据还是上升的
// 本函数不做超出范围的插值处理，如果超出范围，按最大/最小的值处理
unsigned int get_insert_dec_data(unsigned int data_insert,         ST_INSERT_DATA *data_ptr)
{
    unsigned short int top, bot, curr;
    double val1, val2, val3;
    top = 0;
    bot = data_ptr->data_len - 1;
    curr = bot >> 1;
    // 判断是否超出范围
    if(data_insert > *(data_ptr->src_ptr + top))       return  *(data_ptr->tar_inc_data_ptr);
    if(data_insert < * (data_ptr->src_ptr + bot))      return  *(data_ptr->tar_inc_data_ptr + data_ptr->data_len - 1);

    // 二分法 定位
    while(curr > top)
    {
        if(data_insert == *(data_ptr->src_ptr + curr)) return *(data_ptr->tar_inc_data_ptr + curr);
        if(data_insert < * (data_ptr->src_ptr + curr))
        {
            top = curr;
        }
        else
        {
            bot = curr;
        }
        curr   = top + bot;
        curr >>= 1;
    }

    // count
    val1  = (double)(*(data_ptr->src_ptr + curr) - data_insert);
    val2  = (double)(*(data_ptr->src_ptr + curr) - * (data_ptr->src_ptr + curr + 1));
    val3  = val1 / val2;

    val1  = (double)(*(data_ptr->tar_inc_data_ptr + curr + 1) - * (data_ptr->tar_inc_data_ptr + curr));
    val2  = (double)(*(data_ptr->tar_inc_data_ptr + curr));
    val3 *= val1;
    val3 += val2;

    return (unsigned int) val3;
}


// init_insert_control_data()用于具有滞环特性的控制数据的插值
/*
    ST_INSERT_DATA *data_ptr  用于插值计算的数据结构
    unsigned char len,   数据长度，必须大于2
    unsigned int * data_idx, 源数据 必须是有序上升的
    unsigned int * data_inc_tar, 目标数据需要指定是上升过程中的调整曲线，数据必须是单调上升的
    unsigned int * data_dec_tar, 目标数据需要指定是下降过程中的调整曲线，数据必须是单调上升的
*/
unsigned char init_insert_control_data(
    ST_INSERT_DATA *data_ptr,
    unsigned char len,
    unsigned int *data_idx,
    unsigned int *data_inc_tar,
    unsigned int *data_dec_tar)
{
    unsigned int *current;
    unsigned char i;
    // first set data structure false
    data_ptr->data_status = FALSE;
    // set ptr
    data_ptr->data_len         = len;
    data_ptr->src_ptr          = data_idx;
    data_ptr->tar_inc_data_ptr = data_inc_tar;
    data_ptr->tar_dec_data_ptr = data_dec_tar;
    data_ptr->prev_data        = *data_inc_tar;

    if(len <= 2) return FALSE; //error;
    // check logic
    current = data_idx;
    for(i = 0; i < len - 1; i++)
    {
        if(*current > *(current + 1)) return FALSE;
        current ++;
    }
    // 用于控制环操作
    data_ptr->prev_data = *data_idx;

    current = data_inc_tar;
    for(i = 0; i < len - 1; i++)
    {
        if(*current > *(current + 1))  return FALSE;
        current ++;
    }
    current = data_dec_tar;
    for(i = 0; i < len - 1; i++)
    {
        if(*current < * (current + 1)) return FALSE;
        current ++;
    }
    data_ptr->data_status = TRUE;
    return TRUE;
}


// 本函数不处理负数！！,只处理索引数据为上升序列的插值算法
// 本函数不做超出范围的插值处理，如果超出范围，按最大/最小的值处理
unsigned int get_insert_inc_control_data(unsigned int data_insert,         ST_INSERT_DATA *data_ptr)
{
    unsigned short int top, bot, curr;
    double val1, val2, val3;
    unsigned int *target_ptr;
    if(data_ptr->prev_data <= data_insert)
    {
        // 处于上升路径
        target_ptr = data_ptr->tar_inc_data_ptr;
    }
    else
    {
        // 处于下降路径
        target_ptr = data_ptr->tar_dec_data_ptr;
    }
    top = 0;
    bot = data_ptr->data_len - 1;
    curr = bot >> 1;
    // 判断是否超出范围
    if(data_insert < * (data_ptr->src_ptr + top))      return  *(target_ptr);
    if(data_insert > *(data_ptr->src_ptr + bot))       return  *(target_ptr + data_ptr->data_len - 1);

    // 二分法 定位
    while(curr > top)
    {
        if(data_insert == *(data_ptr->src_ptr + curr)) return *(target_ptr + curr);
        if(data_insert > *(data_ptr->src_ptr + curr))
        {
            top = curr;
        }
        else
        {
            bot = curr;
        }
        curr = top + bot;
        curr >>= 1;
    }

    // count
    val1 = (double)(data_insert - * (data_ptr->src_ptr + curr));
    val2 = (double)(*(data_ptr->src_ptr + curr + 1) - * (data_ptr->src_ptr + curr));
    val3 = val1 / val2;

    val1 = (double)(*(target_ptr + curr + 1) - * (target_ptr + curr));
    val2 = (double)(*(target_ptr + curr));
    val3 *= val1;
    val3 += val2;

    return (unsigned int) val3;
}

// File: code ends

