// file: time_count_correct.c
#define MAX_CORRENT_TIMER_NUMBERS 16
typedef struct __ST_CORRECT_TIMER_DATA_DEFINES
{
    uint8_t count;
    uint32_t temp;
    uint32_t *correct_timer_data_ptr [MAX_CORRENT_TIMER_NUMBERS];
} ST_CORRECT_TIMER_DATA_DEFINES;
static ST_CORRECT_TIMER_DATA_DEFINES time_count_data_pointers;

// initialize
void init_system_correct_timer_data_pointers(void)
{
    int i;
    for(i = 0; i < MAX_CORRENT_TIMER_NUMBERS; i++)
    {
        time_count_data_pointers.correct_timer_data_ptr[i] = &time_count_data_pointers.temp;
    }
    time_count_data_pointers.count = 0;
}

// append
uint8_t system_correct_timer_append(uint32_t *ptr)
{
    return 0;
}
// delete
void system_correct_timer_delete(uint8_t num)
{
    if(num >= time_count_data_pointers.count) return;
    time_count_data_pointers.correct_timer_data_ptr[num] = &time_count_data_pointers.temp;
}
// correct timers
void system_correct_timer_val(void)
{
    static uint32_t time_bak = 0;
    int i;
    if(ms_1_count < time_bak)// TDB
    {
        // 统一    + 60秒
        ms_1_count += 60000;
        for(i = 0; i < time_count_data_pointers.count; i++)
        {
            *(time_count_data_pointers.correct_timer_data_ptr[i]) += 60000;
        }
        // do some correct
    }
    time_bak = ms_1_count;
}

