// File pwm_wave.c
// 作者： 孟群恒
#include "breath_rap_dma_control.h"
#include "stm32f4xx_hal.h"
#include <math.h>
#include "../global.h"
#include "breath_rap.h"
//张志新：这个数取最大可能用到的那个大小
#define MAX_DMA_NUMBER_FOR_RAP 640


//extern DMA_HandleTypeDef  hdma_tim8_ch4_trig_com;      //DMA句柄
extern TIM_HandleTypeDef htim8;

static double amplitude = 0.35;

DMARAM uint16_t wave_Table[MAX_DMA_NUMBER_FOR_RAP];

//张志新： 数据生成要分成多个函数，每个函数只生成一种波形数据，使用（上面）相同的数据区
// 需要形参，不要用全局变量，名称需要带rap_dma前缀如rap_dma_create_sin_wave_data
void rap_dma_create_square_wave_data(uint16_t num_point)
{
    uint16_t i;

    for(i = 0; i < num_point / 2; i++)
    {
        wave_Table[i] = 200;
    }
    for(i = num_point / 2; i < num_point; i++)
    {
        wave_Table[i] = 2000;
    }
}

__weak void rap_creat_wave_data(uint16_t *buf, uint16_t points, uint16_t rap_press_level, uint16_t rap_freq)
{
}

void rap_dma_create_sin_wave_data(uint16_t num_point)
{
    double temp;

    uint16_t i, j;
    //产生正弦波数据
    for(i = 0; i < num_point; i++)
    {
        //张志新： 这一行最好分解成多行，每一行只做一个算法，你可以考虑再定义一个临时变量
        //不同数据类型的操作，要显示的强制转换
        // 比如 Sin_Table[i] = (uint16_t) temp;
        //（要养成习惯）后面类似的操作与此行相同
        //temp = amplitude * 3999 * (1 + sin((double)(2.0 * freq_wave * PI * (1.0 / (freq_wave * num_point) * (i + 1))))) + 100;
        j = i + 1;
        temp = 1.0 / num_point;
        temp *= j;
        temp = temp * PI;
        temp *= 2;
        temp = sin(temp) + 1;
        temp *= 3999;
        temp *= amplitude;
        wave_Table[i] = (uint16_t) temp + 100;
    }
}

void rap_dma_create_triangle_wave_data(uint16_t num_point)
{
    double temp;

    uint16_t i, j, n = 1;
    uint16_t half_num_point = num_point / 2;
    //产生三角波数据
    for(i = 0; i < num_point; i++)
    {
        if(i < half_num_point) //if(i < num_point / 2)
        {
            //temp = amplitude * 3999 * (1.0 / (freq_wave * num_point) * (i + 1)) * 1.732051 * freq_wave;
            j = i + 1;
            /*
               temp = 1.0 / num_point;
               temp *= j;
               temp *= 1.732051;
               temp *= 3999;
             */
            temp = (double) j / (double)num_point;
            temp *= 6928.203230; // = 1.732051 * 4000

            temp *= amplitude;
            wave_Table[i] = (uint16_t) temp;
        }
        else
        {
            wave_Table[i] = wave_Table[half_num_point - n];
            n += 1;
        }
    }
}

void rap_dma_create_trapezoid_wave_data(uint16_t num_point)
{
    double temp;

    uint16_t i, j, n = 1;
    uint16_t point_a = num_point / 20 * 9, point_b = num_point / 20 * 11;
    //产生梯形波数据
    for(i = 0; i < point_a; i++)
    {
        //temp = amplitude * 3999 * (1.0 / (freq_wave * num_point) * (i + 1)) * 1.732051 * freq_wave;
        j = i + 1;
        temp = 1.0 / num_point;
        temp *= j;
        temp *= 1.732051;
        temp *= 3999;
        temp *= amplitude;
        wave_Table[i] = (uint16_t) temp;
    }

    for(i = point_a; i < point_b; i++)
    {
        wave_Table[i] = wave_Table[point_a - 1];
    }

    for(i = point_b; i < num_point; i++)
    {
        wave_Table[i] = wave_Table[point_a - n];
        n += 1;
    }
}

//DMAx的各通道配置
//这里的传输形式是固定的,这点要根据不同的情况来修改
//从存储器->外设模式/32位数据宽度/存储器增量模式
//DMA_Streamx:DMA数据流,DMA1_Stream0~7/DMA2_Stream0~7
//chx:DMA通道选择,@ref DMA_channel DMA_CHANNEL_0~DMA_CHANNEL_7
void rap_dma_config(uint16_t num_point)
{
}

void rap_dma_start(void)
{
    htim8.Instance->DIER |= TIM_DMA_CC4;//使能DMA发送
}

void rap_dma_stop(void)
{
    htim8.Instance->DIER = (TIM_DMA_CC4 & 0xEFFF); //禁止DMA发送
}

/***********************张工提供的接口函数***********************************************/
// 返回值： = 0 禁止叩击  = 1 叩击
// uint8_t is_rap_enable(void)；//不需要你定义，你需要读取状态确定是否要叩击


// 重新设置叩击参数 press = 控制压力， freq = 频率， re_wave_type = 波形类型
void refresh_rap_control_data(uint16_t press, uint8_t re_freq, uint8_t re_wave_type)
{
    uint16_t num_point = 3160 / re_freq;
    switch (press / 10)
    {
    case 1:
        amplitude = 0.1;
        break;
    case 2:
    case 3:
        amplitude = 0.2;
        break;
    case 4:
    case 5:
    case 6:
        amplitude = 0.3;
        break;
    case 7:
    case 8:
    case 9:
        amplitude = 0.4;
        break;
    case 10:
        amplitude = 0.5;
        break;
    default:
        amplitude = 0.6;
        break;
    }
    HAL_TIM_PWM_Stop_DMA(&htim8, TIM_CHANNEL_4);
    switch(re_wave_type)
    {
    case RAP_DMA_SQUARE_WAVE:
        rap_dma_create_square_wave_data(num_point);
        break;
    case RAP_DMA_SIN_WAVE:
        rap_dma_create_sin_wave_data(num_point);
        break;
    case RAP_DMA_TRIANGLE_WAVE:
        rap_dma_create_triangle_wave_data(num_point);
        break;
    case RAP_DMA_TRAPEZOID_WAVE:
        rap_dma_create_trapezoid_wave_data(num_point);
        break;
    default:
        break;
    }
    if (HAL_TIM_PWM_Start_DMA(&htim8, TIM_CHANNEL_4, (uint32_t *)wave_Table, num_point) != HAL_OK)
    {
        /* Starting Error */
        Error_Handler();
    }
    htim8.Instance->DIER = TIM_DMA_CC4;//(TIM_DMA_CC4 & 0xEFFF); //禁止DMA发送
}

// 直接执行叩击动作的函数（如果是DMA操作，需要定义为空函数）
void do_breath_rap_actions(void)
{

}

// File pwm_wave.c code end
