/**
 * File:pcv_control.c
 */

#include "stm32f4xx_hal.h"
#include "../ControlTask/base_control.h"
#include "../global.h"
#include "breath_rap.h"
#include "../blower/blower.h"
#include "../adc/adc_manager.h"
#include "../lib/leak_flow_count.h"
#include "../lib/VentLIB.h"
#include "breath_Atomizer.h"
#include "breath_cycle_adjust.h"
#include "pcv_press_increase_data.c"
#include "pcv_control.h"

enum
{
    EM_RAP_ACTION_IDLE,
    EM_RAP_ACTION_ACTIVE,
    EM_RAP_ACTION_STOP
} ENUM_RAP_WORK_STATUS_TYPES;


ST_PCV_CONTROL_DAT st_pcv_control_dat;

#define is_pcv_press_increase_finished() (st_pcv_control_dat.inc_data_count >= st_pcv_control_dat.max_inc_data_num)

void reset_rap_actions(void)
{
    st_pcv_control_dat.rap_action_flag = EM_RAP_ACTION_IDLE;
}

void do_rap_actions(void)
{
    switch(st_pcv_control_dat.rap_action_flag)
    {
    default:
    case EM_RAP_ACTION_IDLE:
        // start rap
        break;
    case EM_RAP_ACTION_ACTIVE:
        // check when it need stop
        break;
    case EM_RAP_ACTION_STOP:
        // just waiting
        break;
    }
}

//	float *inc_data_ptr;  //数据指针
//	int inc_data_count;   //计数（）
//	int max_inc_data_num; //最大计数
void set_pcv_increase_data(void)
{
    if(main_control_data.T_rising_time > 4) main_control_data.T_rising_time = 4;
    st_pcv_control_dat.inc_data_ptr = (float *) PCV_EXP_INCREASE_DATA[main_control_data.T_rising_time];
    st_pcv_control_dat.max_inc_data_num = PCV_INCREASE_DATA_LEN[main_control_data.T_rising_time];
    st_pcv_control_dat.inc_data_count = 0; // reset
}


void prepare_pcv_first_time(void)
{
    // 临时变量
    uint32_t t1;
    t1 = 60000 / main_control_data.T_freq; // 计算每一周期的时间，用于启动T模式
    st_pcv_control_dat.one_breath_time        = t1;
    // 由呼吸频率决定，确定T触发时间
    // ！要由呼气相开始
    st_pcv_control_dat.breath_start_gate = ms_1_count + t1 - main_control_data.T_inspire_time;
    st_pcv_control_dat.expire_start_gate      = ms_1_count;

    if(t1 > main_control_data.T_inspire_time + 500)
    {
        st_pcv_control_dat.T_expire_3_4_gate    = ((t1 - main_control_data.T_inspire_time) * 3 / 4) + ms_1_count;
    }
    else
    {
        st_pcv_control_dat.T_expire_3_4_gate    = st_pcv_control_dat.breath_start_gate - 200;
    }
}

void prepare_pcv_time(void)
{
    // 临时变量
    uint32_t t1;
    t1 = 60000 / main_control_data.T_freq; // 计算每一周期的时间，用于启动T模式
    st_pcv_control_dat.one_breath_time        = t1;
    // 由呼吸频率决定，确定T触发时间
    // ！要由呼气相开始
    st_pcv_control_dat.breath_start_gate = ms_1_count + t1;
    st_pcv_control_dat.expire_start_gate = ms_1_count + main_control_data.T_inspire_time;

    if(t1 > main_control_data.T_inspire_time + 500)
    {
        st_pcv_control_dat.T_expire_3_4_gate = ((t1 - main_control_data.T_inspire_time) * 3 / 4) + st_pcv_control_dat.expire_start_gate;
    }
    else
    {
        st_pcv_control_dat.T_expire_3_4_gate = st_pcv_control_dat.breath_start_gate - 200;
    }
}

/**
 * [prepare_PCV_control_data description]
 * @method prepare_PCV_control_data
 */
void prepare_PCV_control_data(void)
{
    // 基础数据
    st_pcv_control_dat.cycles = 0;
    // 基础数据-- 显示数据
    if(!is_ui_change_parameters())
    {
        reset_display_data(); // 初始化显示数据
    }
    mark_breath_T_stmap();// 吸呼气 时间戳		强制
    // 基础数据-- 时间
    prepare_pcv_first_time();

    // 基础数据-- 涡轮速度
    st_pcv_control_dat.low_speed = get_blower_speed_for_press_and_flow(main_control_data.epap, CONST_EXHALE_FLOW);
    st_pcv_control_dat.high_speed = get_blower_speed_for_press_and_flow(main_control_data.ipap, CONST_EXHALE_FLOW);
    st_pcv_control_dat.d_speed = (float) st_pcv_control_dat.high_speed - st_pcv_control_dat.low_speed;
    // 基础数据-- 呼气阀数据
    st_pcv_control_dat.peep_val = get_peep_value_with_press_and_flow(main_control_data.epap, CONST_EXHALE_FLOW);
    set_peep_valve_control_val(st_pcv_control_dat.peep_val); // 不用设置了。考虑到排痰，应该再复位一下
    // 模式控制-- 压力上升
    set_pcv_increase_data();
    // 模式控制-- 压力保持

    // 模式控制-- 压力下降（呼气）

    // 模式控制-- 触发

    // 模式控制-- 叩击
    reset_rap_actions();

    // 模式控制-- PID
    set_PID_blower_adjust_mode(ENUM_PID_NO_ADJUST);
    // 模式控制-- 进入pcv的状态
    set_patient_status(EM_PATIENT_T_INSPIRE_DETECT);
}


