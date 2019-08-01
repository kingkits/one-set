// File alpha_beta_filter.c
#include "alpha_beta_filter.h"
//double dt = 0.5;
//double a = 0.85, b = 0.005;
// 0 < α < 1
// 0 < β ≤ 2
// 0 < 4 ? 2 α ? β
#define ALPHA_BETA_FILTER_A      0.9
#define ALPHA_BETA_FILTER_B      0.2
#define ALPHA_BETA_FILTER_DT     0.005
#define ALPHA_BETA_FILTER_DT_REV 200.0 // 必须是上面的倒数 = 1.0 / ALPHA_BETA_FILTER_DT

void alpha_beta_filter_clear_data(ST_ALPHA_BETA_FILTER_DATA *data_ptr)
{
    data_ptr->vk   = 0.0;
    data_ptr->vk_1 = 0.0;
    data_ptr->xk   = 0.0;
    data_ptr->xk_1 = 0.0;
}


int alpha_beta_filter(int new_val, ST_ALPHA_BETA_FILTER_DATA *ab_filter_ptr)
{
    double rk;
    ab_filter_ptr->xk   = ab_filter_ptr->xk_1 + ( ab_filter_ptr->vk_1 * ALPHA_BETA_FILTER_DT );
    ab_filter_ptr->vk   = ab_filter_ptr->vk_1;

    rk = (double)new_val - ab_filter_ptr->xk;

    ab_filter_ptr->xk  += ALPHA_BETA_FILTER_A * rk;
    ab_filter_ptr->vk  += ( ALPHA_BETA_FILTER_B * rk ) * ALPHA_BETA_FILTER_DT_REV; // ALPHA_BETA_FILTER_DT;

    ab_filter_ptr->xk_1 = ab_filter_ptr->xk;
    ab_filter_ptr->vk_1 = ab_filter_ptr->vk;
    return (int) ab_filter_ptr->xk_1;
}

