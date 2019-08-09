// leak_flow_count.h

/*
	要求： 中间数据使用全局变量，请装在下面的数据结构中
*/
typedef  struct __ST_FLOW_LEAK_COMPENSATION_DATA
{
    // 数据定义
    double flow_para_a;
    double perss_para_a;
} ST_FLOW_LEAK_COMPENSATION_DATA;



#define MAX_PIPE_PRESS_COMPENSATION_NUM 4
typedef struct __ST_REAL_FLOW_PRESS_COMPENSATOPN_DEFINES
{
    unsigned char write_locate, read_locate;
    short int ins_press[MAX_PIPE_PRESS_COMPENSATION_NUM];
    int ins_flow[MAX_PIPE_PRESS_COMPENSATION_NUM];
    short int ex_press;
    int ex_flow;
    short int real_press;
    int real_flow;

    int flow_leak;
} ST_REAL_FLOW_PRESS_COMPENSATOPN_DEFINES;


void    flow_leak_refresh_data(short int p, int Fleak_mean);
int flow_leak_compensation(short int p);
void    init_flow_leak_compensation_data (void);
void    init_st_real_flow_press_compensation_data(void);
void    display_refresh_compensation_press_flow(void);
short int get_real_press(void);
short int get_ins_press  (void);
short int get_ex_press  (void);
int get_real_flow(void);
int get_ins_flow(void);
int get_ex_flow(void);
int get_flow_leak(void);
void set_refresh_compensation_press_flow(short int p_ins, short int p_ex, int f_ins, int f_ex);

// end of leak_flow_count.h

