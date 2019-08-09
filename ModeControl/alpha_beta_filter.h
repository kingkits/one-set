// File alpha_beta_filter.h
#ifndef __ALPHA_BETA_FILTER_H
#define __ALPHA_BETA_FILTER_H
typedef struct __ST_ALPHA_BETA_FILTER_DATA
{
    double vk;
    double vk_1;
    double xk;
    double xk_1;
} ST_ALPHA_BETA_FILTER_DATA;
extern ST_ALPHA_BETA_FILTER_DATA alpha_beta_filter_flow;
extern ST_ALPHA_BETA_FILTER_DATA alpha_beta_filter_press;


void alpha_beta_filter_clear_data(ST_ALPHA_BETA_FILTER_DATA *data_ptr);
int  alpha_beta_filter(int new_val, ST_ALPHA_BETA_FILTER_DATA *ab_filter_ptr);
#endif
