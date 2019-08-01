//file: leak_flow_count.c
#include "leak_flow_count.h"

//zzx: important!! do not delete these lines (#define SYSTEM_SQRT)
#define USE_SYSTEM_SQRT 0
#if USE_SYSTEM_SQRT
extern double __sqrtf(double in);
#define SYSTEM_SQRT __sqrtf
#else
#define SYSTEM_SQRT d_sqrt
#endif

//#define SYSTEM_SQRT i_sqrt


#define CORRECT_READ_WRITE_POINTER()                                   \
		{                                                              \
			(st_real_flow_press_compensation_data.write_locate) ++;    \
			st_real_flow_press_compensation_data.write_locate &= 0x3;  \
			(st_real_flow_press_compensation_data.read_locate) ++;     \
			st_real_flow_press_compensation_data.read_locate &= 0x3;   \
		}


#if USE_SYSTEM_SQRT
#else
double d_sqrt(short int data)
{
    double last = 0.0;
    double res = 1.0;
    int i;
    double x;
    if (data <= 1) return 1.0;
    x = (double) data;
    for(i = 0; i < 8; i++) // 8次 迭代精度就足够了
    {
        if (res != last)
        {
            last = res;
            res = (res + x / res) / 2.0;
        }
        else break;
    }
    return res;
}

#endif

/*
	模块名称	void flow_leak_refresh_data(short int Pmean, int Fleak_mean)
	输入
	short int Pmean         -- 上一呼吸周期的平均压力
	int Fleak_mean    -- 上一呼吸周期的平均泄露值
	输出
	说明	给定上一呼吸周期的计算参数，计算用于下一函数使用的中间数据
*/
ST_FLOW_LEAK_COMPENSATION_DATA st_flow_leak_compensation_data;
ST_REAL_FLOW_PRESS_COMPENSATOPN_DEFINES st_real_flow_press_compensation_data;

void flow_leak_refresh_data(short int p, int Fleak_mean)
{
    double dtmp;
    if(p < 1) p = 1;
#if USE_SYSTEM_SQRT
    dtmp = (double)p;
    dtmp = SYSTEM_SQRT(dtmp);
#else
    dtmp = SYSTEM_SQRT(p);
#endif
    st_flow_leak_compensation_data.flow_para_a = (double)Fleak_mean / dtmp;
}
/*
	模块名称	int flow_leak_compensation(short int P)
	输入	short int P            --- 给定当前的口端气压
	输出	返回修正过的泄漏流量
	说明	利用上个函数计算的中间值，根据当前的压力，修正泄漏流量值
*/
int flow_leak_compensation(short int p)
{
    double dtmp;
    if(p < 1) p = 1;
#if USE_SYSTEM_SQRT
    dtmp = (double)p;
    dtmp = SYSTEM_SQRT(dtmp);
#else
    dtmp = SYSTEM_SQRT(p);
#endif

    dtmp *= st_flow_leak_compensation_data.flow_para_a;
    return (int) dtmp;
}

void init_flow_leak_compensation_data (void)
{
    st_flow_leak_compensation_data.flow_para_a = 0.0;
    st_flow_leak_compensation_data.perss_para_a = 0.5;
}

void init_st_real_flow_press_compensation_data(void)
{
    int i;
    st_real_flow_press_compensation_data.ex_flow      = 0;
    st_real_flow_press_compensation_data.ex_press     = 0;
    st_real_flow_press_compensation_data.read_locate  = 2;
    st_real_flow_press_compensation_data.real_flow    = 0;
    st_real_flow_press_compensation_data.real_press   = 0;
    st_real_flow_press_compensation_data.write_locate = 0;
    st_real_flow_press_compensation_data.flow_leak    = 0;

    for(i = 0; i < MAX_PIPE_PRESS_COMPENSATION_NUM; i++)
    {
        st_real_flow_press_compensation_data.ins_flow[i]  = 0;
        st_real_flow_press_compensation_data.ins_press[i] = 0;
    }
}

void set_refresh_compensation_press_flow(short int p_ins, short int p_ex, int f_ins, int f_ex)
{
    // 得到流量和压力
    // Press
    st_real_flow_press_compensation_data.ins_press[st_real_flow_press_compensation_data.write_locate]  = p_ins;
    st_real_flow_press_compensation_data.ex_press  = p_ex;

    st_real_flow_press_compensation_data.ins_flow[st_real_flow_press_compensation_data.write_locate]  = f_ins;
    st_real_flow_press_compensation_data.ex_flow  = f_ex;
}
void display_refresh_compensation_press_flow(void)
{
    double dtemp;

    // 计算口端压力
    dtemp = (double)st_real_flow_press_compensation_data.ex_press +
            (double)st_real_flow_press_compensation_data.ins_press[st_real_flow_press_compensation_data.read_locate];
    dtemp *= st_flow_leak_compensation_data.perss_para_a;
    st_real_flow_press_compensation_data.real_press = (short int) dtemp;
    // 计算口端泄漏量
    st_real_flow_press_compensation_data.flow_leak = flow_leak_compensation(st_real_flow_press_compensation_data.real_press);

    // 计算口端流量
    st_real_flow_press_compensation_data.real_flow  = st_real_flow_press_compensation_data.ins_flow[st_real_flow_press_compensation_data.read_locate] - st_real_flow_press_compensation_data.flow_leak;
    st_real_flow_press_compensation_data.real_flow -= st_real_flow_press_compensation_data.ex_flow;

    // 修正指针
    CORRECT_READ_WRITE_POINTER();
}

short int get_real_press(void)
{
    return st_real_flow_press_compensation_data.real_press;
}

short int get_ins_press(void)
{
    return st_real_flow_press_compensation_data.ins_press[st_real_flow_press_compensation_data.read_locate];
}
short int get_ex_press(void)
{
    return st_real_flow_press_compensation_data.ex_press;
}


int get_real_flow(void)
{
    return st_real_flow_press_compensation_data.real_flow;
}

int get_ins_flow(void)
{
    return st_real_flow_press_compensation_data.ins_flow[st_real_flow_press_compensation_data.read_locate];
}

int get_ex_flow(void)
{
    return st_real_flow_press_compensation_data.ex_flow;
}


int get_flow_leak(void)
{
    return st_real_flow_press_compensation_data.flow_leak;
}

// end of file leak_flow_count.c

