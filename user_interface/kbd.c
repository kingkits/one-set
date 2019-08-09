#include "../GPIO/gpio.h"
#include "kbd.h"
static  unsigned char key;
static  unsigned char new_key_pushed_flag;
static  unsigned char key_pushed_flag;
static  uint16_t key_pushed_time;

static unsigned char key_current;
static unsigned char key_old;
static volatile unsigned char key_cnt;


uint16_t get_switch_data(void)
{
    uint16_t port_val;
    port_val = 0;
    port_val |= ((SW01_GPIO_Port->IDR) & SW01_Pin) >> 3;  // bit 0
    port_val |= ((SW02_GPIO_Port->IDR) & SW02_Pin) >> 12; // bit 1
    port_val |= ((SW03_GPIO_Port->IDR) & SW03_Pin) >> 10; // bit 2
    port_val |= ((SW04_GPIO_Port->IDR) & SW04_Pin) >>  8; // bit 3
    port_val |= ((SW05_GPIO_Port->IDR) & SW05_Pin) >>  6; // bit 1
    port_val |= ((SW06_GPIO_Port->IDR) & SW06_Pin) >> 10; // bit 5
    return port_val;
}

void init_key_data(void)
{
    key = 0;
    key_pushed_flag = 0;
    key_pushed_time = 0;

    key_current = 0;
    key_old = 0;
    key_cnt = 0;
}

// note: one pushed key only one time
uint8_t get_key(void)
{
    uint8_t key_tmp;
    if(new_key_pushed_flag)
    {
        new_key_pushed_flag = 0;
        key_tmp = key;
        key = 0;
        return key_tmp;
    }
    return 0;
}

uint16_t get_key_pushed_time(void)
{
    return key_pushed_time;
}

uint8_t is_key_pushed(void)
{
    return key_pushed_flag;
}
void check_key(void)
{
    uint32_t key_data;
    key_data = GPIOK->IDR;
    key_data >>= 4;

    key_data = (uint8_t)(key_data & 0xf);
    key_data ^= 0xf;
    if(key_data == 0)
    {
        key_pushed_flag = 0;
        key_pushed_time = 0;
    }
    else
    {
        key_pushed_flag = 1;
    }
    key_current = key_data & (key_data ^ key_cnt);
    key_cnt = key_data;
    if(key_current == 0)
    {
        key_old = 0;
        key_pushed_time = 0;
        return;
    }

    if(key_current == key_old)
    {
        if(key_pushed_time < 0xfe)
        {
            key_pushed_time ++;
        }
    }
    else
    {
        new_key_pushed_flag = 1;
        key_old = key_current;
        key = key_current;
    }
}
// File end
