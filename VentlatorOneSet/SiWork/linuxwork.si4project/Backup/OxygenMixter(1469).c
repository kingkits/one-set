//File OxygenMixter.c
#include "../global.h"
#include "OxygenMixter.h"
#include "../adc/adc_manager.h"
#include "../lib/insert_data.h"

#include "../ControlTask/base_control.h"

ST_OXYGEN_MIXER_PID oxygen_pid;
ST_OXYGEN_CONTROL_DATA oxygen_mixer_ctrol;
uint8_t Oxygenmixer_actived_flag;
extern uint32_t Oxgen_Valve_Flow_data[];
extern uint32_t Oxgen_Valve_Control_data[];
extern ST_INSERT_DATA oxygen_valve_control_data;

uint16_t get_inspire_oxygen_concent(void);
void     set_base_info_pid(int dat);


void PWM_change_val(uint32_t chanel, uint32_t val);
void PWM_start(uint32_t chanel);
void PWM_stop(uint32_t chanel);

void set_peep_valve_control_val(uint16_t val)
{
    if(val > MAX_PWM_VAL)
    {
        PWM_change_val(PWM_CHANEL_RAP, MAX_PWM_VAL);
        set_peep_pwm_comtrol_data(MAX_PWM_VAL);
    }
    else
    {
        PWM_change_val(PWM_CHANEL_RAP, val);
        set_peep_pwm_comtrol_data(val);
    }
}

void reset_oxygen_valve_control_data(void)
{
    init_insert_data(&oxygen_valve_control_data, 30, Oxgen_Valve_Flow_data, Oxgen_Valve_Control_data, ENUM_DATA_IS_INCREASE);
}
uint16_t get_oxygen_concentration(uint16_t adc_val)
{

    int32_t val;
    val  = (uint32_t)adc_val - 760;
    val *= 574;
    val /= 1000;
    val += 210;
    if(val < 150)
        val = 150;
    return (uint16_t) val;
}

void set_oxygen_valve_control_val(uint16_t val)
{
    //if(Oxygenmixer_actived_flag == 0) return;
    if(val > MAX_PWM_VAL)
    {
        PWM_change_val(PWM_CHANEL_OXYGEN, MAX_PWM_VAL);
        set_oxygen_pwm_comtrol_data(MAX_PWM_VAL);
    }
    else
    {
        PWM_change_val(PWM_CHANEL_OXYGEN, val);
        set_oxygen_pwm_comtrol_data(val);
    }
}



#define OXYGEN_COUNT_REF 0.00126582278481012658227848101266 // = 1/790
#define OXYGEN_POINT     1.7670886075949367088607594936709

// oxygen_adjust()
// input : Oxygen concentration
//         Flow in airway
//output : flow in oxgen_control
/*
   基础调整的算法：  Fo 氧气流量
                  Fi  气路流量
                  Co 设置的氧浓度
                  Fo = Fi * (Co - 0.21)  / 0.79
 */
__weak void oxygen_adjust(void)
{
    float64_t flow;
    float64_t ftmp;
    if(Oxygenmixer_actived_flag == 0)
        return;

    //得到当前流量
    flow = (float64_t) get_inspire_flow();

    //由公式计算氧气供给流量
    ftmp = oxygen_mixer_ctrol.set_concentration - 210;
    if(ftmp < 0.0)
        ftmp = 0.0;
    // /790
    ftmp = ftmp * OXYGEN_COUNT_REF;
    ftmp = ftmp * flow; // 需要的氧气流量 ml/min

    // 加入补偿因素  // PID计算的补充补偿参数
    ftmp = ftmp * (1.0 + oxygen_mixer_ctrol.adjust_mul); // 比例补偿
    ftmp += oxygen_mixer_ctrol.adjust_add; // 增量补偿


    //计算供氧比例阀控制参数
    oxygen_mixer_ctrol.control_data = (uint16_t) get_insert_inc_data((uint32_t)ftmp, &oxygen_valve_control_data);

    // 控制
    set_oxygen_valve_control_val(oxygen_mixer_ctrol.control_data);
}

