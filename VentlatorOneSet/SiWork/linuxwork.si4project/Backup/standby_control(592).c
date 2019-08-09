// file: standby_control.c
#include "stm32f4xx_hal.h"
#include "../ControlTask/base_control.h"
#include "../global.h"
#include "breath_rap.h"
#include "../blower/blower.h"
#include "../adc/adc_manager.h"
#include "../lib/leak_flow_count.h"
#include "../lib/VentLIB.h"
#include "breath_Atomizer.h"
#include "breath_cycle_adjust.h"

/**
 * [breath_standby_mode description]
 * @method breath_standby_mode
 */
void breath_standby_mode(void)
{
    check_mode_change_flag();
    // 如果有检测要求执行检测流程
#if TEST_LEAK_FLOW
    do_leak_test_actions();
#endif
#if TEST_PEEP_VAL
    do_peep_test_actions();
#endif
}

/**
 * [to_breath_standby_mode description]
 * @method to_breath_standby_mode
 */
void to_breath_standby_mode(void)
{
    // 清除PEEP调整数据
    init_breath_cycles_adjust_data();

    // 停止PID
    set_PID_blower_adjust_mode(ENUM_PID_NO_ADJUST);
    //osDelay(20);

    // 关闭rap
    set_breath_rap_disable();

    // 关闭雾化器
    breath_change_nebulizer(EM_OFF);

    // 关闭混氧
    oxygen_stop();
    //reset_oxygen_mixer_control_data();

    // 涡轮风机最小压力模式
    set_blower_speed(5000);

    // 回流阀开到最大
    // set_overflow_valve(0);

    // 关负压风机
    stop_cough_blower_work();

    // 关咳痰阀
    COUGH_VALVE_CLOSE();

    // 开吸气阀
    INHALE_VALVE_OPEN();

    // 强制开PEEP阀到最小
    set_peep_press(0);

    // 清除所有显示数据
    reset_display_data();

    // 给UI发送显示数据
    set_one_cycle_complete_flag();

    // 禁止报警
    // 发送STANDBY MODE状态给UI

    // 设置工作模式及工作状态
    global_patient_breath_mode = EM_VENTLATOR_STANDBY_MODE;
    set_patient_status(EM_PATIENT_NOT_WORK);
}

