// File:Insert_data.c
/*
    插值法求数据，本函数在求数据时不进行数据有效性检测，
    所有的数据都应该是正数，且时有序的序列，如果未排序，则视为无效数组，
    所有的插值都返回0
    源数据 必须是有序上升的
    目标数据需要指定是否时上升的（data_type）
 */
#define FALSE 0
#define TRUE  1
typedef enum
{
    ENUM_DATA_IS_INCREASE = 0,
    ENUM_DATA_IS_DECREASE = 1
} EM_DATA_LOGIC;

typedef struct __ST_INSERT_DATA
{
    unsigned char data_status;
    EM_DATA_LOGIC data_type;
    unsigned char   data_len;
    unsigned int *src_ptr;
    unsigned int *tar_inc_data_ptr;
    unsigned int *tar_dec_data_ptr;
    unsigned int  prev_data;
} ST_INSERT_DATA;


/*
    ST_INSERT_DATA *data_ptr  用于插值计算的数据结构
    unsigned char len,   数据长度，必须大于2
    unsigned int * data_src, 源数据 必须是有序上升的
    unsigned int * data_tar, 目标数据需要指定是否时上升的（data_type）
    EM_DATA_LOGIC data_type)
*/
unsigned char init_insert_data(ST_INSERT_DATA *data_ptr,
                               unsigned char         len,
                               unsigned int       *data_idx,
                               unsigned int       *data_tar,
                               EM_DATA_LOGIC   data_type);

// 本函数不处理负数！！,只处理上升序列的插值算法
// 本函数不做超出范围的插值处理，如果超出范围，按最大/最小的值处理
unsigned int get_insert_inc_data(unsigned int data_insert,         ST_INSERT_DATA *data_ptr);


// 本函数不处理负数！！,只处理下降序列的插值算法
// 本函数不做超出范围的插值处理，如果超出范围，按最大/最小的值处理
unsigned int get_insert_dec_data(unsigned int data_insert,         ST_INSERT_DATA *data_ptr);


// File: code ends


