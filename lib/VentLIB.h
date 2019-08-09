#ifndef __VENTLIB_H
#define __VENTLIB_H
// if tm1 > tm2, timer overflow solution
unsigned char system_direct_compare_tm1_bg_tm2(unsigned int tm1, unsigned int tm2);
// if tm1 >= tm2, timer overflow solution
unsigned char system_direct_compare_tm1_be_tm2(unsigned int tm1, unsigned int tm2);
#endif

