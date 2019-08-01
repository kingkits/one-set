//#include "stm32f4xx_hal.h"
//#include "cmsis_os.h"
#include "global.h"

extern TIM_HandleTypeDef htim7;
extern TIM_HandleTypeDef htim8;
extern TIM_HandleTypeDef htim6;
extern TIM_HandleTypeDef htim4;

#include "timer.h"
static volatile unsigned char timer7_counter = 0;
#define BLOWER_RPM_CHANEL TIM_CHANNEL_1
void set_blower_speed_s(uint16_t speed);
extern volatile uint32_t blower_count;


void start_timer6(void)
{
    HAL_TIM_Base_Start_IT((TIM_HandleTypeDef *) &htim6);
}
void stop_timer6(void)
{
    HAL_TIM_Base_Stop_IT((TIM_HandleTypeDef *) &htim6);
}

void start_timer4(void)
{
    HAL_TIM_IC_Start_IT((TIM_HandleTypeDef *) &htim4, TIM_CHANNEL_1);
}

void stop_timer4(void)
{
    HAL_TIM_IC_Stop_IT((TIM_HandleTypeDef *) &htim4, TIM_CHANNEL_1);
}



void timer6_int_handler(void)
{
    set_blower_speed_s(blower_count);
    blower_count = 0;
    second_count++;
    // 返回
}

void simple_time7_isr_service(void)
{
    __HAL_TIM_CLEAR_IT(&htim7, TIM_IT_UPDATE);
    timer7_counter++;
}

void start_timer7(void)
{
    if(timer7_counter == 0)
    {
        HAL_TIM_Base_Start_IT((TIM_HandleTypeDef *) &htim7);
    }
}

void stop_timer7(void)
{
    HAL_TIM_Base_Stop_IT((TIM_HandleTypeDef *) &htim7);
    timer7_counter = 0;
}

void delay_us(uint32_t n_us)
{
    unsigned char tmp;

    tmp = timer7_counter;
    start_timer7();
    while(n_us)
    {
        if(tmp != timer7_counter)
        {
            tmp = timer7_counter;
            n_us--;
        }
    }
}

void test_delay_us(void)
{
    int i;
    HAL_GPIO_TogglePin(LED1_GPIO_Port, LED1_Pin);
    for(i = 0; i < 10000; i++)
    {
        delay_us(50);
    }
}


/// PWM
void PWM_change_val(uint32_t chanel, uint32_t val)
{
    switch(chanel)
    {
    case TIM_CHANNEL_1:
        htim8.Instance->CCR1 = val;
        break;
    case TIM_CHANNEL_2:
        htim8.Instance->CCR2 = val;
        break;
    case TIM_CHANNEL_3:
        htim8.Instance->CCR3 = val;
        break;
    case TIM_CHANNEL_4:
        htim8.Instance->CCR4 = val;
        break;
    default:
        break;
    }
}

uint32_t get_pwm_scale(void)
{
    return (uint32_t)  htim8.Init.Period;
}

void set_PWM1_percent(uint32_t val)
{
    uint32_t tmp_val;
    uint32_t base = val;
    if(base >= 1000)
    {
        base = 1000;
    }
    else {};

    tmp_val = htim8.Init.Period;
    tmp_val *= base;
    tmp_val /= 1000;
    PWM_change_val(TIM_CHANNEL_1, tmp_val);
}
void set_PWMx_percent(uint32_t chanel, uint32_t val)
{
    uint32_t tmp_val;
    uint32_t base = val;
    if(base >= 1000)
    {
        PWM_change_val(chanel, htim8.Init.Period);
    }
    else {};

    tmp_val = htim8.Init.Period;
    tmp_val *= base;
    tmp_val /= 1000;
    PWM_change_val(chanel, tmp_val);
}

void PWM_start(uint32_t chanel)
{
    HAL_TIM_PWM_Start((TIM_HandleTypeDef *) &htim8, chanel);

    htim8.Instance->BDTR |= TIM_BDTR_MOE;
}

void PWM_stop(uint32_t chanel)
{
    HAL_TIM_PWM_Stop((TIM_HandleTypeDef *) &htim8, chanel);

    htim8.Instance->BDTR |= TIM_BDTR_MOE;

}
