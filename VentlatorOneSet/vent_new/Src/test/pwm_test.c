//File pwm_test.c
#include "main.h"
#include "stm32f4xx_hal.h"
#include "pwm_test.h"
#include "../global.h"
void set_PWMx_percent(uint32_t        chanel, uint32_t val);
void PWM_change_val(uint32_t chanel, uint32_t val);
extern unsigned int display_flag;
extern TIM_HandleTypeDef htim8;

uint8_t get_key(void);



typedef enum
{
    ENUM_PWM_IDLE_MODE, // idle, waiting
    ENUM_PWM_LOOP_MODE, // up--> top -> keep --> down --> keep --> up ...
    ENUM_PWM_DOWN_MODE, // top --> delay(keep) ---step down ---> delay --> bottom (stop)
    ENUM_PWM_STEP_MODE  // step up --> delay(keep) ---step up ---> delay --> top (stop)
} ENUM_PWM_WORK_MODE;

typedef enum
{
    EM_PWM_READY,
    EM_PWM_RUNNING,
    EM_PWM_PAUSE,
    EM_PWM_STOP
} ENUM_PWM_WORK_STATUS;

typedef enum
{
    EM_LOOP_STEP_A,
    EM_LOOP_STEP_B,
    EM_LOOP_STEP_C,
    EM_LOOP_STEP_D
} ENUM_PWOM_LOOP_WORK_STATUS_TYPES;

typedef struct __ST_PWM_PORT_CONTROL
{
    uint8_t  actived_flag;
    uint8_t  mode;
    uint32_t chanel;

    uint16_t cycles; // 当前运行的循环数
    uint16_t set_cycles; // 设置的循环数
    // 控制 A 段 上升段
    uint16_t control_a_val;  // 增量控制值 0-4000
    uint16_t control_a_time; // 控制时间
    // 控制 B 段 高位保持段
    uint16_t control_b_val;  // 增量控制值 0-4000
    uint16_t control_b_time; // 控制时间
    // 控制   C 段 下降段
    uint16_t control_c_val;  // 增量控制值 0-4000
    uint16_t control_c_time; // 控制时间
    // 控制 D 段 低位保持段
    uint16_t control_d_val;  // 增量控制值 0-4000
    uint16_t control_d_time; // 控制时间

    // 百分比上升控制
    uint16_t inc_step_val;   // 每一步的增加量

    //当前的记录 （运行时的临时变量，有些特殊控制，需要临时设定/更改）
    uint16_t current_control_val; // 当前阀控制值
    uint16_t current_clk; // 当前的计数
    uint16_t target_clk;  // 目标计数
    uint16_t current_work_status;
} ST_PWM_PORT_CONTROL;

ST_PWM_PORT_CONTROL pwm_control[4];
ST_PWM_PORT_CONTROL *actived_pwm;


void pwm_reset(int chanel)
{
    ST_PWM_PORT_CONTROL *pwm_ctrl;
    switch(chanel)
    {
    case 1: // chanel 1
        pwm_ctrl         = &pwm_control[0];
        pwm_ctrl->chanel = TIM_CHANNEL_1;
        break;
    case 2: // chanel 2
        pwm_ctrl         = &pwm_control[1];
        pwm_ctrl->chanel = TIM_CHANNEL_2;
        break;
    case 3: // chanel 3
        pwm_ctrl         = &pwm_control[2];
        pwm_ctrl->chanel = TIM_CHANNEL_3;
        break;
    case 4: // chanel 4
        pwm_ctrl         = &pwm_control[3];
        pwm_ctrl->chanel = TIM_CHANNEL_4;
        break;

    }

    pwm_ctrl->actived_flag        = EM_INACTIVED;
    pwm_ctrl->mode                = ENUM_PWM_IDLE_MODE;
    pwm_ctrl->cycles              = 0;
    pwm_ctrl->set_cycles          = 0;
    pwm_ctrl->inc_step_val        = 0;
    pwm_ctrl->control_a_time      = 0;
    pwm_ctrl->control_a_val       = 0;
    pwm_ctrl->control_b_time      = 0;
    pwm_ctrl->control_b_val       = 0;
    pwm_ctrl->control_c_time      = 0;
    pwm_ctrl->control_c_val       = 0;
    pwm_ctrl->control_d_time      = 0;
    pwm_ctrl->control_d_val       = 0;
    pwm_ctrl->current_control_val = 0; // 当前阀控制值
    pwm_ctrl->current_clk         = 0; // 当前的计数
    pwm_ctrl->target_clk          = 0;  // 目标计数
    pwm_ctrl->current_work_status = EM_PWM_READY;

    // set pwm port
    PWM_change_val(pwm_ctrl->chanel, 0);
    PWM_start(pwm_ctrl->chanel);
}
//