void set_concentration_value(uint8_t val)
{
    uint8_t data;
    data = val;
    if(data < 21)
        data = 21;
    if(data > 100)
        data = 100;
    oxygen_mixer_ctrol.set_concentration = (float64_t)data * 10.0;
    Oxygenmixer_actived_flag = 1;
    if(Oxygenmixer_actived_flag)
    {
        if(oxygen_mixer_ctrol.set_concentration > get_inspire_oxygen_concent() + 50.0)
        {
            oxygen_pid_refresh_mode(ENUM_PID_OXYGEN_CONCENTRATION_LOW_START);
        }
        else if(oxygen_mixer_ctrol.set_concentration < get_inspire_oxygen_concent() - 50.0)
        {
            oxygen_pid_refresh_mode(ENUM_PID_OXYGEN_CONCENTRATION_HIGH_START);
        }
        else
        {
            oxygen_pid_refresh_mode(ENUM_PID_OXYGEN_MIXER_ACTIVED);
        }
    }
}



void oxygen_start(void)
{
    Oxygenmixer_actived_flag = 1;
    PWM_start(PWM_CHANEL_OXYGEN);
    PWM_change_val(PWM_CHANEL_OXYGEN, 0);
    set_oxygen_pwm_comtrol_data(0);
}

void oxygen_stop(void)
{
    Oxygenmixer_actived_flag = 0;
    //PWM_stop(PWM_CHANEL_OXYGEN);
    PWM_change_val(PWM_CHANEL_OXYGEN, 0);
    set_oxygen_pwm_comtrol_data(0);
}



//void pid_init(PID_t *pid, float kp, float ki, float kd, float intmax, unsigned char mode);
void oxygen_pid_reset(void)
{
    oxygen_pid.mode            = ENUM_PID_OXYGEN_MIXER_NOT_ACTIVED;
    oxygen_pid.time_count      = 0;
    oxygen_mixer_ctrol.pid_ptr = &(oxygen_pid.pid);
    pid_init(oxygen_mixer_ctrol.pid_ptr, 0.2, 0.03, 0.1, 500.0, PID_MODE_DERIVATIV_CALC);
}

void oxygen_pid_refresh_mode(uint8_t mode)
{
    oxygen_pid.mode = mode;
}


