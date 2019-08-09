/*-------------------------------------------------------------------------

   -------------------------------------------------------------------------*/
//File:breath_control.h
#ifndef __BREATH_CONTROL_H
#define __BREATH_CONTROL_H

#define VAL_PRESS_INCREASE_STEP_TIME 50 // 单步压力上升调整时间

typedef enum __ENUM_VENTELATOR_WORK_MODE
{
    EM_VENTLATOR_STANDBY_MODE = 0,
    EM_VENTLATOR_CPAP_MODE, // 通用普通模式
    EM_VENTLATOR_PAV_MODE, // 通用高级模式
    EM_VENTLATOR_COUGH_MODE,// 将咳痰模式独立出来，进行单独控制
    // EM_VENTLATOR_TEST_MODE, // 自检模式，还未规划
    EM_VENTLATOR_COUGH_REPEAT_MODE,
    //兼容 PHILIPS 伟康 V60
    EM_VENTLATOR_BIPAP_ST_MODE = 0x10,
    EM_VENTLATOR_BIPAP_PCV_MODE,
    // 伟康高级模式，不可用

    //兼容五合一的模式 VOCSN
    EM_VENTLATOR_BiLEVEL_MODE = 0xA0,
    EM_VENTLATOR_SPONTANEOUS_MODE,
    EM_VENTLATOR_AC_P_MODE,
    EM_VENTLATOR_SIMV_PCV_MODE,
    EM_VENTLATOR_AC_V_MODE,
    EM_VENTLATOR_SIMV_VCV_MODE
} ENUM_VENTELATOR_WORK_MODE_DEFINES;

/*
   switch(get_patient_breath_mode())
   {
   case EM_VENTLATOR_STANDBY_MODE:
   break;

   //基本模式
   case EM_VENTLATOR_CPAP_MODE:
   break;

   // 高级模式
   case EM_VENTLATOR_PAV_MODE:
   break;
   case EM_VENTLATOR_COUGH_MODE:// 将咳痰模式独立出来，进行单独控制
   break;
   //case EM_VENTLATOR_TEST_MODE: // 自检模式，还未规划
   //	break;
   //兼容 PHILIPS 伟康 V60
   case EM_VENTLATOR_BIPAP_ST_MODE:
   break;
   case EM_VENTLATOR_BIPAP_PCV_MODE:
   break;

   // 伟康高级模式，不可用

   //兼容五合一的模式 VOCSN
   case EM_VENTLATOR_BiLEVEL_MODE:
   break;
   case EM_VENTLATOR_SPONTANEOUS_MODE:
   break;
   case EM_VENTLATOR_AC_P_MODE:
   break;
   case EM_VENTLATOR_SIMV_PCV_MODE:
   break;
   case EM_VENTLATOR_AC_V_MODE:
   break;
   case EM_VENTLATOR_SIMV_VCV_MODE:
   break;
   default:
   break;
   }

 */
typedef enum _ENUM_MAIN_WORK_STATUS_TYPE_DEFINES
{
    // 需要设定 set_PID_blower_adjust_mode(ENUM_PID_FAST_FOR_PRESS);
    // 即： PID_blower_control_data.adjust_mode = ENUM_PID_FAST_FOR_PRESS
    //                            说明                         PID调整目标
    EM_BREATH_STANDBY = 0, // 未启动                        无需调整
    EM_BREATH_PRESS_HOLD_HIGH, // IPAP BiLEVEL               main_control_data.ipap
    EM_BREATH_PRESS_HOLD_LOW, // EPAP BiLEVEL               main_control_data.epap
    EM_BREATH_FLOW_SUPPORT, // 用于容量控制CMV/VCV              未确定
    EM_BREATH_PRESS_INCREASE, // 用于压力上升控制                   st_ST_mode_control_data.T_target_press
    EM_BREATH_COUGH_SUPPORT, // 用于COUGH压力调整，               cough_control_data.cough_target_press
    EM_BREATH_PCV_PRESS_INC, // 用于PCV压力上升过程                main_control_data.epap -> -> main_control_data.T_press
    EM_BREATH_PCV_PRESS_HOLD, // 用于PCV压力保持过程                main_control_data.T_press
    EM_BREATH_INHALE_SUPPORT, // 吸气时的补偿状态 辅助吸气      ，未确定
    EM_BREATH_EXHALE_SUPPORT // 呼气时的补偿状态 辅助呼气    ，main_control_data.c_flex_level
} ENUM_MAIN_WORK_STATUS_TYPE_DEFINES;
/* *****************************************************
   Usage:
   switch(get_main_control_status())
   {
     default:
       break;
     case EM_BREATH_STANDBY: // 未启动
       break;
     case EM_BREATH_PRESS_HOLD_HIGH: // IPAP BiLEVEL
       break;
     case EM_BREATH_PRESS_HOLD_LOW:  // EPAP BiLEVEL
       break;
     case EM_BREATH_FLOW_SUPPORT:	 // 用于容量控制CMV/VCV
       break;
     case EM_BREATH_PRESS_INCREASE:	 // 用于PCV压力控制
       break;
     case EM_BREATH_INHALE_SUPPORT: // 吸气时的补偿状态 Spontaneous Psupport
       break;
     case EM_BREATH_EXHALE_SUPPORT:  // 呼气时的补偿状态 Spontaneous 辅助呼气
       break;
   }

 * *****************************************************
 */


