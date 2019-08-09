#include "main.h"
#include "stm32f4xx_hal.h"
#include "cmsis_os.h"
#include "./user_interface/kbd.h"
#include "global.h"
extern TIM_HandleTypeDef htim8;

static uint8_t LED_error_code;

void    test_start(void);
void    test_period(void);
void    start_sdram(void);
uint8_t get_patient_breath_mode(void);
__weak void    test_start(void)
{
}
__weak void    test_period(void)
{
}
__weak uint8_t is_system_fault_active(void)
{
    return EM_INACTIVED;
}

static void close_buzzer(void)
{
    HAL_GPIO_WritePin(GPIOI, BUZZER_Pin, GPIO_PIN_SET);
}


static void close_all_LED(void)
{
    HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LED3_GPIO_Port, LED3_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LED4_GPIO_Port, LED4_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LED5_GPIO_Port, LED5_Pin, GPIO_PIN_SET);
}

void set_LED_error_code(uint8_t val)
{
    LED_error_code = val;
}
void set_Status_LED(void)
{
    // led1
    if(LED_error_code & 1)
    {
        HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET); // LED on
    }
    else
    {
        HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET); // LED off
    }

    // led2
    if(LED_error_code & 2)
    {
        HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_RESET); // LED on
    }
    else
    {
        HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_SET); // LED off
    }

    // led3
    if(LED_error_code & 4)
    {
        HAL_GPIO_WritePin(LED3_GPIO_Port, LED3_Pin, GPIO_PIN_RESET); // LED on
    }
    else
    {
        HAL_GPIO_WritePin(LED3_GPIO_Port, LED3_Pin, GPIO_PIN_SET); // LED off
    }

    // led4
    if(LED_error_code & 8)
    {
        HAL_GPIO_WritePin(LED4_GPIO_Port, LED4_Pin, GPIO_PIN_RESET); // LED on
    }
    else
    {
        HAL_GPIO_WritePin(LED4_GPIO_Port, LED4_Pin, GPIO_PIN_SET); // LED off
    }

    // led5
    if(LED_error_code & 0x10)
    {
        HAL_GPIO_WritePin(LED5_GPIO_Port, LED5_Pin, GPIO_PIN_RESET); // LED on
    }
    else
    {
        HAL_GPIO_WritePin(LED5_GPIO_Port, LED5_Pin, GPIO_PIN_SET); // LED off
    }
}

void Run_LED_status(void)
{
    static uint8_t count = 0;

    // 如果出现故障，则保持常亮
    if(is_system_fault_active())
    {
        HAL_GPIO_WritePin(RUN_LED_GPIO_Port, RUN_LED_Pin, GPIO_PIN_RESET);

        count = 4;

        // 由LED 显示错误代码
        set_Status_LED();
        return;
    }
    if(count == 4)
    {
        // 这里清除之前的错误信息
        set_Status_LED();
        count = 0;
    }
    // 如果是StandBy,则每2次调用反转一次
    if( !get_patient_breath_mode() )
    {
        if(count++ >= 2)
        {
            count = 0;
            HAL_GPIO_TogglePin(RUN_LED_GPIO_Port, RUN_LED_Pin);
        }
    }
    else // 否则 每次调用都反转一次
    {
        count = 0;
        HAL_GPIO_TogglePin(RUN_LED_GPIO_Port, RUN_LED_Pin);
    }
}

/* Main_Task function */
void Main_Task(void const *argument)
{
    uint16_t loop_count;
    loop_count = 0;
    /* USER CODE BEGIN Main_Task */
    /* Infinite loop */
    close_all_LED();
    close_buzzer();
    start_sdram();
    //test_start();
    for(;;)
    {
        test_period();

        // delay 500ms // led-toggle
        loop_count++;
        if(loop_count > 64)
        {
            loop_count = 0;
            Run_LED_status();
            //HAL_GPIO_TogglePin(LED5_GPIO_Port, LED5_Pin);
        }
        osDelay(2);
    }
    /* USER CODE END Main_Task */
}