/* ***********************************************
* 混氧调整步骤：代码需要重写
*    1 当氧浓度改变时，计时清零，需要重新设定状态， ***PID数据不会清零
*    2 只有在到达指定门限时，PID调整才会打开
*      (2.1 计时器计时到， 2.2 浓度穿过设定值)
*    3 当PID打开后，调整时间会依据偏差适当调整
*    4 当测定的氧浓度穿过设定的氧浓度时，应该及时进行调整一次（幅度约为偏差的20%）
*    5 当接近设定的氧浓度时，应该适当增加调整时间
* ***********************************************/
// 每3.2秒调用一次，但实际调整时，需要32秒调整一次
__weak void oxygen_pid_adjust(void)
{
    float64_t ftmp;
    float64_t pid_val;
    float64_t current_o2_concent;
    if(!Oxygenmixer_actived_flag)
        return;       // 无氧浓度控制
    if(oxygen_mixer_ctrol.set_concentration < 220)
        return; // 未设置氧浓度，不进行调整

    switch(oxygen_pid.mode)
    {
    case ENUM_PID_OXYGEN_MIXER_NOT_ACTIVED:    // 未工作
        return;
    case ENUM_PID_OXYGEN_CONCENTRATION_LOW_START: // 从低于设置的氧浓度值开始
        if(oxygen_mixer_ctrol.set_concentration < get_inspire_oxygen_concent() + 40.0)
        {
            //oxygen_pid_refresh_mode(ENUM_PID_OXYGEN_MIXER_ACTIVED);
            current_o2_concent = oxygen_mixer_ctrol.set_concentration - 40.0;
            pid_val = pid_calculate(
                          &(oxygen_pid.pid),              // *pid_ptr
                          oxygen_mixer_ctrol.set_concentration, // sp
                          current_o2_concent,             // val
                          0,                              // val_dot
                          1.0);
            oxygen_pid.time_count++;
            oxygen_pid.time_count++;
            oxygen_pid.time_count++;// 加快节奏
            break;
        }
        oxygen_pid.time_count++;
        if(oxygen_pid.time_count < 15)
        {
            return;
        }
        oxygen_pid.time_count = 0;
        oxygen_pid_refresh_mode(ENUM_PID_OXYGEN_MIXER_ACTIVED);
        break;
    case ENUM_PID_OXYGEN_CONCENTRATION_HIGH_START: // 从高于设置的氧浓度值开始
        if(oxygen_mixer_ctrol.set_concentration > get_inspire_oxygen_concent() - 50.0)
        {
            //oxygen_pid_refresh_mode(ENUM_PID_OXYGEN_MIXER_ACTIVED);
            current_o2_concent = oxygen_mixer_ctrol.set_concentration + 50.0;
            pid_val = pid_calculate(
                          &(oxygen_pid.pid),              // *pid_ptr
                          oxygen_mixer_ctrol.set_concentration, // sp
                          current_o2_concent,             // val
                          0,                              // val_dot
                          1.0);
            oxygen_pid.time_count++;
            oxygen_pid.time_count++;
            oxygen_pid.time_count++;// 加快节奏
            break;
        }
        oxygen_pid.time_count++;
        if(oxygen_pid.time_count < 10)
        {
            return;
        }
        oxygen_pid.time_count = 0;
        oxygen_pid_refresh_mode(ENUM_PID_OXYGEN_MIXER_ACTIVED);
        break;

    case ENUM_PID_OXYGEN_MIXER_ACTIVED:        // 正常工作
        break;
    default:
        oxygen_pid_refresh_mode(ENUM_PID_OXYGEN_MIXER_NOT_ACTIVED);
        break;
    }

    // 确保不会除0
    if(oxygen_mixer_ctrol.set_concentration <= 210.0)
    {
        oxygen_mixer_ctrol.set_concentration = 210.0;
        // 不需要调整
        return;
    }
    if(oxygen_mixer_ctrol.set_concentration > 1000.0)
    {
        oxygen_mixer_ctrol.set_concentration = 1000.0;
    }

    oxygen_pid.time_count++;
    if(oxygen_pid.time_count < 6)
        return;
    oxygen_pid.time_count = 0;

    //得到当前流量
    ftmp = (float64_t) get_inspire_flow();
    if(ftmp < 500)
        return; // 流量太小不调整

    // 得到上次补偿后的实际数值
    current_o2_concent = (float64_t)get_inspire_oxygen_concent();

    // 计算与预期数值的差异
    // PID 控制过程 计算新的补偿参数
    pid_val = pid_calculate(
                  &(oxygen_pid.pid),                      // *pid_ptr
                  oxygen_mixer_ctrol.set_concentration,   // sp
                  current_o2_concent,                     // val
                  0,                                      // val_dot
                  1.0);                                  // dt

    // 更新调整数据
    oxygen_mixer_ctrol.adjust_mul = pid_val / 100;
    set_base_info_pid(1);
}


__weak void reset_oxygen_mixer_control_data(void)
{
    // 基本参数复位
    oxygen_mixer_ctrol.work_status       = EM_INACTIVED;
    oxygen_mixer_ctrol.control_data      = 0;
    oxygen_mixer_ctrol.set_concentration = 210.0;
    Oxygenmixer_actived_flag             = 0;

    // 调整的过程参数
    oxygen_mixer_ctrol.base_val          = 0.0;
    oxygen_mixer_ctrol.adjust_mul        = 0.0;
    oxygen_mixer_ctrol.adjust_add        = 0.0;

    // 氧气阀实际控制值
    oxygen_mixer_ctrol.control_data      = 0;

}

__weak void reset_oxygen_mixer(void)
{
#if USE_ADC_RAM_DATA
#else
    reset_oxygen_valve_control_data();
#endif

    reset_oxygen_mixer_control_data();

    // PID复位
    oxygen_pid_reset();
}

// --------------Oxygenmixer.c end