typedef struct _ST_MAIN_CONTROL_DATA
{
    // parameters change flages
    uint8_t change_mode_flag;
    uint8_t change_parameter_flag;
    uint8_t work_mode;

    // BASE value
    uint32_t flow;        // (ml/min)预设的流量。属于基础流量（base_flow）
    uint8_t status;       // (0.1cmH2O)工作状态
    // CPAP
    int16_t epap;        // (0.1cmH2O)预设的呼气压力

    // Bilevel/BiPAP
    int16_t ipap;        // (0.1cmH2O)预设的吸气压力


    int16_t c_flex_level; // (0.1cmH2O) 呼气时压力释放强度
    // T mode press
    int16_t T_press;     // (0.1cmH2O)机械控制模式的设置压力

    //T mode rising time;
    uint32_t T_rising_time; // (1ms) 压力上升时间

    //T mode breath inhale time T 模式的吸气时间
    uint32_t T_inhale_time; // (1ms)

    //T_mode Freq
    uint16_t T_freq;      // (BPM)

    // 触发门限选择 --- 用于启动触发检测
    // 触发模式选择
    uint8_t trigger_mode;
    // 吸气相--相关的 触发 门限
    uint16_t trigger_flow_level_i; // 1ml/min
    uint16_t trigger_press_level_i;// 0.1 x cmH2O
    // 呼气相--相关的 触发 门限
    uint16_t trigger_flow_level_e; // 1ml/min
    uint16_t trigger_press_level_e;// 0.1 x cmH2O

    // 切换吸呼气相 （呼吸支持强度）未确定！！！！！
    // 吸气相--相关的阈值
    uint8_t gate_mode_i;
    uint16_t gate_flow_limited_i; //%
    uint16_t gate_press_limited_i;//0.1 x cmH2O

    // 呼气相--相关的阈值
    uint8_t gate_mode_e;
    uint16_t gate_flow_limited_e; //%
    uint16_t gate_press_limited_e;//0.1 x cmH2O

    // 震动
    // 震动强度
    // 震动频率

    // 排痰
    // 初始 排痰压力/潮气量
    int16_t cough_prepare_press;     // 排痰准备压力（最大吸气压力）
    int16_t cough_start_press;       // 排痰压力 （呼气时最小压力） 此数值为正，与实际值相反
    uint16_t cough_start_vt;        // 排痰开始前需要准备的潮气量
    // 压力上升时间/屏气时间
    uint32_t cough_press_increase_time; // 压力上升时间
    uint32_t cough_hold_time;       // ms 咳痰最大屏气时间

    /* *****************************************************************
       检测条件及阈值定义：
       stop_mode	                 stop_value     单位(uint)
       = 0   潮气量阈值(绝对值)            0-150			x10ml
       = 1   潮气量百分比                50-100         %
       = 2   呼气流量阈值              0-100			L/min
       = 3   时间阈值                  1-20           0.1秒
    * *************************************************************** */
    uint8_t cough_finish_types;
    uint8_t cough_finish_val;

    // 结束压力？？（排出潮气量）？？待定
} ST_MAIN_CONTROL_DATA;
extern ST_MAIN_CONTROL_DATA main_control_data, ui_set_data;



