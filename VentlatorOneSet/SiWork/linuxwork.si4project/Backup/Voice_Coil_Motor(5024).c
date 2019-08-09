// File: Voice_Coil_Motor.c
/*
    音圈电机的控制文件
    音圈电机由于其本身设计问题，存在开启电压和饱和电压
    此文件不仅包含音圈电机的控制，还包含 （氧气控制）比例阀控制
    涡轮控制在单独的文件里，很有可能会更换控制逻辑（硬件）
    PWM使用TIMER8的4路输出，其中通道2用于控制涡轮（吸气模块），
    其他三路的控制都在此文件，    这三路包括：
        A 吸气阀回流控制（吸气模块）
        B PEEP及叩击控制（叩击及呼气模块）
        C 氧气控制（混氧模块）
 */

#include "../global.h"

#include "Voice_Coil_Motor.h"

//VOICE_COIL_MOTOR_CONTROL voice_coil_motor_control_data;
//VOICE_COIL_MOTOR_BASE coil_motor_base;


void init_voice_coil_motor_base(
    VOICE_COIL_MOTOR_BASE *base,
    uint16_t open_val,           // 开启值
    uint16_t full_val,           // 饱和值
    uint32_t chanel)             // PWM通道
{
    base->open_val    = open_val;
    base->full_val    = full_val;
    base->scale       = (double) full_val - (double) open_val;
    base->scale      /= 4000;
    base->chanel      = chanel;
    base->current_set = 0;
}

void init_voice_coil_motor_control_data(VOICE_COIL_MOTOR_CONTROL *base)
{
    // 状态
    base->mode                = EM_VOICE_COIL_MOTOR_MODE_STANDBY;// 模式
    base->status              = EM_VOICE_COIL_MOTOR_STOP;      // 当前的状态
    base->cycles              = 0;                             // 当前运行的循环数

    // 控制 A 段 上升段
    base->control_a_val       = 0;                             // 每次调整增量值 0-4000
    base->control_a_time      = 0;                             // 控制时间
    // 控制 B 段 第一保持段（高位）
    base->control_b_val       = 0;                             // 保持值 0-4000
    base->control_b_time      = 0;                             // 保持时间
    // 控制	C 段 下降段
    base->control_c_val       = 0;                             // 每次调整增量值 0-4000
    base->control_c_time      = 0;                             // 控制时间
    // 控制 D 段 第二保持段（低位）
    base->control_d_val       = 0;                             // 保持值 0-4000
    base->control_d_time      = 0;                             // 保持时间

    //当前的记录 （运行时的临时变量，有些特殊控制，需要临时设定/更改）
    base->current_control_val = 0;                             // 当前阀控制值
    base->current_clk         = 0;                             // 当前的计数
    base->target_clk          = 0;                             // 目标计数
}

void set_voice_coil_motor_control_x_bank(
    VOICE_COIL_MOTOR_CONTROL *base,
    uint8_t bank,
    uint16_t val,
    uint16_t time)
{
    switch(bank)
    {
    case EM_VOICE_COIL_MOTOR_BANK_A:
        base->control_a_val = val;
        base->control_a_time = time;
        break;
    case EM_VOICE_COIL_MOTOR_BANK_B:
        base->control_b_val = val;
        base->control_b_time = time;
        break;
    case EM_VOICE_COIL_MOTOR_BANK_C:
        base->control_c_val = val;
        base->control_c_time = time;
        break;
    case EM_VOICE_COIL_MOTOR_BANK_D:
        base->control_d_val = val;
        base->control_d_time = time;
        break;
    case EM_VOICE_COIL_MOTOR_BANK_X:
    default:
        break;
    }
}
void set_voice_coil_motor_base_val(VOICE_COIL_MOTOR_BASE *base, uint16_t val);

void start_voice_coil_motor(VOICE_COIL_MOTOR_CONTROL *dbase)
{
    dbase->mode                = EM_VOICE_COIL_MOTOR_MODE_RUNNING;// 模式
    dbase->status              = EM_VOICE_COIL_MOTOR_A_PERIOD;  // 当前的状态
    dbase->cycles              = 0;                             // 当前运行的循环数
    dbase->target_clk          = dbase->control_d_time;
    dbase->current_clk         = 0;
    dbase->current_control_val = dbase->control_d_val;

    set_voice_coil_motor_base_val(&(dbase->base), dbase->control_d_val);
}

void stop_voice_coil_motor(VOICE_COIL_MOTOR_CONTROL *dbase)
{
    dbase->mode                = EM_VOICE_COIL_MOTOR_MODE_STANDBY;// 模式
    dbase->status              = EM_VOICE_COIL_MOTOR_STOP;      // 当前的状态
    dbase->cycles              = 0;                             // 当前运行的循环数
    dbase->target_clk          = 0;
    dbase->current_clk         = 0;
    dbase->current_control_val = 0;

    set_voice_coil_motor_base_val(&(dbase->base), dbase->control_d_val);
}

