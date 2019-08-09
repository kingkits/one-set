//File: Breath_Control_Task.c
#include "stm32f4xx_hal.h"
#include "cmsis_os.h"
#include "../global.h"
#include "../ControlTask/base_control.h"


void init_breath_AI_data(void);
__weak void breath_AI_namager(void);
void init_display_press_flow_MA_buf(void);

void initialize_base_task(void)
{
    init_global_data();
    init_patient_data();
    set_patient_breath_mode(EM_VENTLATOR_STANDBY_MODE);
    reset_display_data();
    reset_display_temp_data();
#if USE_ADC_RAM_DATA
#else
    init_vacuum_pump_control_data();
    init_peep_valve_control_data();
#endif
    init_display_press_flow_MA_buf();


    // 自动控制
    init_breath_AI_data();
}
#define get_1ms_count() ms_1_count

/* Base_Task function */
void Base_Task(void const *argument)
{

    /* USER CODE BEGIN 5 */
    initialize_base_task();
    /* Infinite loop */
    for(;;)
    {
        get_1ms_count() = osKernelSysTick();
        // 自动控制
        //breath_AI_namager();

        // 数据计算
        calculate_breath_data();

        // 状态监测
        patient_status_monitor();

        // 紧急状态处理

        // 报警

        // 模式控制
        breath_mode_control();

        osDelay(2);
    }
    /* USER CODE END 5 */
}

