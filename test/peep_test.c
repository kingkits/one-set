//File: peep_test.c
#include"../global.h"
#include "../adc/adc_manager.h"

#if TEST_PEEP_VAL
void     Test_start_press_pid(int16_t press);
void     Test_stop_press_pid(void);
void     set_peep_valve_control_val(uint16_t val);
uint32_t get_inhale_flow(void);
uint16_t get_blower_current_set_speed(void);
void     count_blower_speed(void);


#define MAX_PEEP_TEST_PRESS_NUM 8
#define MAX_PEEP_TEST_FLOW_NUM 28

const uint16_t PEEP_TEST_PRESS_ARRAY[] =
{
    50,		100,	150,	200,
    250,	300,	350,	400
};

const uint16_t PEEP_TEST_FLOW_ARRAY[] =
{
    5000,	6000,	7000,	8000,	9000,
    10000,	11000,	12000,	13000,	14000,
    15000,	16000,	17000,	18000,	19000,
    20000,	21000,	22000,	23000,	24000,
    25000,	26000,	27000,	28000,	29000,
    30000,	40000,	50000
};

typedef struct __ST_PEEP_TEST_DATA_DEFINES
{
    uint16_t set_peep_val;
    int16_t  peep_target_press;
    int32_t  peep_target_flow;
    uint8_t  work_status;
    uint32_t time_count;
    uint16_t start_press_locate;
    uint16_t stop_press_locate;
    uint16_t start_flow_locate;
    uint16_t stop_flow_locate;
    uint16_t current_press_locate;
    uint16_t current_flow_locate;
} ST_PEEP_TEST_DATA_DEFINES;

typedef enum __ENUM_PEEP_TEST_WORK_TYPES
{
    EM_PEEP_TEST_IDLE,     // 0空闲
    EM_PEEP_TEST_START,    // 1开始
    EM_PEEP_TEST_CLEAR_ALL,// 2清除所有
    EM_PEEP_TEST_SET_NEXT, // 3设置下一调整数据
    EM_PEEP_TEST_ADJUST,   // 4调整
    EM_PEEP_TEST_SAVE,     // 5写入数据
    EM_PEEP_TEST_STOP,     // 6结束
    EM_PEEP_TEST_OTHER
} ENUM_PEEP_TEST_WORK_TYPES;

ST_PEEP_TEST_DATA_DEFINES peep_test_data;


CCMRAM uint16_t peep_test_val_matrix[MAX_PEEP_TEST_PRESS_NUM][MAX_PEEP_TEST_FLOW_NUM];
CCMRAM uint16_t peep_test_blower_matrix[MAX_PEEP_TEST_PRESS_NUM][MAX_PEEP_TEST_FLOW_NUM];
static uint8_t  peep_test_actived_flag;


// CODE begin
void peep_test_clear_blower_data(void)
{
    int i, j;
    for(i = 0; i < MAX_PEEP_TEST_PRESS_NUM; i++)
        for(j = 0; j < MAX_PEEP_TEST_FLOW_NUM; j++)
            peep_test_blower_matrix[i][j] = 0;
}

void peep_test_correct_locate(void)
{
    // check range
    if(peep_test_data.start_flow_locate > MAX_PEEP_TEST_FLOW_NUM - 1)  peep_test_data.start_flow_locate  = MAX_PEEP_TEST_FLOW_NUM - 1;
    if(peep_test_data.stop_flow_locate  > MAX_PEEP_TEST_FLOW_NUM - 1)  peep_test_data.stop_flow_locate   = MAX_PEEP_TEST_FLOW_NUM - 1;
    if(peep_test_data.start_press_locate > MAX_PEEP_TEST_PRESS_NUM - 1) peep_test_data.start_press_locate = MAX_PEEP_TEST_PRESS_NUM - 1;
    if(peep_test_data.stop_press_locate > MAX_PEEP_TEST_PRESS_NUM - 1) peep_test_data.stop_press_locate  = MAX_PEEP_TEST_PRESS_NUM - 1;

    // check logic
    if(peep_test_data.start_flow_locate > peep_test_data.stop_flow_locate) peep_test_data.stop_flow_locate = peep_test_data.start_flow_locate;
    if(peep_test_data.start_press_locate > peep_test_data.stop_press_locate)peep_test_data.stop_press_locate = peep_test_data.start_press_locate;
}
void start_peep_test_valve(void)
{
    peep_test_actived_flag = 1;
}

void stop_peep_test_valve(void)
{
    peep_test_actived_flag = 0;
}

void peep_test_set_status(uint8_t status)
{
    peep_test_data.work_status = status;
}

void peep_test_press_pid_start(void)
{
    Test_start_press_pid(peep_test_data.peep_target_press);
}

void peep_test_press_pid_stop(void)
{
    Test_stop_press_pid();
}

void peep_test_clear_data(void)
{
    int i, j;
    for(i = 0; i < MAX_PEEP_TEST_PRESS_NUM; i++)
        for(j = 0; j < MAX_PEEP_TEST_FLOW_NUM; j++)
            peep_test_val_matrix[i][j] = 0;
}

void init_peep_test_data(void)
{
    peep_test_data.current_flow_locate  =     0;
    peep_test_data.current_press_locate =     5;
    peep_test_data.peep_target_flow     =  5000;
    peep_test_data.peep_target_press    =   300;
    peep_test_data.set_peep_val         =  1000;
    peep_test_data.start_flow_locate    =     0;
    peep_test_data.start_press_locate   =     5;
    peep_test_data.stop_flow_locate     =     27;
    peep_test_data.stop_press_locate    =     7;
    peep_test_data.time_count           =     0;
    peep_test_data.work_status          = EM_PEEP_TEST_IDLE;
    peep_test_clear_data();
}

