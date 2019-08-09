#ifndef __PWM_WAVE_H
#define __PWM_WAVE_H
#include "stm32f4xx_hal.h"


//void create_wavedata(void);
void rap_dma_start(void);
void rap_dma_stop(void);
void refresh_rap_control_data(uint16_t press, uint8_t re_freq, uint8_t re_wave_type);//2019.01.14
void do_breath_rap_actions(void);
#endif


