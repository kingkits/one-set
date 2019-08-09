//File: timer.h
#include "stm32f4xx_hal.h"
#include "cmsis_os.h"
#include "stm32f4xx_it.h"

#ifndef __TIMER_H
#define __TIMER_H

void PWM_start(uint32_t Channel);
void PWM_change_val(uint32_t chanel, uint32_t val);
void delay_us(uint32_t n_us);
void set_PWM_percent(uint32_t val);
#endif

