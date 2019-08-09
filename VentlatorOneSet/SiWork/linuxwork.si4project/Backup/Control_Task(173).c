// File: Control_Task.c
#include "../global.h"
//#include "stm32f4xx_hal.h"
//#include "cmsis_os.h"

#include "base_control.h"

//Bit:                                         FEDCBA9876543210
#define OXYGEN_ADJUST_FLAG      0x1          //0000000000000001  //
#define OXYGEN_PID_ACTIVED_FLAG 0x2          //0000000000000010  //
#define FLOW_ADJUST_OVERFLOW_VALVE_FLAG 0x4  //0000000000000100  // 回流阀调整标记
#define EPAP_ADJUST_FLAG 0x8                 //0000000000001000  // 呼气阀调整标记
// Total 64
const uint32_t CONTROL_SCH_ARRAY[] =
{
    //                              Bit:  FEDCBA9876543210
    OXYGEN_ADJUST_FLAG,                 //0000000000000001,     // 0
    0x0000,                             //0000000000000000,     // 1
    0x0000,                             //0000000000000000,     // 2
    0x0000,                             //0000000000000000,     // 3
    0x0000,                             //0000000000000000,     // 4
    0x0000,                             //0000000000000000,     // 5
    0x0000,                             //0000000000000000,     // 6
    0x0000,                             //0000000000000000,     // 7
    OXYGEN_ADJUST_FLAG,                 //0000000000000001,     // 8
    0x0000,                             //0000000000000000,     // 9
    //                              Bit:  FEDCBA9876543210
    0x0000,                             //0000000000000000,     // 0
    0x0000,                             //0000000000000000,     // 1
    0x0000,                             //0000000000000000,     // 2
    0x0000,                             //0000000000000000,     // 3
    0x0000,                             //0000000000000000,     // 4
    0x0000,                             //0000000000000000,     // 5
    OXYGEN_ADJUST_FLAG,                 //0000000000000001,     // 6
    0x0000,                             //0000000000000000,     // 7
    0x0000,                             //0000000000000000,     // 8
    0x0000,                             //0000000000000000,     // 9
    //                              Bit:  FEDCBA9876543210
    0x0000,                             //0000000000000000,     // 0
    0x0000,                             //0000000000000000,     // 1
    0x0000,                             //0000000000000000,     // 2
    0x0000,                             //0000000000000000,     // 3
    OXYGEN_ADJUST_FLAG,                 //0000000000000001,     // 4
    0x0000,                             //0000000000000000,     // 5
    0x0000,                             //0000000000000000,     // 6
    0x0000,                             //0000000000000000,     // 7
    0x0000,                             //0000000000000000,     // 8
    0x0000,                             //0000000000000000,     // 9
    //                              Bit:  FEDCBA9876543210
    0x0000,                             //0000000000000000,     // 0
    0x0000,                             //0000000000000000,     // 1
    OXYGEN_ADJUST_FLAG,                 //0000000000000001,     // 2
    0x0000,                             //0000000000000000,     // 3
    0x0000,                             //0000000000000000,     // 4
    0x0000,                             //0000000000000000,     // 5
    0x0000,                             //0000000000000000,     // 6
    0x0000,                             //0000000000000000,     // 7
    0x0000,                             //0000000000000000,     // 8
    0x0000,                             //0000000000000000,     // 9
    //                              Bit:  FEDCBA9876543210
    OXYGEN_ADJUST_FLAG,                 //0000000000000001,     // 0
    0x0000,                             //0000000000000000,     // 1
    0x0000,                             //0000000000000000,     // 2
    0x0000,                             //0000000000000000,     // 3
    0x0000,                             //0000000000000000,     // 4
    0x0000,                             //0000000000000000,     // 5
    0x0000,                             //0000000000000000,     // 6
    0x0000,                             //0000000000000000,     // 7
    OXYGEN_ADJUST_FLAG,                 //0000000000000001,     // 8
    0x0000,                             //0000000000000000,     // 9
    //                              Bit:  FEDCBA9876543210
    0x0000,                             //0000000000000000,     // 0
    0x0000,                             //0000000000000000,     // 1
    0x0000,                             //0000000000000000,     // 2
    0x0000,                             //0000000000000000,     // 3
    0x0000,                             //0000000000000000,     // 4
    0x0000,                             //0000000000000000,     // 5
    OXYGEN_ADJUST_FLAG,                 //0000000000000001,     // 6
    0x0000,                             //0000000000000000,     // 7
    EPAP_ADJUST_FLAG,                   //0000000000000000,     // 8
    0x0000,                             //0000000000000000,     // 9
    //                              Bit:  FEDCBA9876543210
    FLOW_ADJUST_OVERFLOW_VALVE_FLAG,    //0000000000000100,     // 0
    0x0000,                             //0000000000000000,     // 1
    0x0000,                             //0000000000000000,     // 2
    OXYGEN_PID_ACTIVED_FLAG,            //0000000000000010      // 3
};


void close_all_valve(void);
void reset_blower(void);
void oxygen_adjust(void);
void oxygen_pid_adjust(void);
void reset_oxygen_mixer(void);
void Blower_enable(void);
void reset_all_pwm(void);
void init_rap_contorl_data(void);

/**
 * [reset_all_control_ph description]
 * @method reset_all_control_ph
 */
void reset_all_control_ph(void)
{
    close_all_valve();
    reset_all_pwm();
    reset_blower();
    Blower_enable();
    reset_oxygen_mixer();
}

/**
 * [reset_all_control_data description]
 * @method reset_all_control_data
 */
void reset_all_control_data(void)
{
    reset_main_control_data();

    reset_PID_blower_control_data();
#if USE_ADC_RAM_DATA
#else
    init_blower_control_data_for_flow_0();
#endif
}

/**
 * [Control_task description]
 * @method Control_task
 * @param  argument     [description]
 */
void Control_task(void const *argument)
{
    /* USER CODE BEGIN Control_task */
    uint16_t cycles;
    uint16_t control_cycle;
    uint32_t flag;
    cycles = 0;
    control_cycle = 0;

    reset_all_control_ph();
    reset_all_control_data();

    /* Infinite loop */
    for(;;)
    {
        cycles ++;
        if(cycles >= 64) // 64 x 2 ms actions
        {
            cycles = 0; // reset counter

            control_cycle ++;
            control_cycle &= 0x3f;
            flag = (uint32_t)CONTROL_SCH_ARRAY[control_cycle];
            // Do actions

            // Oxygen mixer adjust
            if(flag & OXYGEN_ADJUST_FLAG)
            {
                oxygen_adjust();
            }

            // Oxygen mixer PID
            if(flag & OXYGEN_PID_ACTIVED_FLAG)
            {
                oxygen_pid_adjust();
            }

            if(flag & FLOW_ADJUST_OVERFLOW_VALVE_FLAG)
            {
                // 回流阀调整依赖于流量及工作压力，是个极为复杂的工作

            }
            // Flow Blower adjust
            // 考虑风机调整的速度，每50ms 调整一下风机
            //flow_adjust();
            //main_control_pid_blower_adjust();
            if((cycles & 0x3) == 0)
                main_control_pid_blower_adjust();

            // EPAP adjust
            if(flag & EPAP_ADJUST_FLAG)
            {
                //epap_adjust();
            }
        }

        // 2ms cycles
        osDelay(2);
    }
    /* USER CODE END Control_task */
}

/// ----- File Control_Task.c end