static ST_PWM_PORT_CONTROL *get_pwm_actived_ptr(uint8_t chanel)
{
    ST_PWM_PORT_CONTROL *pwm_ptr;

    switch(chanel)
    {
    case 1: // chanel 1
        pwm_ptr = &pwm_control[0];
        break;
    case 2: // chanel 2
        pwm_ptr = &pwm_control[1];
        break;
    case 3: // chanel 3
        pwm_ptr = &pwm_control[2];
        break;
    case 4: // chanel 4
        pwm_ptr = &pwm_control[3];
        break;
    default:
        pwm_ptr = (ST_PWM_PORT_CONTROL *) 0;
        break; // error
    }
    return pwm_ptr;
}

static void set_actived_pwm(uint8_t chanel)
{
    actived_pwm = get_pwm_actived_ptr(chanel);
}

void start_loop_mode_status(ST_PWM_PORT_CONTROL *pwm_ctrl)
{
    switch(pwm_ctrl->cycles)
    {
    default:
    case EM_LOOP_STEP_A: // low level keep mode
LABEL_LOOP_NEXT:
        if(pwm_ctrl->control_a_time != 0)
        {
            pwm_ctrl->target_clk          = pwm_ctrl->control_a_time;
            pwm_ctrl->current_control_val = pwm_ctrl->control_a_val;
            // set pwm val
            set_PWMx_percent(pwm_ctrl->chanel, pwm_ctrl->current_control_val);
            pwm_ctrl->current_clk = 0;
            break;
        }
        else
        {
            // time is 0, direct to next loop
        }
    case EM_LOOP_STEP_B: // wave up method
        if(pwm_ctrl->control_b_time != 0)
        {
            pwm_ctrl->target_clk  = pwm_ctrl->control_b_time;
            pwm_ctrl->current_clk = 0;
            break;
        }
        else
        {
            // time is 0, direct to next loop
        }
    case EM_LOOP_STEP_C: // high level keep mode
        if(pwm_ctrl->control_c_time != 0)
        {
            pwm_ctrl->target_clk          = pwm_ctrl->control_c_time;
            pwm_ctrl->current_control_val = pwm_ctrl->control_c_val;
            set_PWMx_percent(pwm_ctrl->chanel, pwm_ctrl->current_control_val);
            pwm_ctrl->current_clk = 0;
            // set pwm val
            break;
        }
        else
        {
            // time is 0, direct to next loop
        }
    case EM_LOOP_STEP_D: // down mode
        if(pwm_ctrl->control_d_time != 0)
        {
            pwm_ctrl->target_clk  = pwm_ctrl->control_d_time;
            pwm_ctrl->current_clk = 0;
            break;
        }
        else
        {
            goto LABEL_LOOP_NEXT;
            // time is 0, direct to next loop
        }
    }
}


