// File: Leak_test.c
#include "../global.h"
#include "../lib/leak_flow_count.h"
typedef struct __ST_LEAK_TEST_DATA_DEFINES
{
    int32_t base_leak_flow;
    int16_t base_leak_press;
    int16_t current_press;
    int32_t current_count_leak;

    uint8_t work_mode;

    // for leak count
    float64_t ins_flow_sum_data;
    float64_t exp_flow_sum_data;
    float64_t press_sum_data;
    uint32_t count;
} ST_LEAK_TEST_DATA_DEFINES;
ST_LEAK_TEST_DATA_DEFINES leak_test_data;

typedef enum __ENUM_LEAK_TEST_MODE
{
    EM_LEAK_DO_NOTHING,     //=0
    EM_LEAK_SET_BASE,       //=1
    EM_LEAK_COUNT_BASE,     //=2
    EM_LEAK_COUNT_LEAK,     //=3
    EM_LEAK_CLEAR_ALL,      //=4
    EM_LEAK_CLEAR_COUNT		//=5
} ENUM_LEAK_TEST_MODE;
#define LEAK_TEST_CHANGE_WORK_MODE(new_mode) leak_test_data.work_mode = new_mode


uint32_t get_inspire_flow(void);
int16_t  get_inspire_press(void);
int16_t  get_expire_press(void);
uint32_t get_expire_flow(void);
void set_main_control_epap_data(int16_t epap);
void Leak_test_start_epap_control(void);

static uint8_t leak_test_actived_flag = 0;

void leak_direct_set_base(int32_t flow, int16_t press)
{
    leak_test_data.base_leak_press = press;
    leak_test_data.base_leak_flow  = flow;
    flow_leak_refresh_data(leak_test_data.base_leak_press, leak_test_data.base_leak_flow);
}
void leak_test_set_work_press(uint32_t press)
{
    set_main_control_epap_data((int16_t)press);
    Leak_test_start_epap_control();
}

void start_leak_test(void)
{
    leak_test_actived_flag = 1;
}

void stop_leak_test(void)
{
    leak_test_actived_flag = 0;
}

void leak_test_change_to_idle(void)
{
    leak_test_data.work_mode = EM_LEAK_DO_NOTHING;
}

void leak_test_set_mode(uint8_t mode)
{
    leak_test_data.work_mode = mode;
}

void leak_clear_count_data(void)
{
    leak_test_data.exp_flow_sum_data  = 0.0;
    leak_test_data.ins_flow_sum_data  = 0.0;
    leak_test_data.press_sum_data     = 0.0;
    leak_test_data.count              = 0;
}
void leak_clear_all_data(void)
{
    leak_test_data.base_leak_flow     =   0;
    leak_test_data.base_leak_press    = 100;
    leak_test_data.current_count_leak =   0;
    leak_test_data.current_press      =   0;

    leak_clear_count_data();
}

void leak_test_count_leak(void)
{
    leak_test_data.current_press = (get_inspire_press() + get_expire_press()) / 2;
    leak_test_data.current_count_leak = flow_leak_compensation(leak_test_data.current_press);
}
void leak_test_count_base(void)
{
    leak_test_data.exp_flow_sum_data += (float64_t) get_expire_flow();
    leak_test_data.ins_flow_sum_data += (float64_t) get_inspire_flow();
    leak_test_data.press_sum_data    += (float64_t)(get_inspire_press() + get_expire_press()) / 2.0;
    leak_test_data.count ++;
}
void leak_test_set_base(void)
{
    float64_t dtmp;
    if(leak_test_data.count < 1000)
    {
        return;
    }
    //flow
    dtmp = (leak_test_data.ins_flow_sum_data - leak_test_data.exp_flow_sum_data) / (float64_t)leak_test_data.count;
    if(dtmp < 0.0) dtmp = 0.0;
    leak_test_data.base_leak_flow  = (int32_t) dtmp;
    // press
    dtmp = leak_test_data.press_sum_data / (float64_t) leak_test_data.count;
    if(dtmp < 0.0) dtmp = 0.0;
    leak_test_data.base_leak_press = (int16_t) dtmp;
    flow_leak_refresh_data(leak_test_data.base_leak_press, leak_test_data.base_leak_flow);

    leak_clear_count_data();
}

void do_leak_test_actions(void)
{
    if(!leak_test_actived_flag) return;
    switch(leak_test_data.work_mode)
    {
    default:
    case EM_LEAK_DO_NOTHING:	 //=0
        break;
    case EM_LEAK_SET_BASE:       //=1
        leak_test_set_base();
        LEAK_TEST_CHANGE_WORK_MODE(EM_LEAK_DO_NOTHING);
        break;
    case EM_LEAK_COUNT_BASE:     //=2
        leak_test_count_base();
        break;
    case EM_LEAK_COUNT_LEAK:     //=3
        leak_test_count_leak();
        break;
    case EM_LEAK_CLEAR_ALL:      //=4
        leak_clear_all_data();
        LEAK_TEST_CHANGE_WORK_MODE(EM_LEAK_DO_NOTHING);
        break;
    case EM_LEAK_CLEAR_COUNT:    //=5
        leak_clear_count_data();
        LEAK_TEST_CHANGE_WORK_MODE(EM_LEAK_DO_NOTHING);
        break;
    }
}

// The end of File: Leak_test.c

