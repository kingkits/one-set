// File: Display_data_count.h
// 显示参数要定义在这里
#include "stm32f4xx_hal.h"
#include "arm_math.h"
#ifndef __DISPLY_DATA_COUNT_H
#define __DISPLY_DATA_COUNT_H
#define is_over_breath_time(tm_gate) (system_direct_compare_tm1_be_tm2(ms_1_count,display_temp_data.T_stamp+tm_gate))

#define MA_APPEND_DISPLAY_FLOW(n) moving_average_data_append_s32(&MA_display_flow, n)
#define MA_APPEND_DISPLAY_PRESS(n) moving_average_data_append_s32(&MA_display_press, n)
typedef enum
{
    EM_TRIGGER_INHALE_INACTIVE_BREATH = 0,
    EM_TRIGGER_INHALE_ACTIVE_BREATH   = 1,
    EM_TRIGGER_EXHALE_IN_BREATH       = 2
} EM_PATIENT_ACTIVE_BREATH_TYPES;

/* ****************************************************************
	功能	1      2     3         4	   5       6        7
A	泄漏状态
    A1-极好 A2-较好 A3-好         A4-正常    A5-偏差 A6-差	   A7-极差
	<5L/min <10Lmin <25L/min <60Lmin <80Lmin <120Lmin >120Lmin
B	工作压力
	B1-极低    B2-偏低    B3-低    B4-正常    B5-偏高	B6-高	B7-极高
	<6cmH2O <8cmH2O <10cmH2O <15cmH2O <20cmH2O <25cmH2O >25cmH2O
 * ****************************************************************	
*/
typedef enum
{
    EM_BREATH_LEAK_PERFACT, // 泄漏 <5 L/min
    EM_BREATH_LEAK_BETTER,  // 泄漏 <10 L/min    
    EM_BREATH_LEAK_GOOD,    // 泄漏 <25 L/min
    EM_BREATH_LEAK_NORMAL,  // 泄漏 <60 L/min
    EM_BREATH_LEAK_BAD,     // 泄漏 <80 L/min
    EM_BREATH_LEAK_WORSE,   // 泄漏 <120 L/min
    EM_BREATH_LEAK_WORST,   // 泄漏   >120 L/min
    EM_BREATH_LEAK_PIP_OUT  // 
} ENUM_BREATH_LEAK_LEVEL_TYPES;
extern uint8_t system_breath_press_level;

typedef enum
{
	EM_BREATH_PRESS_LEVEL_LOWEST, // B1-极低 <6cmH2O
	EM_BREATH_PRESS_LEVEL_LOWER,  // B2-偏低 <8cmH2O 
	EM_BREATH_PRESS_LEVEL_LOW,	  // B3-低    <10cmH2O 
	EM_BREATH_PRESS_LEVEL_NORMAL, // B4-正常	 <15cmH2O 
	EM_BREATH_PRESS_LEVEL_HIGH,   // B5-偏高 <20cmH2O 
	EM_BREATH_PRESS_LEVEL_HIGHER, // B6-高    <25cmH2O 
	EM_BREATH_PRESS_LEVEL_HIGHEST // B7-极高 >25cmH2O
} EM_PRESS_LEVEL_STATUS_TYPES;

// 上一呼吸周期的泄漏水平
// 泄漏水平会影响PEEP控制，排痰，叩击，通气参数补偿和触发控制方案
// 需要在后期进行完善
extern uint8_t system_breath_leak_level;


typedef enum
{
    EM_FREQ_MV_OPERATION_RESET,
    EM_FREQ_MV_OPERATION_NORMAL,
    EM_FREQ_MV_OPERATION_FORCE
} ENUM_FREQ_MV_OPERATION_STATUS;

typedef struct __ST_LEAK_COUNT_DATA
{
    float _3s_val;             		// 每   3秒周期的平均泄漏值
    float _3s_data_ins;        		// 累计数
    float _3s_data_exp;        		// 累计数
    uint32_t _3s_count;        		// 每   3秒周期的计数

    float _10s_val;            		// 每  10秒周期的平均泄漏值
    float _10s_data_ins;       		// 累加数
    float _10s_data_exp;       		// 累加数
    uint32_t _10s_count;       		// 每  10秒周期的计数

    float _30s_val;        	   		// 每  30秒周期的平均泄漏值
    float _30s_data_ins;       		// 累加数
    float _30s_data_exp;       		// 累加数
    uint32_t _30s_count;       		// 每  30秒周期的计数

    float average_peep_valve_flow;	// 计算的平均呼气阀流量

    int32_t unknown_leakage_30s;

    uint8_t active_flag;
} ST_LEAK_COUNT_DATA;
extern ST_LEAK_COUNT_DATA leak_count_data;