void do_pwm_loop_actions(ST_PWM_PORT_CONTROL *pwm_ctrl)
{
    if(pwm_ctrl->actived_flag == EM_INACTIVED) return;
    switch(pwm_ctrl->current_work_status)
    {
    case EM_PWM_READY:// 如果没工作就直接返回
        // do nothing
        break;
    case EM_PWM_RUNNING:
        switch(pwm_ctrl->cycles)
        {
        default:
        case EM_LOOP_STEP_A: // low level keep mode
            (pwm_ctrl->current_clk) ++;
            if(pwm_ctrl->current_clk >= pwm_ctrl->target_clk)
            {
                pwm_ctrl->cycles = EM_LOOP_STEP_B;
                start_loop_mode_status(pwm_ctrl);
            }
            break;

        case EM_LOOP_STEP_B: // wave up method
            (pwm_ctrl->current_clk) ++;
            // increase control value
            //                    // TBD
            //                    pwm_ctrl->current_control_val += pwm_ctrl->control_b_val;
            //                    if(pwm_ctrl->current_control_val > pwm_ctrl->control_c_val)
            //                    {
            //                        pwm_ctrl->current_control_val = pwm_ctrl->control_c_val;
            //                    }
            //                    set_PWMx_percent(pwm_ctrl->chanel, pwm_ctrl->current_control_val);
            if(pwm_ctrl->current_clk >= pwm_ctrl->target_clk)
            {
                pwm_ctrl->cycles = EM_LOOP_STEP_C;
                start_loop_mode_status(pwm_ctrl);
            }
            break;
        case EM_LOOP_STEP_C: // high level keep mode
            (pwm_ctrl->current_clk) ++;
            if(pwm_ctrl->current_clk >= pwm_ctrl->target_clk)
            {
                pwm_ctrl->cycles = EM_LOOP_STEP_D;
                start_loop_mode_status(pwm_ctrl);
            }
            break;

        case EM_LOOP_STEP_D: // down mode
            (pwm_ctrl->current_clk) ++;
            // decrease control value
            // TBD
            //
            //                    if(pwm_ctrl->current_control_val > pwm_ctrl->control_b_val)
            //                    {
            //                        pwm_ctrl->current_control_val -= pwm_ctrl->control_b_val;
            //                    }
            //                    else
            //                    {
            //                        pwm_ctrl->current_control_val = 0;
            //                    }
            //                    set_PWMx_percent(pwm_ctrl->chanel, pwm_ctrl->current_control_val);
            if(pwm_ctrl->current_clk >= pwm_ctrl->target_clk)
            {
                pwm_ctrl->cycles = EM_LOOP_STEP_A;
                start_loop_mode_status(pwm_ctrl);
            }
            break;
        }
        break;
    case EM_PWM_PAUSE:// 如果处于PAUSE状态就返回
        return;       // do nothing
    case EM_PWM_STOP: // 如果处于STOP状态就返回
        return;
    default:
        break;
    }

}
void wave_start(uint8_t chanel, uint32_t d1, uint32_t d2, uint16_t t1, uint16_t t2 )
{
    uint32_t tmp_val;
    uint32_t base;
    ST_PWM_PORT_CONTROL *pwm_ctrl;
    set_actived_pwm(chanel);
    pwm_ctrl = get_pwm_actived_ptr(chanel);


    if(t1 + t2 == 0)
    {
        if(pwm_ctrl->actived_flag == EM_ACTIVED)
        {
            pwm_reset(chanel);
        }
        return;
    }

    base = d1;
    if(base > 999) base = 999;

    tmp_val                  = base;
    pwm_ctrl->control_a_val  = tmp_val;
    pwm_ctrl->control_a_time = t1;

    pwm_ctrl->control_b_val  = tmp_val;
    pwm_ctrl->control_b_time = 0;


    base = d2;
    if(base > 999) base      = 999;

    tmp_val                  = base;
    pwm_ctrl->control_c_val  = tmp_val;
    pwm_ctrl->control_c_time = t2;

    pwm_ctrl->control_d_val  = tmp_val;
    pwm_ctrl->control_d_time = 0;

    pwm_ctrl->cycles         = EM_LOOP_STEP_A;
    pwm_ctrl->actived_flag   = 1;
    pwm_ctrl->mode           = ENUM_PWM_LOOP_MODE;
    start_loop_mode_status(pwm_ctrl);
    pwm_ctrl->current_work_status = EM_PWM_RUNNING;
    PWM_start(pwm_ctrl->chanel);
    // 显示相关
    display_flag = 1;
}





void do_pwm_step_actions(ST_PWM_PORT_CONTROL *pwm_ctrl)
{
    switch(pwm_ctrl->current_work_status)
    {
    case EM_PWM_READY:// 如果没工作就直接返回
        // do nothing
        break;
    case EM_PWM_RUNNING:
        // 计数++
        pwm_ctrl->current_clk ++;
        // 判定是否延时到期
        if(pwm_ctrl->current_clk >= pwm_ctrl->target_clk)
        {
            // 如果延时到，则进到下一阶段

            // 下一阶段 重置延时
            pwm_ctrl->current_clk = 0;
            if(pwm_ctrl->mode == ENUM_PWM_STEP_MODE)
            {
                // 增加控制量
                pwm_ctrl->current_control_val += pwm_ctrl->inc_step_val;
                // 控制实现
                set_PWMx_percent(pwm_ctrl->chanel, pwm_ctrl->current_control_val);
                // 判定是否结束
                if(pwm_ctrl->current_control_val > 1000)
                {
                    pwm_ctrl->current_control_val = 1000;
                    // 结束
                    pwm_ctrl->current_work_status = EM_PWM_STOP;
                }
            }
            else //if(pwm_ctrl->mode == ENUM_PWM_DOWN_MODE)
            {
                // 减少控制量
                if(pwm_ctrl->current_control_val > pwm_ctrl->inc_step_val)
                {
                    pwm_ctrl->current_control_val -= pwm_ctrl->inc_step_val;
                }
                else
                {
                    pwm_ctrl->current_control_val = 0;
                    // 结束
                    pwm_ctrl->current_work_status = EM_PWM_STOP;
                }
                // 控制实现
                set_PWMx_percent(pwm_ctrl->chanel, pwm_ctrl->current_control_val);
            }
            // 判定是否结束
            if((pwm_ctrl->cycles) < (pwm_ctrl->set_cycles))
            {
                (pwm_ctrl->cycles) ++;
            }
            else
            {
                // 结束
                pwm_ctrl->current_work_status = EM_PWM_STOP;
            }
        }
        break;
    case EM_PWM_PAUSE:// 如果处于PAUSE状态就返回
        return;  // do nothing
    case EM_PWM_STOP:   // 如果处于STOP状态就返回
        return;
    default:
        break;
    }
}

