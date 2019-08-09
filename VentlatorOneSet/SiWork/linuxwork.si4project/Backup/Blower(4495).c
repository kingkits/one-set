// File: Blower.c
/*
    风机调整程序
    目前先做简单调整
    后期再做精细的调整
    为了控制一致（主要是输出 0），PWM必须反相
 */
#include "../global.h"
#include "../comunication/Ui_data.h"
#include "Blower.h"
#include "../lib/insert_data.h"
#include "../ControlTask/base_control.h"


#define MAX_BLOWER_SPEED 40000

void start_timer6(void);
void stop_timer6(void);
void start_timer4(void);
void stop_timer4(void);

extern volatile uint32_t blower_count;


// 数据原型定义在这里
ST_BLOWER_STATUS blower_status;
ST_BLOWER_CONTROL_DATA blower_main;
extern uint32_t blower_control_idx[], blower_control_data[];
extern ST_INSERT_DATA blower_speed_control_data;

// 初始化涡轮转速相关的数据
void reset_blower_rpm_data(void)
{
    init_insert_data(&blower_speed_control_data, 34, blower_control_idx, blower_control_data, ENUM_DATA_IS_INCREASE);
}

// 风机复位，
void reset_blower_control_data(void)
{
    // 工作状态
    blower_main.work_status           = EM_BLOWER_STOP;
    blower_main.temperature_status    = 0; // 温度状态
    // control
    blower_main.direction             = EM_FORWARD;
    blower_main.set_speed             = 0;
    blower_main.control_data          = 0; //PWM set_val
    blower_main.overflow_control_data = 0;

    // pid data
    blower_main.pid_ptr = (PID_t *) 0; // null ptr
}

void reset_blower_status(void)
{
    blower_status.speed_s      = 0;
    blower_status.data_ptr     = 0;
    blower_status.speed        = 0;
    blower_status.actived_flag = 1;
}

void set_blower_speed_s(uint32_t speed)
{
    blower_status.speed_s = speed;
}


// 用于计算涡轮转速的程序
void count_blower_speed(void)
{
    uint32_t val;
    val = 0;
    val = blower_status.speed_s;
    val *= 10;

    if(val > 65535) val = 65535;
    blower_status.speed = (uint16_t) val;
}


// 涡轮风机初始化
void reset_blower(void)
{
#if USE_ADC_RAM_DATA
#else
    reset_blower_rpm_data();
#endif
    reset_blower_control_data();
    blower_count = 0;
    Blower_enable();
    start_timer4();
    start_timer6();
}


// 设置风机速度
// 默认4万转为最大转速
void set_blower_speed(uint16_t speed)
{
    double val1;
    //if(speed > MAX_BLOWER_SPEED)speed = MAX_BLOWER_SPEED;
    // 工作状态
    blower_main.work_status  = EM_BLOWER_RUNNING;
    blower_main.set_speed    = speed;
    val1 = get_insert_inc_data(speed, &blower_speed_control_data);

    // backup
    set_blower_pwm_comtrol_data(val1);

    PWM_change_val(PWM_CHANEL_BLOWER, (int32_t) val1);
    blower_main.control_data = (uint16_t) val1;
}

void Blower_stop(void)
{
    set_blower_speed(0);
    blower_main.work_status = EM_BLOWER_STOP;
}

uint16_t get_blower_current_set_speed(void)
{
    return blower_main.set_speed;
}
// 使能涡轮的控制，目前 使能后风机转动速度为5000rpm
void Blower_enable(void)
{
    // Power
    HAL_GPIO_WritePin(Blower_Enable_GPIO_Port, Blower_Enable_Pin, GPIO_PIN_SET);

    // enable control
    HAL_GPIO_WritePin(_Disable_GPIO_Port, _Disable_Pin, GPIO_PIN_RESET);

    PWM_start(PWM_CHANEL_BLOWER);

    PWM_start(PWM_CHANEL_RAP);
}


void Overflow_enable(void)
{
    PWM_start(PWM_CHANEL_OVERFLOW);
}

void Overflow_disable(void)
{
    PWM_stop(PWM_CHANEL_OVERFLOW);
}