typedef struct __ST_DISPLAY_DATA
{
    // PFV data for display
    int16_t Press; // 当前口端压力
    int32_t Flow;  // 当前口端流速
    int16_t volume;// 当前计算的潮气量

    // 呼吸周期/频率
    uint16_t freq; //(0-200) 计算的呼吸频率

    // 吸气时间 Ti
    uint32_t Ti;
    // 总呼吸时间 Ti+Te
    uint32_t Ttotal;

    // 通气时间
    uint32_t ventilation_work_seconds; // 从模式开始到现在的工作时间 0 - ?
    // 上一周期的呼吸数据
    // RC数据
    uint16_t R; // for patient lung; 0-2000 (unit 0.1 cmH2O/(L/s))
    uint16_t C; // 0-1000 (0.1 ml/cmH2O)
    // 潮气量
    uint16_t vt; // 1 - 3000ml

    uint16_t vt_sec[4];

    // 分钟通气量
    uint32_t MV; // 0-100000mL/Min (100L/Min)

    // 峰值压力
    int16_t P_peak;    // 0-1000 (0.1cmH2O)
    int16_t P_lowest;  // 1000-> -1000 还要考虑 峰值低压

    // 平均压力
    int16_t P_mean; // 0- 1000 (0.1cmH2O)
    int16_t P_plat; // 0- 1000 (0.1cmH2O)

    // PEEP
    uint16_t P_peep; // 0- 1000 (0.1cmH2O)
    // 总呼吸周期计数 0 - 100
    uint8_t total_breath_cycles;

    // 总触发周期计数 0 - 100
    uint8_t total_tirgger_cycles;

    // 流量/泄漏
    uint32_t flow_leak;    // 每个呼吸周期的平均泄露值

    // 缺省泄漏计算
    uint32_t default_flow_leak;
    uint8_t  default_flow_leak_refresh_flag;

    //峰值流速  // 用于肺功能监测
    int32_t inhale_peak_flow;
    int32_t exhale_peak_flow;

    // 当前周期的触发标记 =0 无触发，=1 有触发
    uint8_t trigger_flag;
} ST_DISPLAY_DATA_DEFINES;
extern ST_DISPLAY_DATA_DEFINES display_count_data;


#define MAX_BREATH_COUNT 8  // 必须为8，如果更改，（因为有简化的除法算法>>3）需要遍历所有的标注有此标号的位置
typedef struct __ST_DISPLAY_TEMP_DATA
{
    uint8_t last_work_status;
    /* ****************************************
     * 最近x（x = MAX_BREATH_COUNT）个周期的数据
     * ****************************************/
    // 呼吸数据的指针
    uint8_t current_data_count;

    // 最近x个周期的潮气量
    uint32_t vt_rec[MAX_BREATH_COUNT];

    uint32_t vt_sec[4];
    uint32_t vt_second_cnt;
    uint8_t vt_second_active_flag;

    // x个周期的吸呼气时间
    uint32_t inhale_time[MAX_BREATH_COUNT];
    uint32_t exhale_time[MAX_BREATH_COUNT];

    // x个周期的呼吸周期（需要有x个呼吸周期的记录）
    uint32_t breath_time[MAX_BREATH_COUNT];

    // x个周期开始的时间戳
    uint32_t breath_time_stamp[MAX_BREATH_COUNT];
    /* ********************************************
    	当前呼吸周期的数据
     * ********************************************/
    //
    // 通气时间
    //uint32_t Ti, Te; // 当前周期的吸呼气计时
    uint32_t T_stamp;

    // RC数据单独有定义

    // 压力
    uint16_t P_peak;  // 峰值压力
    uint16_t P_lowest;  // 还要考虑 峰值低压
    uint32_t P_mean;
    uint16_t P_meancount;
    uint8_t  P_peep_start_flag;
    uint32_t P_peep;
    uint16_t P_peepcount;

    // 容量积分
    int32_t vt_sum; //用于显示PFV 中的参数V
    uint32_t vti; // 吸气潮气量计算临时变量
    uint32_t vte; // 呼气潮气量计算临时变量
    int32_t last_dv; // 上一时间段的潮气量计算 (有符号整型变量，  >0时为吸气   <0时为呼气)

    uint8_t vti_count_flag;
    uint8_t vte_count_flag;
    // 一周期的流量积分，用于计算泄漏
    uint32_t flow_i_sum;
    uint32_t flow_e_sum;
    uint16_t flow_count;
    /* *******************************************
     * 上一时刻的必要数据
     * *******************************************/
    // 上一时刻的时间记录，计算时长。用于计算潮气量积分等
    uint32_t last_time;//(ms)

    // 流量
    int32_t last_flow;

    // PPFFVV for test!!!!
    //int16_t inhale_Press; // 当前吸气口压力
    //int16_t exhale_Press; // 当前吸气口压力

    //int32_t inhale_Flow;  // 当前吸气口流速
    //int32_t exhale_Flow;  // 当前呼气口流速
    int32_t inhale_peak_flow;
    int32_t exhale_peak_flow;

    int16_t inhale_Volume;// 当前计算的吸气潮气量
    int16_t exhale_Volume;// 当前计算的呼气潮气量


    uint8_t trigger_inhale_flag;
    uint8_t trigger_exhale_flag;

#if USER_TEST_PPFFVV
    uint32_t v_i;
    uint32_t v_e;
#endif

    uint32_t no_breath_detected_stemp;
    uint8_t volume_count_actived; // 如果标记为1则计算容量曲线，否则，容量曲线数据归零
} ST_DISPLAY_TEMP_DATA_DEFINES;
extern ST_DISPLAY_TEMP_DATA_DEFINES display_temp_data;