void pcv_set_speed(void)
{
    // count speed
    float cur_speed;
    uint16_t set_speed;
    if(st_pcv_control_dat.inc_data_count >= st_pcv_control_dat.max_inc_data_num) return;

    cur_speed = st_pcv_control_dat.d_speed * (*(st_pcv_control_dat.inc_data_ptr));
    cur_speed += (float32_t) st_pcv_control_dat.low_speed;
    // currect ptr
    (st_pcv_control_dat.inc_data_count) ++;
    (st_pcv_control_dat.inc_data_ptr) ++;

    // set blower
    set_speed = (uint16_t)cur_speed;
    if(set_speed > MAX_BLOWER_SPEED)set_speed = MAX_BLOWER_SPEED;
    set_blower_speed(set_speed);
}

void pcv_set_speed_high(void)
{
    set_blower_speed(st_pcv_control_dat.high_speed);
    // 这里可能还是需要PID
}

void pcv_set_speed_low(void)
{
    // 这里准备触发的方案

    set_blower_speed(st_pcv_control_dat.low_speed);
}
/**
 * [breath_pcv_mode description]
 * @method breath_pcv_mode
 */
void breath_pcv_mode(void)
{
    /* *******************************************************************/
    // 非正常状态
    /* *******************************************************************/
case EM_PATIENT_NOT_WORK:       // 待机 standby
    break;
case EM_PATIENT_PIPE_OUT:       // 管路处于脱落状态
    break;
    /* *******************************************************************/
    // 正常工作状态
    /* *******************************************************************/
case EM_PATIENT_BREATH_DETECT:  // 专门用于测试的状态
    break;
    /* *******************************************************************/
    // 机械通气过程
    /* *******************************************************************/
case EM_PATIENT_T_INSPIRE_START:
    // 检查UI设置是否有变化
    if(check_mode_change_flag())
        return;
    check_breath_PCV_parameter_change();
    // 要重置时间和控制
    prepare_pcv_time();
    set_pcv_increase_data();

    // 如果有必要，调整一下风机速度(low_speed & high_speed)
    // 设置起步状态
    // 涡轮+雾化
    pcv_set_speed();
    breath_force_operation_nebulizer(EM_ON);
    // 进入下一状态
    set_patient_status(EM_PATIENT_T_INSPIRE_PERIOD);
    break;
case EM_PATIENT_T_INSPIRE_PERIOD:
    //如果有气道压力高报警，需要处理一下
    //TBD

    // 压力上升阶段，禁止叩击及其它有干扰的
    // 安心控制涡轮风机
    // 判断是否结束
    if(is_pcv_press_increase_finished())
    {
        pcv_set_speed_high();
        set_patient_status(EM_PATIENT_BREATH_HOLD);
        // 关闭雾化
        breath_force_operation_nebulizer(EM_OFF);
        // 启动叩击

    }
    else break;
case EM_PATIENT_BREATH_HOLD:
    // 叩击过程
    // 判断是否结束
    if(0)
    {
        pcv_set_speed_low();
        set_patient_status(EM_PATIENT_T_EXPIRE_START);
    }
    break;
case EM_PATIENT_T_EXPIRE_START:
    // 设置涡轮

    // 叩击
    set_patient_status(EM_PATIENT_T_EXPIRE_PERIOD);
    break;
    //case EM_PATIENT_T_EXPIRE_DETECT:
    //无需检测
    //break;
case EM_PATIENT_T_EXPIRE_PERIOD:
    // 处理叩击
    break;
case EM_PATIENT_T_INSPIRE_DETECT:
    // PCV应该打开触发通道
    // 如果有触发则直接进入吸气相
    if(0)
    {
        if(st_pcv_control_dat.cycles ++ == 0)
        {
            display_temp_data.last_work_status = EM_PATIENT_INSPIRE_DETECT;
        }
        set_patient_status(EM_PATIENT_T_INSPIRE_START);
    }
    // 如果时间到，进入吸气相
    if(0)
    {
        if(st_pcv_control_dat.cycles ++ == 0)
        {
            display_temp_data.last_work_status = EM_PATIENT_INSPIRE_DETECT;
        }
        set_patient_status(EM_PATIENT_T_INSPIRE_START);
    }
    break;
    /* *******************************************************************/
    // 状态结束标记
    /* *******************************************************************/
case EM_PATIENT_STATUS_ENDS:
    break;
default:
    break;
}
