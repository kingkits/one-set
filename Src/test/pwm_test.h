// File pwm_test.h
#ifndef __PWM_TEST_H
#define __PWM_TEST_H
#include "stm32f4xx_hal.h"
#include "../user_interface/kbd.h"
#define PWM_PAUSE_KEY USER_KEY1
#define PWM_CONTINUE_KEY USER_KEY2
#define PWM_RESET_KEY USER_KEY4
#define PWM_TOGGLE_DISP_KEY USER_KEY3

/*
    uint8_t chanel, 通道
    uint8_t delay,  延时
    uint16_t data_inc 步进 增加量（千分比）
 */
void start_pwm_step_test(uint8_t chanel, uint8_t delay, uint16_t data_inc);
uint8_t is_pwm_chanel_actived(uint8_t chanel);
uint16_t get_pwm_control_val(uint8_t chanel);

void pwm_test(void);


#endif // *ifndef __PWM_TEST_H
// File pwm_test.h end

