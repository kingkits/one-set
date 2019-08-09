//File: kbd.h
#include "stm32f4xx_hal.h"
#include "cmsis_os.h"
#include "stm32f4xx_it.h"

#ifndef __KBD_H
#define __KBD_H
typedef enum _ENUM_USER_KEY_TYPES_DEFINED
{
    NO_ANY_KEY,             //0b0000
    USER_KEY1,              //0b0001
    USER_KEY2,              //0b0010
    USER_KEY1_2,            //0b0011
    USER_KEY3,              //0b0100
    USER_KEY1_3,            //0b0101
    USER_KEY2_3,            //0b0110
    USER_KEY1_2_3,          //0b0111
    USER_KEY4,              //0b1000
    USER_KEY1_4,            //0b1001
    USER_KEY2_4,            //0b1010
    USER_KEY1_2_4,          //0b1011
    USER_KEY3_4,            //0b1100
    USER_KEY1_3_4,          //0b1101
    USER_KEY2_3_4,          //0b1110
    USER_KEY1_2_3_4,        //0b1111
} ENUM_USER_KEY_TYPES_DEFINED;


void init_key_data(void);
uint8_t get_key(void);// note: one pushed key only one time
uint16_t get_key_pushed_time(void);
uint8_t is_key_pushed(void);
void check_key(void);
uint16_t get_switch_data(void);
#endif