/* *******************For S/T mode******************************--> */
typedef struct __st_ST_mode_control_data_DEFINES
{
    // 压力上升相关的
    uint16_t rising_time_count;    // 计数，用于计算上升时间
    uint16_t rising_time_steps;    // 每次吸气时的总的上升次数
    uint16_t blower_increase_speed; // 压力上升时的步进（每50毫秒调整一次）

    int16_t T_target_press;
    uint32_t last_time_stemp;

    // 吸气相--相关的 触发 门限/阈值
    int32_t flow_e_gate;
    // 呼气相--相关的 触发 门限/阈值
    // C-FLEX 控制相关
    uint32_t c_flex_completed_gate;

    // 时间控制相关的
    uint32_t T_next_breath_start_gate; // 由呼吸频率/周期决定，确定T触发时间(下一吸气开始时刻)
    uint32_t T_one_breath_time;    // 一个呼吸的周期
    uint32_t T_exhale_start_gate;  // 下一呼气开始时刻

    uint32_t T_exhale_3_4_gate;    // 呼气经过T呼气时间的3/4 时刻，用于检测吸气触发及PEEP计算
} st_ST_mode_control_data_DEFINES;
extern st_ST_mode_control_data_DEFINES st_ST_mode_control_data;

typedef enum __ENUM_COUGH_FINISHED_TYPES
{
    EM_COUGH_FINISH_WITH_VT_VAL       = 0,  //   潮气量阈值(绝对值)			0-150		   x10ml
    EM_COUGH_FINISH_WITH_VT_PERCENT   = 1,  //   潮气量百分比				50-100		   %
    EM_COUGH_FINISH_WITH_FLOW_LIMITED = 2,  //   呼气流量阈值				0-100		   L/min
    EM_COUGH_FINISH_WITH_TIME_LIMITED = 3,  //   时间阈值				1-20		   0.1秒
    EM_COUGH_FINISH_WITH_OTHERS
} ENUM_COUGH_FINISHED_TYPES;
/*
   switch(main_control_data.cough_finish_types)
   {
   case EM_COUGH_FINISH_WITH_VT_VAL:      // = 0   潮气量阈值(绝对值)			0-150		   x10ml
   break;
   case EM_COUGH_FINISH_WITH_VT_PERCENT:  // = 1   潮气量百分比				50-100		   %
   break;
   case EM_COUGH_FINISH_WITH_FLOW_LIMITED:// = 2   呼气流量阈值				0-100		   L/min
   break;
   case EM_COUGH_FINISH_WITH_TIME_LIMITED:// = 3   时间阈值         1-20		   0.1秒
   break;
   case EM_COUGH_FINISH_WITH_OTHERS:
   break;
   default:
   break;
   }
 */
typedef struct __ST_COUGH_MODE_CONTROL_DATA_DEFINES
{
    // 咳痰周边设备的工作状态
    //uint8_t  cough_system_actived_flag;
    // 之前的工作模式,有两个功能
    // 1 在准备期间，维持之前的工作模式
    // 2 在咳痰结束时，返回之前的控制模式
    uint8_t prev_work_mode; // 之前的工作模式
    uint8_t prev_work_status; // 之前的控制状态

    // 压力上升时
    uint16_t rising_time_count;    // 计数，用于计算上升时间
    uint16_t rising_time_steps;    // 每次吸气时的总的上升次数
    uint16_t blower_increase_speed; // 压力上升时的步进（每50毫秒调整一次）

    int16_t cough_target_press;    // PID 当前的目标压力
    uint32_t last_time_stemp;      // 计计时时间戳（用于调整目标上升压力）

    // 压力保持
    uint32_t auto_cough_time_start_gate;// 自动咳痰时间（长时间不能检测到
    // 咳痰时
    /* *****************************************************************
       检测条件及阈值定义：
       stop_mode					 stop_value   单位(uint)
       = 0   潮气量阈值(绝对值)			 0-150			x10ml
       = 1   潮气量百分比				 50-100     %
       = 2   呼气流量阈值				 0-100			L/min
       = 3   时间阈值          1-20			0.1秒
    * *************************************************************** */
    uint16_t cough_finish_gate_vt; // 潮气量
    uint16_t cough_finish_gate_vt_cent; // 潮气量百分比(50-100)
    int32_t cough_finish_gate_flow_e; // 呼气流量阈值（0-100L/min）
    uint32_t cough_finish_gate_time_e; // 时间阈值(0.1-2.0秒)


    // 负压排痰工作状态
    uint8_t vacuum_pressure_work_status; // 负压排痰操作状态 =1：表示正在进行负压排痰， = 0表示没有进行负压排痰
    uint8_t cough_display_data_ready;
} ST_COUGH_MODE_CONTROL_DATA_DEFINES;
extern ST_COUGH_MODE_CONTROL_DATA_DEFINES cough_control_data;

