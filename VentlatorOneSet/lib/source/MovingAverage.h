//File:MovingAverage.h
//#include "stm32f4xx_hal.h"
//#include "arm_math.h"
// 这个结构用于移动平均窗的算法 32位整型数据
typedef struct __ST_MOVING_AVERAGE_DATA_U32
{
    unsigned char len; // 平均窗的缓存大小，必须大于4
    unsigned char average_data;
    unsigned int *data;  // 指向数据缓存的指针
    unsigned char data_ptr;
} ST_MOVING_AVERAGE_DATA_U32;
typedef struct __ST_MOVING_AVERAGE_DATA_S32
{
    unsigned char len; // 平均窗的缓存大小，必须大于4
    unsigned char average_data;
    int *data;  // 指向数据缓存的指针
    unsigned char data_ptr;
} ST_MOVING_AVERAGE_DATA_S32;

typedef struct __ST_MOVING_AVERAGE_DATA_F64
{
    unsigned char len; // 平均窗的缓存大小，必须大于4
    unsigned char average_data;
    double *data;  // 指向数据缓存的指针
    unsigned char data_ptr;
} ST_MOVING_AVERAGE_DATA_F64;

void init_moving_average_data_s32(    ST_MOVING_AVERAGE_DATA_S32 *base,    unsigned char    len, int *buf_ptr);
void moving_average_data_append_s32(ST_MOVING_AVERAGE_DATA_S32 *base, int dat);
int get_moving_average_data_s32(ST_MOVING_AVERAGE_DATA_S32 *base);
unsigned char is_Move_Average_big_than_gate(ST_MOVING_AVERAGE_DATA_S32 *ma_ptr, int gate_val, unsigned char gate_num);
unsigned char is_Move_Average_small_than_gate(ST_MOVING_AVERAGE_DATA_S32 *ma_ptr, int gate_val, unsigned char gate_num);


// 用于32bits 整型
void init_moving_average_data_u32(ST_MOVING_AVERAGE_DATA_U32 *base, unsigned char len, unsigned int *buf_ptr);
void moving_average_data_append_u32(ST_MOVING_AVERAGE_DATA_U32 *base, unsigned int dat);
unsigned int get_moving_average_data_u32(ST_MOVING_AVERAGE_DATA_U32 *base);
// only for base->len = 18
unsigned int fast_get_moving_average_data_u32(ST_MOVING_AVERAGE_DATA_U32 *base);

// 用于浮点计算
void init_moving_average_data_f64(ST_MOVING_AVERAGE_DATA_F64 *base,         unsigned char len, double *buf_ptr);
void moving_average_data_append_f64(ST_MOVING_AVERAGE_DATA_F64 *base, double dat);
double get_moving_average_data_f64(ST_MOVING_AVERAGE_DATA_F64 *base);
/* *********************************
 * for sum average
 * *********************************/
typedef struct __ST_SUM_AVERAGE_DATA
{
    unsigned int count;
    unsigned int sum;
} ST_SUM_AVERAGE_DATA_DEFINES;

void reset_sum_average(ST_SUM_AVERAGE_DATA_DEFINES *base);
void add_sum_average_data(ST_SUM_AVERAGE_DATA_DEFINES *base, unsigned int data);
unsigned int get_sum_average_data(ST_SUM_AVERAGE_DATA_DEFINES *base);