void peep_test_set_current_work_value(void)
{
    peep_test_data.peep_target_press = (int16_t) PEEP_TEST_PRESS_ARRAY[peep_test_data.current_press_locate];
    peep_test_data.peep_target_flow  = (int16_t) PEEP_TEST_FLOW_ARRAY[peep_test_data.current_flow_locate];
    peep_test_data.time_count        = 0;
}

// =1 not ready
// =0 peep_data is ready to be save
uint8_t peep_test_adjust_flow(void)
{
    int32_t flow;
    int32_t diff, abs_val;
    int16_t adjust;
    double  dtmp;
    // delay
    peep_test_data.time_count ++;
    if(peep_test_data.time_count < 2000) return 1;
    peep_test_data.time_count = 0;

    // get flow
    flow = (int32_t)get_inhale_flow();

    NO_LESS_THAN(peep_test_data.peep_target_flow, 5000);
    // count different
    diff = flow - peep_test_data.peep_target_flow;



    // count adjust value
    if(diff < 0) abs_val = -diff;
    else         abs_val =  diff;

    // detect if is ready to save data
    if(abs_val < peep_test_data.peep_target_flow / 20)
    {
        return 0;
    }
    // Max change scale = 1
    if(abs_val > peep_test_data.peep_target_flow) abs_val = peep_test_data.peep_target_flow;

    dtmp = (double) abs_val / (double)peep_test_data.peep_target_flow;
    dtmp = 300.0 * dtmp;
    if(diff > 0)
    {
        adjust = peep_test_data.set_peep_val + (uint16_t)(dtmp / 1.5);
    }
    else
    {
        adjust = peep_test_data.set_peep_val - (uint16_t) dtmp;
    }


//    if(adjust < 0)
//    {
//        return 0;
//    }

//    if(adjust > 3500)
//    {
//        if(peep_test_data.set_peep_val > 3500) return 0;
//    }
	if(adjust < 0 || adjust > 3500)
	{
		return 2;
	}

    peep_test_data.set_peep_val = adjust;

    // set peep
    set_peep_valve_control_val(adjust);
    return 1;
}

// =1 not finished
// =0 work finished
uint8_t peep_test_set_next_step(void)
{
    peep_test_data.current_flow_locate ++;
    if(peep_test_data.current_flow_locate > peep_test_data.stop_flow_locate)
    {
        peep_test_data.current_flow_locate = peep_test_data.start_flow_locate;
        peep_test_data.current_press_locate ++;
        if(peep_test_data.current_press_locate > peep_test_data.stop_press_locate)
        {
            peep_test_data.current_press_locate = peep_test_data.start_press_locate;

            return 0; // =0 work finished
        }
    }
    else
    {
    }
    //peep_test_set_current_work_value();
    peep_test_set_current_work_value();
    peep_test_press_pid_start();

    return 1;// =1 not finished
}

void peep_test_save_data(void)
{
    peep_test_val_matrix[peep_test_data.current_press_locate][peep_test_data.current_flow_locate]
        = peep_test_data.set_peep_val;
    count_blower_speed();
    peep_test_blower_matrix[peep_test_data.current_press_locate][peep_test_data.current_flow_locate]
        = get_blower_current_set_speed();
}

void peep_test_start(void)
{
	init_peep_test_data();
    peep_test_clear_data();
    peep_test_correct_locate();
    peep_test_data.current_flow_locate  = peep_test_data.start_flow_locate;
    peep_test_data.current_press_locate = peep_test_data.start_press_locate;
    peep_test_set_current_work_value();
    set_peep_valve_control_val(peep_test_data.set_peep_val);
    peep_test_press_pid_start();
}

void do_peep_test_actions(void)
{
    if(!peep_test_actived_flag) return;

    switch(peep_test_data.work_status)
    {
    default:
        peep_test_data.work_status = EM_PEEP_TEST_IDLE;
        break;
    case EM_PEEP_TEST_IDLE:    // 空闲
        break;
    case EM_PEEP_TEST_START:   // 开始
        peep_test_start();
        peep_test_data.work_status = EM_PEEP_TEST_ADJUST;
        break;
    case EM_PEEP_TEST_SET_NEXT:// 设置下一调整数据
        if(peep_test_set_next_step())
        {
            peep_test_data.work_status = EM_PEEP_TEST_ADJUST;
            break;
        }
        else
        {
            peep_test_data.work_status = EM_PEEP_TEST_STOP;
            break;
        }
    case EM_PEEP_TEST_ADJUST:  // 调整
        switch(peep_test_adjust_flow())
        {
        	case 1:
            	break;
        	default:
				peep_test_data.work_status = EM_PEEP_TEST_SAVE;
				break;
        }
        break;
    case EM_PEEP_TEST_SAVE:    // 写入数据
        peep_test_save_data();
        peep_test_data.work_status = EM_PEEP_TEST_SET_NEXT;
        break;
    case EM_PEEP_TEST_STOP:    // 结束
    	active_buzzer();
        peep_test_press_pid_stop();
        peep_test_data.work_status = EM_PEEP_TEST_IDLE;
        break;
    case EM_PEEP_TEST_CLEAR_ALL:// 清除所有
        peep_test_press_pid_stop();
        init_peep_test_data();
        peep_test_data.work_status = EM_PEEP_TEST_IDLE;
        break;
    case EM_PEEP_TEST_OTHER:
        peep_test_data.work_status = EM_PEEP_TEST_IDLE;
        break;
    }
}

#endif //TEST_PEEP_VAL

