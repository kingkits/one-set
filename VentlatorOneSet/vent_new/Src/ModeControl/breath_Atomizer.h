// file:breath_Atomizer.h
#ifndef __BREATH_NEBULIZER_H
#define __BREATH_NEBULIZER_H

#include "stm32f4xx_hal.h"

void breath_turn_on_nebulizer(void);
void breath_trun_off_nebulizer(void);
void breath_force_operation_nebulizer(uint8_t status);
uint8_t is_nebulizer_valve_open(void);

#endif
//end of file:Atomizer.h