void Blower_enable(void);
void Blower_disable(void);
extern unsigned int display_flag;
void start_pwm_step_test(uint8_t chanel, uint8_t delay, uint16_t data_inc)
{
    ST_PWM_PORT_CONTROL *pwm_ctrl;
    uint16_t tmp;
    if(chanel == 2)
    {
        Blower_enable();
    }
    set_actived_pwm(chanel);
    pwm_ctrl                      = get_pwm_actived_ptr(chanel);
    pwm_ctrl->actived_flag        = EM_ACTIVED;

    // 设定延时
    pwm_ctrl->target_clk          = delay * 1000;

    // 计算 cycles
    tmp                           = 1000 / data_inc + 1;
    pwm_ctrl->set_cycles          = tmp;
    pwm_ctrl->inc_step_val        = data_inc;
    pwm_ctrl->cycles              = 0;
    pwm_ctrl->current_control_val = 0;
    // 设置初值
    PWM_change_val(pwm_ctrl->chanel, 0);

    // start
    pwm_ctrl->mode                = ENUM_PWM_STEP_MODE;
    pwm_ctrl->current_work_status = EM_PWM_RUNNING;
    // 显示相关
    display_flag = 1;
}

void reverse_pwm_test(uint8_t chanel)
{
    ST_PWM_PORT_CONTROL *pwm_ctrl;

    set_actived_pwm(chanel);
    pwm_ctrl = get_pwm_actived_ptr(chanel);
    if(pwm_ctrl->mode == ENUM_PWM_DOWN_MODE)
    {
        pwm_ctrl->mode = ENUM_PWM_STEP_MODE;
    }
    else
    {
        pwm_ctrl->mode = ENUM_PWM_DOWN_MODE;
    }
}
void start_pwm_down_test(uint8_t chanel, uint8_t delay, uint16_t data_dec)
{
    ST_PWM_PORT_CONTROL *pwm_ctrl;
    uint16_t tmp;
    if(chanel == 2)
    {
        Blower_enable();
    }
    set_actived_pwm(chanel);
    pwm_ctrl                      = get_pwm_actived_ptr(chanel);
    pwm_ctrl->actived_flag        = EM_ACTIVED;

    // 设定延时
    pwm_ctrl->target_clk          = delay * 1000;

    // 计算 cycles
    tmp                           = 1000 / data_dec + 1;
    pwm_ctrl->set_cycles          = tmp;
    pwm_ctrl->inc_step_val        = data_dec;
    pwm_ctrl->cycles              = 0;
    pwm_ctrl->current_control_val = 1000;
    // 设置初值
    set_PWMx_percent(pwm_ctrl->chanel, pwm_ctrl->current_control_val);

    // start
    pwm_ctrl->mode                = ENUM_PWM_DOWN_MODE;
    pwm_ctrl->current_work_status = EM_PWM_RUNNING;
    // 显示相关
    display_flag                  = 1;
}

void restart_pwm_step_test(ST_PWM_PORT_CONTROL *pwm_ctrl)
{
    int i;

    pwm_ctrl->actived_flag        = EM_INACTIVED;
    pwm_ctrl->mode                = ENUM_PWM_IDLE_MODE;
    pwm_ctrl->cycles              = 0;
    pwm_ctrl->set_cycles          = 0;
    pwm_ctrl->inc_step_val        = 0;
    pwm_ctrl->control_a_time      = 0;
    pwm_ctrl->control_a_val       = 0;
    pwm_ctrl->control_b_time      = 0;
    pwm_ctrl->control_b_val       = 0;
    pwm_ctrl->control_c_time      = 0;
    pwm_ctrl->control_c_val       = 0;
    pwm_ctrl->control_d_time      = 0;
    pwm_ctrl->control_d_val       = 0;
    pwm_ctrl->current_control_val = 0; // 当前阀控制值
    pwm_ctrl->current_clk         = 0; // 当前的计数
    pwm_ctrl->target_clk          = 0;  // 目标计数
    pwm_ctrl->current_work_status = EM_PWM_READY;

    // set pwm port
    PWM_change_val(pwm_ctrl->chanel, 0);


    // check diaplay flag
    for(i = 0; i < 4; i++)
    {
        if(pwm_control[i].current_work_status == EM_PWM_RUNNING)
        {
            return;
        }
    }
    display_flag = 0; // disable display

}

