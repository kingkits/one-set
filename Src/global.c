#include "stm32f4xx_hal.h"
#include "cmsis_os.h"

#define extern
#include "global.h"
CCMRAM uint8_t spi_cmd_buf[16];

uint16_t basic_VT_btps_and_pipe_compensation(uint16_t vt, int16_t dp)
{
    double dtmp;
    dtmp  = (double)vt;
    dtmp *= 1.0526;
    if(basic_control_parameters.air_type == EM_BTPS)
    {
        dtmp *= basic_control_parameters.btps_fact;
        //dtmp -= (double)basic_control_parameters.pipe_fact * (double)dp;
    }
    return (uint16_t) dtmp;
}


void refresh_btps_fact(void)
{
    double ftmp1, ftmp2;
    ftmp2 = (double)basic_control_parameters.ambient_pressure;
#if 0
    ftmp1 = 0.945161290;
    ftmp1 *= (ftmp2 - 627.75) / (ftmp2 - 470);
#else
    ftmp1 = 1.058020; //310.0/(273.0+20);
    ftmp1 *= (ftmp2 - 470) / (ftmp2 - 627.75);
#endif
    basic_control_parameters.btps_fact = (float) ftmp1;
}

// Code begin =============================>>>>
void init_basic_control_parameters(void)
{
    basic_control_parameters.ambient_pressure        = 10076.0;// 北京的平均大气压强   //10132.5;// 0.1 cmH2O

    basic_control_parameters.environment_temperiture = 22;

    basic_control_parameters.air_type                = EM_ATP;
#if INVASIVE_VENTLATION_SUPPORT
    basic_control_parameters.invasive_support        = 1;
#else
    basic_control_parameters.invasive_support        = 0;
#endif
    basic_control_parameters.patient_types           = EM_ADULT;
    basic_control_parameters.body_disease            = EM_DISEASE_NORMAL;

    basic_control_parameters.pipe_fact               = 0.1;// ml/cmH2O
    refresh_btps_fact();
}

void change_basic_ambient_pressure(uint16_t press)
{
    basic_control_parameters.ambient_pressure = (float32_t)press;
    refresh_btps_fact();
}

void init_global_data(void)
{
    second_count = 0;
    ms_1_count   = 0;

    init_basic_control_parameters();
}

void inc_1ms_count(void)
{
    ms_1_count++;
}

uint32_t get_second_count(void)
{
    return second_count;
}

uint32_t get_different_second(uint32_t old_time)
{
    return second_count - old_time;
}

// dummy function
__weak uint32_t get_1ms_count(void)
{
    return 0;
}

// end