#define is_T_mode_inhale_press_over() (current_counted_press > main_control_data.ipap + 10)
#define is_T_mode_inhale_press_insufficient() (current_counted_press < main_control_data.ipap - 20)

/* ******************************************
	系统复位时的初始化
 * ******************************************/
void     reset_display_data(void);
void     reset_display_temp_data(void);
void     reset_second_vt_temp(void);
void     breath_clear_second_vt(void);

/* *************************************
    计算触发比率及相关
 * *************************************/
uint16_t display_get_trigger_rate(void);// 返回值 0-100, 触发比率 (单位 1%)
uint8_t  get_breath_trigger_flag(void);
void     inc_trigger_rate(void);
void     dec_trigger_rate(void);
void     breath_set_trigger_active_breath_type(uint16_t type);
void     reset_trigger_rate_data(void);

/* **********************************************
 * 呼吸数据 总的计算入口
 * **********************************************/
void     calculate_breath_data(void);
void     breath_reset_display_temp_data(void);
/* **********************************************
 * 时间相关
 * **********************************************/
uint32_t breath_get_time_interval(void);
void     mark_breath_T_stmap(void);

/* *************************************
    计算峰值流速相关
 * *************************************/
int32_t  breath_get_inhale_peak_Flow(void);
int32_t  breath_get_exhale_peak_Flow(void);

/* **************************************
 * PEEP开始时的相关函数
 * **************************************/
void     display_reset_peep_data(void);
void     set_peep_start_flag(void);
void     reset_peep_start_flag(void);

/* **************************************
* 获取当前状态的相关函数
* **************************************/
int32_t  breath_get_current_flow(void);
int16_t  display_get_last_peep(void);
uint8_t  check_pipe_out_status(void);
uint8_t  monitor_exhale_end(void);
uint32_t breath_get_Vte(void);
uint32_t breath_get_Vti(void);
int16_t  breath_get_remaining_vt(void);
void enable_volume_count(void);
void disable_volume_count(void);

int16_t display_get_Pmean(void);

/* **************************************
 * 患者主动呼吸的相关函数
 * **************************************/
void     breath_set_trigger_flag(void);
void     breath_reset_trigger_flag(void);
void     breath_set_inhale_trigger_flag(void);
void     breath_reset_inhale_trigger_flag(void);
void     breath_set_exhale_trigger_flag(void);
void     breath_reset_exhale_trigger_flag(void);
void     display_set_peak_flow(void);
//void count_data_for_exhale_end(void);

/* **********************************************
 * 肺气阻及肺顺应性
 * **********************************************/
void     breath_reset_R_C_data(void);
void     breath_integral_R_C_DATA(void);
void     breath_calculate_R_C(void);
void     breath_cycle_completed_actions(void);
uint32_t get_display_temp_vti(void);
uint32_t get_display_temp_vte(void);

uint8_t  is_breath_T_mode_start(void);


/* **********************************************
 * 咳痰数据 计算的相关函数
 * **********************************************/
// 初始化咳痰相关的数据
void     reset_display_data_for_cough(void);

// 在负压排咳痰开始时，需要调用这个函数
void     display_start_cough_count(void);

void     calculate_cough_cycle_display_data(void);


void     display_set_last_work_status(uint8_t status);


void     breath_count_Te(void);
void     breath_count_Ti(void);

int32_t  get_display_flow(void);
int32_t  get_display_press(void);


/* **********************************************
 * 泄漏 相关函数
 * **********************************************/
void     set_system_breath_leak_level(int32_t leak);
void     display_count_leak(void);
void     correct_peep_for_leak(void);
void     reset_leak_count_data(void);
void     set_leak_flow_force(void);
void     set_leak_flow_fast(void);

void     clear_no_breath_detected_stemp(void);
void     set_no_breath_detected_stemp(void);
uint32_t get_no_breath_detected_time(void);

void set_system_breath_press_level(int16_t press);

void adjust_breath_time_stemp(uint32_t tt);
void breath_completed_Vte_count(void);

#endif //__DISPLY_DATA_COUNT_H
// ========= The end