typedef struct __ST_INCREASE_PRESS_STEP_DATA_DEFINES
{
    int32_t d_press; // 总的上升压力
    int32_t total_steps; // 总的上升步数
} ST_INCREASE_PRESS_STEP_DATA_DEFINES;
extern ST_INCREASE_PRESS_STEP_DATA_DEFINES ST_increase_press_data;
extern ST_INCREASE_PRESS_STEP_DATA_DEFINES PCV_increase_press_data;
extern ST_INCREASE_PRESS_STEP_DATA_DEFINES cough_increase_press_data;

/* *******************For S/T mode******************************<-- */

void breath_ST_inhale_monitor_status(void);

void set_changemode_actived_mode(uint8_t mode);
uint8_t check_mode_change_flag(void);

void reset_main_control_data(void);
void reset_ui_set_data(void);

// 不进行合法性检测，需要调用者自己确认
void set_patient_breath_mode(uint8_t mode);
uint8_t get_patient_breath_mode(void);
void set_PID_blower_control_status(uint8_t status);
uint8_t get_main_control_status(void);

/* ***************STANDBY **************
 */
void breath_standby_mode(void);
void to_breath_standby_mode(void);


/* ***************CPAP **************
 */
void breath_cpap_mode(void);
void to_breath_cpap_mode(void);


/* ***************PAV **************
 */
void breath_pav_mode(void);
void to_breath_pav_mode(void);


/* ***************BIPAP-ST **************
 */
void breath_st_mode(void);
void to_breath_stv_mode(void);
int16_t get_ST_T_target_press(void);


/* ***************BIPAP-PCV **************
 */
void breath_pcv_mode(void);
void to_breath_pcv_mode(void);

/* ***************BiLevel **************
 */
void breath_bilevel_mode(void);
void to_breath_bilevel_mode(void);

/* ***************SPONTANEOUS_MODE **************
 */
void breath_spontaneous_mode(void);
void to_breath_spontaneous_mode(void);


/* ***************AC-P **************
 */
void breath_ac_p_mode(void);
void to_breath_ac_p_mode(void);


/* ***************AC-V **************
 */
void breath_ac_v_mode(void);
void to_breath_ac_v_mode(void);



/* ***************SIMV-PCV **************
 */
void breath_simv_pcv_mode(void);
void to_breath_simv_pcv_mode(void);


/* ***************SIMV-VCV **************
 */
void breath_simv_vcv_mode(void);
void to_breath_simv_vcv_mode(void);

/* ***************  COUGH  **************
 */
void stop_cough_blower_work(void);
void cough_back_to_prev_work(void);
void force_start_vacuum_pump(void);
void UI_reapeat_cough_com(void);
int16_t get_Cough_target_press(void);


/* *******************************************
*  总的控制入口
* *******************************************/
void breath_mode_control(void);


uint8_t is_ui_change_parameters(void);
void set_ui_change_parameters(void);

uint8_t  is_epap_gate_ready(int16_t delta_p);
uint8_t  is_ST_breath_exhale_start_time(void);
void     to_default_breath_status(void);
uint16_t pcv_get_inhale_end_time(void);
int16_t  get_PCV_T_target_press(void);
uint16_t PCV_get_exhale_end_time(void);

uint16_t ST_get_exhale_end_time(void);


void     copy_control_data(ST_MAIN_CONTROL_DATA *src, ST_MAIN_CONTROL_DATA *tar);
void     breath_AI_back_to_prev_mode(void);

#endif