/*
    uint8_t chanel, 通道
 */
void stop_pwm_step_test(uint8_t chanel)
{
    int i;
    if(chanel == 2)
    {
        Blower_disable();
    }

    pwm_reset(chanel);

    // check diaplay flag
    for(i = 0; i < 4; i++)
    {
        if(pwm_control[i].current_work_status == EM_PWM_RUNNING)
        {
            return;
        }
    }
    display_flag = 0; // disable display
}

void continue_pwm_step_test(uint8_t chanel)
{
    ST_PWM_PORT_CONTROL *pwm_ctrl;

    pwm_ctrl = get_pwm_actived_ptr(chanel);

    if(pwm_ctrl->current_work_status == EM_PWM_PAUSE)
    {
        set_actived_pwm(chanel);
        pwm_ctrl->current_work_status = EM_PWM_RUNNING;
    }
}

void pause_pwm_step_test(uint8_t chanel)
{
    ST_PWM_PORT_CONTROL *pwm_ctrl;
    pwm_ctrl = get_pwm_actived_ptr(chanel);

    if(pwm_ctrl->current_work_status == EM_PWM_RUNNING)
    {
        set_actived_pwm(chanel);
        pwm_ctrl->current_work_status = EM_PWM_PAUSE;
    }
}


extern uint32_t test_val;

uint8_t is_pwm_chanel_actived(uint8_t chanel)
{
    test_val = (uint32_t) chanel;

    return (pwm_control[chanel].current_work_status != EM_PWM_READY) ? 1 : 0;
}

uint16_t get_pwm_control_val(uint8_t chanel)
{
    return pwm_control[chanel].current_control_val;
}




void do_pwm_actions(uint8_t mode, ST_PWM_PORT_CONTROL *pwm_ctrl)
{
    switch(mode)
    {
    case ENUM_PWM_LOOP_MODE: // up--> top -> keep --> down --> keep --> up ...
        do_pwm_loop_actions(pwm_ctrl);
        break;
    case ENUM_PWM_STEP_MODE: // step up --> delay(keep) ---step up ---> delay --> top (stop)
        do_pwm_step_actions(pwm_ctrl);
        break;
    case ENUM_PWM_DOWN_MODE: // step up --> delay(keep) ---step up ---> delay --> top (stop)
        do_pwm_step_actions(pwm_ctrl);
        break;
    case ENUM_PWM_IDLE_MODE:
        break;
    default:
        // du nothiong
        break;
    }

}

void check_pwm_key(ST_PWM_PORT_CONTROL *pwm_ctrl, uint8_t current_key)
{
    switch(current_key)
    {
    case PWM_PAUSE_KEY:
        if(actived_pwm->current_work_status == EM_PWM_RUNNING)
        {
            actived_pwm->current_work_status = EM_PWM_PAUSE;
        }
        break;
    case PWM_CONTINUE_KEY:
        if(actived_pwm->current_work_status == EM_PWM_PAUSE)
        {
            actived_pwm->current_work_status = EM_PWM_RUNNING;
        }
        break;
    case PWM_RESET_KEY:
        if(actived_pwm->current_work_status != EM_PWM_READY)
        {
            //actived_pwm->current_work_status = EM_PWM_READY;
            restart_pwm_step_test(actived_pwm);
        }
        break;
    case PWM_TOGGLE_DISP_KEY:
        if(display_flag == 1) display_flag = 0;
        else                  display_flag = 1;
        break;
    default:
        break;
    }
}

void reset_all_pwm(void)
{
    int i;
    for(i = 1; i <= 4; i++)
    {
        pwm_reset(i);
    }
}

void pwm_test(void)
{
    uint8_t i;
    uint8_t pwm_key;
    ST_PWM_PORT_CONTROL *pwm_ctrl;

    pwm_key = get_key();
    for(i = 0; i < 4; i++)
    {
        // 定位
        pwm_ctrl = &pwm_control[i];
        // 先处理按键
        check_pwm_key(pwm_ctrl, pwm_key);
        // 进行PWM控制

        do_pwm_actions(pwm_control[i].mode, &pwm_control[i]);
    }
}
//File pwm_test.c end