// val 为千分比
void set_voice_coil_motor_base_val(VOICE_COIL_MOTOR_BASE *base, uint16_t val)
{
    double dtmp;
    if(val > MAX_PWM_VAL) val = MAX_PWM_VAL; // 确定不会超过最大控制量
    // 由千分比计算控制值
    dtmp  = (double) val;
    dtmp *= base->scale;
    dtmp += (double)base->open_val;
    base->current_set = (uint16_t) dtmp;
    if(base->current_set > MAX_PWM_VAL) base->current_set = MAX_PWM_VAL; // 确定不会超过最大控制量

    PWM_change_val(base->chanel, MAX_PWM_VAL - base->current_set);
}

void inc_voice_coil_motor_base_val(    VOICE_COIL_MOTOR_BASE *base,    uint16_t inc_val)
{
    double dtmp;
    if(inc_val > MAX_PWM_VAL) inc_val = MAX_PWM_VAL; // 确定不会超过最大控制量
    // 由千分比计算控制值
    dtmp  = (double) inc_val;
    dtmp *= base->scale;
    dtmp += (double)base->current_set;
    if((uint16_t)dtmp > base->full_val)
    {
        base->current_set = base->full_val;
    }
    else
    {
        base->current_set = (uint16_t) dtmp;
    }

    // if(base->current_set > MAX_PWM_VAL) base->current_set = MAX_PWM_VAL; // 确定不会超过最大控制量
    PWM_change_val(base->chanel, MAX_PWM_VAL - base->current_set);
}

void dec_voice_coil_motor_base_val(    VOICE_COIL_MOTOR_BASE *base,    uint16_t dec_val)
{
    double dtmp;
    if(dec_val > MAX_PWM_VAL) dec_val = MAX_PWM_VAL; // 确定不会超过最大控制量
    // 由千分比计算控制值
    dtmp  = (double) dec_val;
    dtmp *= base->scale;
    dtmp  = (double)base->current_set - dtmp;
    if((int16_t)dtmp < base->open_val)
    {
        base->current_set = base->open_val;
    }
    else
    {
        base->current_set = (uint16_t) dtmp;
    }

    PWM_change_val(base->chanel, MAX_PWM_VAL - base->current_set);
}


// 这个函数主要用于叩击控制，产生那个叩击效果
void do_voice_coil_motor_actions(VOICE_COIL_MOTOR_CONTROL *motor)
{
    VOICE_COIL_MOTOR_BASE *voice_motor_base;
    voice_motor_base = (VOICE_COIL_MOTOR_BASE *) & (motor->base);
    switch(motor->status)
    {
    case EM_VOICE_COIL_MOTOR_STOP: // 停止状态
        //do nothing
        break;
    case EM_VOICE_COIL_MOTOR_A_PERIOD:
        // 增量
        inc_voice_coil_motor_base_val(voice_motor_base, motor->control_a_val);
        // 判断是否到达
        if(((motor->current_clk)++) >= motor->target_clk)
        {
            // 要进入下一阶段
            motor->status      = EM_VOICE_COIL_MOTOR_B_PERIOD;
            motor->current_clk = 0;
            motor->target_clk  = motor->control_b_time;
        }
        break;
    case EM_VOICE_COIL_MOTOR_B_PERIOD:
        // keep status, do not set any
        // 判断是否到达
        if(((motor->current_clk)++) >= motor->target_clk)
        {
            // 要进入下一阶段
            motor->status      = EM_VOICE_COIL_MOTOR_C_PERIOD;
            motor->current_clk = 0;
            motor->target_clk  = motor->control_c_time;
        }
        break;
    case EM_VOICE_COIL_MOTOR_C_PERIOD:
        // 减量
        dec_voice_coil_motor_base_val(voice_motor_base, motor->control_c_val);
        // 判断是否到达

        if(((motor->current_clk)++) >= motor->target_clk)
        {
            // 要进入下一阶段
            motor->status      = EM_VOICE_COIL_MOTOR_D_PERIOD;
            motor->current_clk = 0;
            motor->target_clk  = motor->control_d_time;
        }
        break;
    case EM_VOICE_COIL_MOTOR_D_PERIOD:
        // keep status, do not set any
        // 判断是否到达
        if(((motor->current_clk)++) >= motor->target_clk)
        {
            // 要进入下一阶段
            motor->status      = EM_VOICE_COIL_MOTOR_NEXT_LOOPS;
            motor->current_clk = 0;
            motor->target_clk  = motor->control_c_time;
        }
        break;
    case EM_VOICE_COIL_MOTOR_NEXT_LOOPS:
        // inc cycles
        (motor->cycles)++;
        // reset loops
        motor->status      = EM_VOICE_COIL_MOTOR_A_PERIOD;
        motor->current_clk = 0;
        motor->target_clk  = motor->control_a_time;
        break;
    default:
        motor->status      = EM_VOICE_COIL_MOTOR_STOP;
        // call stop
        break;
    }
}



// File Voice_Control_Motor.c end