// 禁止涡轮控制
void Blower_disable(void)
{
    blower_main.work_status = EM_BLOWER_STOP;

    // enable control
    HAL_GPIO_WritePin(_Disable_GPIO_Port, _Disable_Pin, GPIO_PIN_SET);

    // Power
    HAL_GPIO_WritePin(Blower_Enable_GPIO_Port, Blower_Enable_Pin, GPIO_PIN_RESET);
}

// 回流阀的设置函数
void set_overflow_valve(uint16_t data)
{
    if(data > MAX_PWM_VAL)
    {
        blower_main.overflow_control_data = MAX_PWM_VAL;
    }
    else
    {
        blower_main.overflow_control_data = data;
    }
    set_flowback_pwm_comtrol_data(blower_main.overflow_control_data);
    PWM_change_val(PWM_CHANEL_OVERFLOW, blower_main.overflow_control_data);
}


/* 涡轮风机的调整包括2个方面，
   1 是风机本身的转速设定
   2 是回流阀的调整
   依据工作需要，涡轮调整暂定10个步进，一次步进为 4000转/分钟（约为5个厘米水），
   当回流阀关闭到1/3（暂定）还不能实现预期压力时，需要提升转速到下一个档次。
   而回流阀至少需要完成回流1/2（推测）的流量。
   正常时，回流阀处在半开（处于平衡点，不一定是1/2面积处，需要仔细测试）的状态，
   以应对气道内的微小压力变化
   下一步测试涡轮工作的内容就是：
     1 找出10-12个涡轮风机的工作点（控制电压），每个工作点在堵口状态及回流阀完全关闭下的气道压力相差约为5厘米水
     2 在每个工作点下，堵口状态及回流阀完全关闭下的气道压力（同上）
     3 在每个工作点下，完成上面一半压力的回流阀工作点（控制数据）
     4 每个工作点下，回流阀全开控制电压（相对精确的值）（猜测有一定漂移）
     5 每个工作点配置一个调整方案
       建立一个通用型的控制方法，基于如下条件      <待完善>
       a 工作点涡轮风机的控制电压
       b 堵口压力
       c 回流阀数据
          c1 堵口时的控制压力和气道压力(可能是一个表)
          c2 堵口时控制c1一半压力的电压，及此控制电压在开口时的流量
          c3 全开和全闭的控制电压（可能有差异）

       * *****（可控制性测试）
     6 每个工作点下，堵住输出，回流阀的调整对压力变化的影响精度
     7 每个工作点下，敞开输出，回流阀的调整对输出流量的影响精度
     8 固定泄漏（回流阀全开时，泄漏压力调整为堵口压力的一半）
       测试回流阀的变化对压力及输出流量的影响
       * 管路脱落的状态
 回流阀调整依赖于流量及工作压力，是个极为复杂的工作
 也许称为压力调节会更好些

 */
#if 0
typedef struct __ST_BLOER_CONTROL_BASE
{
    uint8_t level; // 涡轮控制等级
    uint16_t max_press;// 堵口 最大压力
    uint16_t min_press;// 堵口 最小压力
    uint32_t max_flow;// 开口最大流速
    uint32_t min_flow;// 开口最小流速

    uint16_t max_overflow_ctrol;// 回流阀 最大控制值（闭合值）
    uint16_t mid_overflow_ctrol;// 回流阀 中间控制值
    uint16_t min_overflow_ctrol;// 回流阀 最小控制值（最大打开值）
} ST_BLOER_CONTROL_BASE;

typedef struct __ST_BLOWER_STATUS_DEFINED
{
    uint8_t type; // 是压力控制还是流量控制还是转速控制
    uint16_t target_press; // 依赖于type
    uint32_t target_flow;  // 依赖于type
    uint16_t target_rpm;   // 依赖于type
} ST_BLOWER_STATUS_DEFINED;
typedef struct __ST_BREATH_CONTROL
{
    ST_BLOER_CONTROL_BASE *blower_base;
    ST_BLOWER_STATUS_DEFINED *blower_target;
    ST_BLOWER_STATUS_DEFINED *blower_current;
    ST_BLOWER_STATUS_DEFINED *blower_prev;
    ST_BLOWER_STATUS_DEFINED *blower_temp;
} ST_BREATH_CONTROL_TYPES;
#endif

// File: Blower.c End
