#define TEST_SDRAM 0
#define TEST_PWM 0
#define TEST_DELAY_US 0
#define TEST_KEY 0
#define TEST_SPI 0
#define TEST_FLOAT 0
#define TEST_OXYGEN_MIXTER 0
#define TEST_sensor_flow 0
#define TEST_ST_MODE 1
#define TEST_COUGH_MODE 1
#define TEST_RAP_MODE 1
#define TEST_NEBULIZER 1
#include "global.h"
#include "timer.h"
#include "./user_interface/kbd.h"
#include "stm32f4xx_hal.h"
#include "./gpio/gpio.h"


#define INHALE_VALVE_OPEN() close_valve(EM_VALVE_CHANEL_3)
#define INHALE_VALVE_CLOSE() open_valve(EM_VALVE_CHANEL_3)

#if TEST_KEY
uint16_t switch_key;
#endif

#if TEST_LEAK_FLOW
void leak_test_set_work_press(uint32_t press);
void start_leak_test(void);
void stop_leak_test(void);
void leak_test_set_mode(uint8_t mode);
void leak_direct_set_base(int32_t flow, int16_t press);

uint32_t base_leak_flow;
int16_t base_leak_press;
#endif

#if TEST_PEEP_VAL
void start_peep_test_valve(void);
void stop_peep_test_valve(void);
void peep_test_set_status(uint8_t status);
uint8_t peep_test_work_status = 0;

#endif

void test_sdram(void);
void start_sdram(void);
void test_loopback_uart3(void);
void test_delay_us(void);
void spi_test(void);
void vacuum_pressure_blower_start(int16_t press);
void GenerateSystemReset(void);
void set_blower_speed(uint16_t speed);
void Blower_enable(void);
void Blower_disable(void);
void reset_blower(void);
void init_tsi_peripheral(void);
void load_tsi_eeprom_data(void);
void test_start_st_mode1(void);
void test_start_st_mode(void);

void test_start_pcv_mode(void);

void test_start_cough_mode(void);
void test_stop_cough_mode(void);
void test_repeat_coughmode(void);
void open_valve(uint8_t chanel);
void close_valve(uint8_t chanel);
void test_rap(void);
void set_breath_rap_disable(void);
void format_eeprom_data(void);

#if SIMPLE_CLIBRATION_FLOW_ENABLE
void reset_simple_calibration_flow_data(void);
void do_simple_calibration_flow_work(void);
void simple_calibration_flow_start(void);
#endif


#if TEST_PWM
void reset_all_pwm(void);
#endif

void start_cpap_test(uint32_t val);

extern unsigned char test_flag;
extern uint32_t test_val;

// code begin
void test_start(void)
{
    test_flag = 0;
    test_val = 0;
#if TEST_PWM
    //reset_all_pwm();
#endif

#if TEST_SDRAM
    //start_sdram();
#endif
#if TEST_PWM
    //PWM_change_val(TIM_CHANNEL_1,6800);
#endif
#if TEST_KEY
    //switch_key = 0;
#endif
#if SIMPLE_CLIBRATION_FLOW_ENABLE
    reset_simple_calibration_flow_data();
#endif

}

float test_f_result;
void test_float(void)
{
    float f1;
    f1 = 2.0;
    test_f_result = 0.0;
    test_f_result = __sqrtf(f1);
}




void test_period(void)
{
#if TEST_DELAY_US
    if(test_flag == 5)
    {
        while(test_flag == 5)
        {
            test_delay_us();
        }
    }
#endif

#if TEST_PWM
    if(test_flag == 3)
    {
        test_flag = 0;
        PWM_start(TIM_CHANNEL_1); // no
    }
    if(test_flag == 4)
    {
        test_flag = 0;
        PWM_change_val(TIM_CHANNEL_1, test_val); // no
    }
#endif

#if TEST_SDRAM
    if(test_flag == 2)
    {
        test_flag = 0;
        test_sdram();
    }
#endif



#if TEST_KEY
    if(test_flag == 6)
    {
        test_flag = 0;
        switch_key = get_switch_data();
    }
#endif

#if TEST_SPI
    if(test_flag == 7)
    {
        test_flag = 0;
        spi_test();
    }
#endif

#ifdef TEST_FLOAT
    if(test_flag == 8)
    {
        test_flag = 0;
        test_float();
    }
#endif
    if(test_flag == 9)
    {
        test_flag = 0;
        GenerateSystemReset();
    }
#if TEST_OXYGEN_MIXTER
    if(test_flag == 10)
    {
        test_flag = 0;

        set_blower_speed(test_val);
    }
    if(test_flag == 11)
    {
        test_flag = 0;
        PWM_change_val(TIM_CHANNEL_3, test_val);
    }
#endif


#if TEST_sensor_flow
    if(test_flag == 0xf0)
    {
        test_flag = 0;
        init_tsi_peripheral();
        load_tsi_eeprom_data();
    }
#endif


#if TEST_COUGH_MODE
    if(test_flag == 0x20)
    {
        test_flag = 0;
        test_start_cough_mode();
    }
    if(test_flag == 0x21)
    {
        test_flag = 0;
        test_repeat_coughmode();
    }
    if(test_flag == 0x22)
    {
        test_flag = 0;
        test_stop_cough_mode();
    }
    if(test_flag == 0x23)
    {
        test_flag = 0;
        close_valve(EM_VALVE_CHANEL_3);
    }
    if(test_flag == 0x24)
    {
        test_flag = 0;
        open_valve(EM_VALVE_CHANEL_3);
    }
    if(test_flag == 0x25)
    {
        test_flag = 0;
        close_valve(EM_VALVE_CHANEL_6);
    }
    if(test_flag == 0x26)
    {
        test_flag = 0;
        open_valve(EM_VALVE_CHANEL_6);
    }
    if(test_flag == 0x27)
    {
        test_flag = 0;
        PWM_change_val(TIM_CHANNEL_3, test_val);
    }
    if(test_flag == 0x28)
    {
        test_flag = 0;
        test_val = 100;
        vacuum_pressure_blower_start(test_val);
    }
#endif


#if TEST_ST_MODE
    if(test_flag == 0x30)
    {
        test_flag = 0;
        test_start_st_mode();
    }
    if(test_flag == 0x31)
    {
        test_flag = 0;
        test_start_st_mode1();
    }
    if(test_flag == 0x32)
    {
        test_flag = 0;
        // 关闭吸气阀
        INHALE_VALVE_CLOSE();
    }
    if(test_flag == 0x33)
    {
        test_flag = 0;
        // 关闭吸气阀
        INHALE_VALVE_OPEN();
    }

    if(test_flag == 0x34)
    {
        test_flag = 0;
        test_start_pcv_mode();
    }

#endif
    if(test_flag == 0x40)
    {
        test_flag = 0;
        //
        PWM_change_val(TIM_CHANNEL_2, test_val);
    }
    if(test_flag == 0x41)
    {
        test_flag = 0;
        //
        PWM_change_val(TIM_CHANNEL_4, test_val);
    }
    if(test_flag == 0x42)
    {
        test_flag = 0;
        //
        set_blower_speed(test_val);
    }
#if TEST_RAP_MODE
    if(test_flag == 0x50)
    {
        // start rap
        test_flag = 0;
        test_rap();
    }
    if(test_flag == 0x51)
    {
        // stop rap
        test_flag = 0;
        set_breath_rap_disable();
    }
    if(test_flag == 0x52)
    {
        // stop rap
        test_flag = 0;
        open_valve((uint8_t)test_val);
    }
    if(test_flag == 0x53)
    {
        // stop rap
        test_flag = 0;
        close_valve((uint8_t)test_val);
    }
#endif

#if TEST_LEAK_FLOW

    if(test_flag == 0x60)
    {
        // 设置压力
        test_flag = 0;
        leak_test_set_work_press(test_val);
    }
    if(test_flag == 0x61)
    {
        // 启动泄漏测试
        test_flag = 0;
        start_leak_test();
    }
    if(test_flag == 0x62)
    {
        // 停止泄漏测试
        test_flag = 0;
        stop_leak_test();
    }

    if(test_flag == 0x63)
    {
        // 改变泄漏测试工作模式
        /*
           空闲 EM_LEAK_DO_NOTHING,           //=0
           设置基础泄漏 EM_LEAK_SET_BASE,      //=1
           计算基础泄漏 EM_LEAK_COUNT_BASE,    //=2
           计算当前泄漏 EM_LEAK_COUNT_LEAK,    //=3
           清除所有         EM_LEAK_CLEAR_ALL,//=4
           清除基础泄漏 EM_LEAK_CLEAR_COUNT		 //=5
         */
        test_flag = 0;
        leak_test_set_mode((uint8_t) (test_val & 0xff));
    }

    if(test_flag == 0x64)
    {
        // 改变泄漏测试工作模式
        test_flag = 0;
        leak_direct_set_base(base_leak_flow, base_leak_press);
    }
#endif

    /* **************************************************************
     * const uint16_t PEEP_TEST_PRESS_ARRAY[]=
     *   {
     *     50,	 100,	 150,	 200,
     *     250,	 300,	 350,	 400
     *   };
     *   const uint16_t PEEP_TEST_FLOW_ARRAY[]=
     *   {
     *     5000,	 6000,	 7000,	 8000,	 9000,
     *     10000,  11000,  12000,  13000,  14000,
     *     15000,  16000,  17000,  18000,  19000,
     *     20000,  21000,  22000,  23000,  24000,
     *     25000,  26000,  27000,  28000,  29000,
     *     30000,  40000,  50000
     *   };
     * **************************************************************
     */
#if TEST_PEEP_VAL
    if(test_flag == 0x65)
    {
        // 启动peep测试
        test_flag = 0;
        start_peep_test_valve();
    }

    if(test_flag == 0x66)
    {
        // 停止Peep测试
        test_flag = 0;
        stop_peep_test_valve();
    }

    if(test_flag == 0x67)
    {
        // 设置工作模式
        /* ************************************
            peep_test_work_status =
                  0 空闲
                  1 开始
                  2 清除所有数据
         ************************************
         */
        test_flag = 0;
        if(peep_test_work_status > 2) peep_test_work_status = 0;
        peep_test_set_status(peep_test_work_status);
    }

    if(test_flag == 0x70)
    {
        test_flag = 0;
        // 清除铁电RAM的数据，并用缺省值覆盖
        format_eeprom_data();
    }
    if(test_flag == 0x71)
    {
        test_flag = 0;
    }


#endif
    if(test_flag == 0x80)
    {
#if SIMPLE_CLIBRATION_FLOW_ENABLE
        simple_calibration_flow_start();
#endif
        test_flag = 0;
    }

	if(test_flag == 0x81)
	{
		start_cpap_test(test_val);
		test_flag = 0;
	}

}
// code end
