#include "cmd_line.h"
#include "stm32f4xx_hal.h"
#include "cmsis_os.h"
#include "stm32f4xx_it.h"
#include "../adc/adc_manager.h"
#include "stdarg.h"              /*支持函数接收不定量参数*/
#include "../global.h"

extern UART_HandleTypeDef huart1, huart3;
const char *const g_pcHex = "0123456789abcdef";
cmd_analyze_struct cmd_analyze;
unsigned int display_flag = 0;

TaskHandle_t xCmdAnalyzeHandle;

#pragma import(__use_no_semihosting)
//标准库需要的支持函数
struct __FILE
{
    int handle;
};

FILE __stdout;
//定义_sys_exit()以避免使用半主机模式
void _sys_exit(int x)
{
    x = x;
}

void uart3_put_char(uint8_t ch);
//重定义fputc函数
void uart3_put_char(unsigned char ch);

int fputc(int ch, FILE *f)
{
#if 0
    while((USART3->SR & 0X40) == 0); //循环发送,直到发送完毕
    USART3->DR = (uint8_t) ch;
#else
    uart3_put_char(ch);
#endif
    return ch;

}


extern uint8_t usart3_comand_buf[64];
__weak void show_command(void)
{
    printf("\r%s", usart3_comand_buf);
}


void do_downline()
{
    printf("\x1b\x5b\x42");
}


void do_upline(void)
{
    //printf("%c%c%c",0x1b,0x5b,0x41);
    printf("\x1b\x5b\x41");
}

void do_clear_diaplay_actions(void)
{
    if(display_flag)
    {
        do_upline();
        do_upline();
        do_upline();
        printf("                                                                                                 \r\n\n");
    }
}

uint8_t is_pwm_chanel_actived(uint8_t chanel);
uint16_t get_pwm_control_val(uint8_t chanel);
void display_pwm_info(void)
{
    uint8_t i;
    printf("\r       \033[1;40;32m");
    for(i = 0; i < 4; i++)
    {
        if(is_pwm_chanel_actived(i))
        {
            printf("PWM%d = %0.1f%%   \t", i + 1, get_pwm_control_val(i) / 10.0);
        }
    }
}
void do_display_actions(void)
{
    static int count = 0;
    if(display_flag)
    {
        count ++;

        //do_upline();
        do_upline();
        do_upline();
        //        printf("\r       \033[1;40;31m  O2=%0.3fV    p1=%0.3f   p2=%0.3f   p3=%0.3f   p4=%0.3f",
        //               adc_convert_data.O2_concent_2_val / 1000.0,
        //               adc_convert_data.pressure_1_val / 1000.0,
        //               adc_convert_data.pressure_2_val / 1000.0,
        //               adc_convert_data.pressure_3_val / 1000.0,
        //               adc_convert_data.pressure_4_val / 1000.0
        //              );
        display_pwm_info();
        do_downline();
        //if(count > 5)
        {
            count = 0;
            printf("\r\t\033[1;40;33m\tF1=%0.3f\tF2=%0.3f\tF3=%0.3f\tF4=%0.3f",
                   adc_convert_data.flow_1_val / 1000.0,
                   adc_convert_data.flow_2_val / 1000.0,
                   adc_convert_data.flow_3_val / 1000.0,
                   adc_convert_data.adc_flow_oxygen_val / 1000.0
                  );
        }
        do_downline();
        show_command();
    }
}


/*提供给串口服务程序，保存串口接收到的单个字符*/
void fill_rec_buf(char data)
{
    //接收数据
    static uint32_t rec_count = 0;

    cmd_analyze.rec_buf[rec_count] = data;
    if(0x0A == cmd_analyze.rec_buf[rec_count] && 0x0D == cmd_analyze.rec_buf[rec_count - 1])
    {
        rec_count = 0;

        /*收到一帧数据，向命令行解释器任务发送通知*/
        CmdAnalyzeExecute();

    }
    else
    {
        rec_count++;

        /*防御性代码，防止数组越界*/
        if(rec_count >= CMD_BUF_LEN)
        {
            rec_count = 0;
        }
    }
}

/**
* 使用SecureCRT串口收发工具,在发送的字符流中可能带有不需要的字符以及控制字符,
* 比如退格键,左右移动键等等,在使用命令行工具解析字符流之前,需要将这些无用字符以
* 及控制字符去除掉.
* 支持的控制字符有:
*   上移:1B 5B 41
*   下移:1B 5B 42
*   右移:1B 5B 43
*   左移:1B 5B 44
*   回车换行:0D 0A
*  Backspace:08
*  Delete:7F
*/
static uint32_t get_true_char_stream(char *dest, const char *src)
{
    uint32_t dest_count = 0;
    uint32_t src_count = 0;

    while(src[src_count] != 0x0D && src[src_count + 1] != 0x0A)
    {
        if(isprint(src[src_count]))
        {
            dest[dest_count++] = src[src_count++];
        }
        else
        {
            switch(src[src_count])
            {
            case    0x08:                          //退格键键值
            {
                if(dest_count > 0)
                {
                    dest_count --;
                }
                src_count ++;
            }
            break;
            case    0x1B:
            {
                if(src[src_count + 1] == 0x5B)
                {
                    if(src[src_count + 2] == 0x41 || src[src_count + 2] == 0x42)
                    {
                        src_count += 3;             //上移和下移键键值
                    }
                    else if(src[src_count + 2] == 0x43)
                    {
                        dest_count++;               //右移键键值
                        src_count += 3;
                    }
                    else if(src[src_count + 2] == 0x44)
                    {
                        if(dest_count > 0)          //左移键键值
                        {
                            dest_count --;
                        }
                        src_count += 3;
                    }
                    else
                    {
                        src_count += 3;
                    }
                }
                else
                {
                    src_count ++;
                }
            }
            break;
            default:
            {
                src_count++;
            }
            break;
            }
        }
    }
    dest[dest_count++] = src[src_count++];
    dest[dest_count++] = src[src_count++];
    return dest_count;
}


/*字符串转10/16进制数*/
static int32_t string_to_dec(uint8_t *buf, uint32_t len)
{
    uint32_t i = 0;
    uint32_t base = 10;     //基数
    int32_t  neg = 1;       //表示正负,1=正数
    int32_t  result = 0;

    if((buf[0] == '0') && (buf[1] == 'x'))
    {
        base = 16;
        neg = 1;
        i = 2;
    }
    else if(buf[0] == '-')
    {
        base = 10;
        neg = -1;
        i = 1;
    }
    for(; i < len; i++)
    {
        if(buf[i] == 0x20 || buf[i] == 0x0D) //为空格
        {
            break;
        }

        result *= base;
        if(isdigit(buf[i]))                 //是否为0~9
        {
            result += buf[i] - '0';
        }
        else if(isxdigit(buf[i]))           //是否为a~f或者A~F
        {
            result += tolower(buf[i]) - 87;
        }
        else
        {
            result += buf[i] - '0';
        }
    }
    result *= neg;

    return result ;
}


/**
* 命令参数分析函数,以空格作为一个参数结束,支持输入十六进制数(如:0x15),支持输入负数(如-15)
* @param rec_buf   命令参数缓存区
* @param len       命令的最大可能长度
* @return -1:       参数个数过多,其它:参数个数
*/
static int32_t cmd_arg_analyze(char *rec_buf, unsigned int len)
{
    uint32_t i;
    uint32_t blank_space_flag = 0;  //空格标志
    uint32_t arg_num = 0;           //参数数目
    uint32_t index[ARG_NUM];        //有效参数首个数字的数组索引

    /*先做一遍分析,找出参数的数目,以及参数段的首个数字所在rec_buf数组中的下标*/
    for(i = 0; i < len; i++)
    {
        if(rec_buf[i] == 0x20)      //为空格
        {
            blank_space_flag = 1;
            continue;
        }
        else if(rec_buf[i] == 0x0D) //换行
        {
            break;
        }
        else
        {
            if(blank_space_flag == 1)
            {
                blank_space_flag = 0;
                if(arg_num < ARG_NUM)
                {
                    index[arg_num] = i;
                    arg_num++;
                }
                else
                {
                    return -1;      //参数个数太多
                }
            }
        }
    }

    for(i = 0; i < arg_num; i++)
    {
        cmd_analyze.cmd_arg[i] = string_to_dec((unsigned char *)(rec_buf + index[i]), len - index[i]);
    }
    return arg_num;
}




/*打印字符串:Hello world!*/
void printf_hello(int32_t argc, void *cmd_arg)
{
    printf("Hello world!\r\n");
}


/*打印每个参数*/
void handle_arg(int32_t argc, void *cmd_arg)
{
    uint32_t i;
    int32_t  *arg = (int32_t *)cmd_arg;

    if(argc == 0)
    {
        printf("no argument\r\n");
    }
    else
    {
        for(i = 0; i < argc; i++)
        {
            printf("The %d arg:%d\r\n", i + 1, arg[i]);
        }
    }
}

/*打印每个参数*/
void set_PWM1_percent(uint32_t val);

void handle_set(int32_t argc, void *cmd_arg)
{
    int32_t  *arg = (int32_t *)cmd_arg;

    if(argc == 0)
    {
        printf("no arg\r\n");
    }
    else
    {
        printf("set PWM duty: %d %%\r\n", arg[0] / 10);
        set_PWM1_percent(arg[0]);
    }
}

uint32_t get_pwm_scale(void);
void PWM_change_val(uint32_t chanel, uint32_t val);

void handle_pwm(int32_t argc, void *cmd_arg)
{
    uint32_t value;
    int32_t  *arg = (int32_t *)cmd_arg;


    if(argc < 2)
    {
        printf("too few arg\r\n");
        printf("usage: pwm <chanel> <0-1000>  (unit 0.1%% of max duty)\r\n");
        return;
    }

    value = (uint32_t)arg[1];
    if(value > 1000) value = 1000;
    value *= get_pwm_scale();
    value /= 1000;
    switch(arg[0])
    {
    case 1:
        PWM_change_val(TIM_CHANNEL_1, value);
        printf("set chanel %d PWM duty: %d %%\r\n", arg[0], arg[1] / 10);
        break;
    case 2:
        PWM_change_val(TIM_CHANNEL_2, value);
        printf("set chanel %d PWM duty: %d %%\r\n", arg[0], arg[1] / 10);
        break;
    case 3:
        PWM_change_val(TIM_CHANNEL_3, value);
        printf("set chanel %d PWM duty: %d %%\r\n", arg[0], arg[1] / 10);
        break;
    case 4:
        PWM_change_val(TIM_CHANNEL_4, value);
        printf("set chanel %d PWM duty: %d %%\r\n", arg[0], arg[1] / 10);
        break;
    default:
        printf("error arg\r\n");
        printf("usage: pwm <chanel(1-4)> <val(0-1000)>  (unit 0.1%% of max duty)\r\n");
        break;
    }
}

void wave_start(uint8_t     chanel, uint32_t d1, uint32_t d2, uint16_t t1, uint16_t t2 );

void handle_wave(int32_t argc, void *cmd_arg)
{
    int32_t  *arg = (int32_t *)cmd_arg;


    if(argc < 5)
    {
        printf("too few arg\r\n");
        printf("usage: w <chanel> <L1(0-1000)> <L2(0-1000)> <T1(ms)> <T2(ms)>\r\n");
        return;
    }
    if(arg[0] < 1 || arg[0] > 4)
    {
        printf("Error! chanel must betueen 1-4\r\n");
    }
    wave_start(arg[0], arg[1], arg[2], arg[3], arg[4]);
    printf("Done!\r\n");
}

void handle_disp(int32_t argc, void *cmd_arg)
{
    int i;
    if(display_flag == 0)
    {
        for(i = 0; i < 25; i++)
        {
            printf("\r\n");
        }
        display_flag = 1;
    }
    else display_flag = 0;
}
void stop_pwm_step_test(uint8_t chanel);

void stop_ui_data(void);
void handle_stop(int32_t argc, void *cmd_arg)
{
#if UART_TXT_COM_ENABLED
    int32_t  *arg = (int32_t *)cmd_arg;

    if(argc != 1)
    {
        printf("error arg\r\n");
        printf("usage: s <chanel(1-4)>\r\n");
        return;
    }
    if(arg[0] < 1 || arg[0] > 4)
    {
        printf("Error! chanel must between 1-4\r\n");
    }

    stop_pwm_step_test(arg[0]);
#endif
}
void pause_pwm_step_test(uint8_t chanel);
void continue_pwm_step_test(uint8_t chanel);

void reverse_pwm_test(uint8_t chanel);
void handle_reverse(int32_t argc, void *cmd_arg)
{
#if UART_TXT_COM_ENABLED
    int32_t  *arg = (int32_t *)cmd_arg;

    if(argc != 1)
    {
        printf("error arg\r\n");
        printf("usage: r <chanel(1-4)>\r\n");
        return;
    }
    if(arg[0] < 1 || arg[0] > 4)
    {
        printf("Error! chanel must between 1-4\r\n");
    }

    reverse_pwm_test(arg[0]);
#endif
}

void handle_continue(int32_t argc, void *cmd_arg)
{
#if UART_TXT_COM_ENABLED
    int32_t  *arg = (int32_t *)cmd_arg;

    if(argc != 1)
    {
        printf("error arg\r\n");
        printf("usage: c <chanel(1-4)>\r\n");
        return;
    }
    if(arg[0] < 1 || arg[0] > 4)
    {
        printf("Error! chanel must between 1-4\r\n");
    }

    continue_pwm_step_test(arg[0]);
#endif
}
void handle_pause(int32_t argc, void *cmd_arg)
{
#if UART_TXT_COM_ENABLED
    int32_t  *arg = (int32_t *)cmd_arg;

    if(argc != 1)
    {
        printf("error arg\r\n");
        printf("usage: p <chanel(1-4)>\r\n");
        return;
    }
    if(arg[0] < 1 || arg[0] > 4)
    {
        printf("Error! chanel must between 1-4\r\n");
    }

    pause_pwm_step_test(arg[0]);
#endif
}
void start_pwm_down_test(uint8_t chanel, uint8_t delay, uint16_t data_dec);

void start_pwm_step_test(uint8_t chanel, uint8_t delay, uint16_t data_inc);
void start_ui_data(void);
void handle_start(int32_t argc, void *cmd_arg)
{
    int32_t  *arg = (int32_t *)cmd_arg;

    if(argc < 3)
    {
        printf("too few arg\r\n");
        printf("usage: t <chanel(1-4)> <T(S)> <Step(0-1000)>\r\n");
        return;
    }
    if(arg[0] < 1 || arg[0] > 4)
    {
        printf("Error! chanel must between 1-4\r\n");
    }
    start_pwm_step_test(arg[0], arg[1], arg[2]);
}
void handle_pwm_down(int32_t argc, void *cmd_arg)
{
    int32_t  *arg = (int32_t *)cmd_arg;

    if(argc < 3)
    {
        printf("too few arg\r\n");
        printf("usage: d <chanel(1-4)> <T(S)> <Step(0-1000)>\r\n");
        return;
    }
    if(arg[0] < 1 || arg[0] > 4)
    {
        printf("Error! chanel must between 1-4\r\n");
    }
    start_pwm_down_test(arg[0], arg[1], arg[2]);
}

void open_valve(uint8_t chanel);
void close_valve(uint8_t chanel);

void handle_open_valve(int32_t argc, void *cmd_arg)
{
    int32_t  *arg = (int32_t *)cmd_arg;

    if(argc != 1)
    {
        printf("too few arg\r\n");
        printf("usage: k <chanel(1-12)>\r\n");
        return;
    }
    if(arg[0] < 1 || arg[0] > 12)
    {
        printf("Error! chanel must between 1-12\r\n");
    }
    open_valve(arg[0]);
}

void handle_close_valve(int32_t argc, void *cmd_arg)
{
    int32_t  *arg = (int32_t *)cmd_arg;

    if(argc != 1)
    {
        printf("too few arg\r\n");
        printf("usage: g <chanel(1-12)>\r\n");
        return;
    }
    if(arg[0] < 1 || arg[0] > 12)
    {
        printf("Error! chanel must between 1-12\r\n");
    }
    close_valve(arg[0]);
}

void handle_help(int32_t argc, void *cmd_arg)
{
    //    printf("命令    参数数目                        帮助信息 \r\n");
    //    printf("help                                    -本命令 \r\n");
    //    printf("pwm <chanel> <0-1000>                   -pwm单端口设置命令 \r\n");
    //    printf("disp                                    -参数显示命令 \r\n");
    //    printf("w <chanel> a b c d                   -波形设置命令 \r\n");
    //    printf("t <chanel(1-4)> <dt(s)> <step(0-1000)>  -上升测试命令 \r\n");
    //    printf("d <chanel(1-4)> <dt(s)> <step(0-1000)>  -下降测试命令 \r\n");
    //    printf("s <chanel(1-4)>                          -停止测试命令 \r\n");
    //    printf("c <chanel(1-4)>                          -继续测试命令 \r\n");
    //    printf("p <chanel(1-4)>                          -暂停测试命令 \r\n");
    //    printf("r <chanel(1-4)>                          -反转测试命令 \r\n");
    //    printf("k <chanel(1-12)>                         -阀开启命令 \r\n");
    //    printf("g <chanel(1-12)>                         -阀关闭命令 \r\n");
}


/*命令表*/
const cmd_list_struct cmd_list[] =
{
    /* 命令    参数数目    处理函数                 帮助信息                */

    {"help",    0,      handle_help,     "help                     this command"},

#if UART_TXT_COM_ENABLED
    {"pwm",     2,      handle_pwm,      "pwm <chanel> <x.x%*1000> ... -pwm set val"},
    {"disp",    0,      handle_disp,     "disp  ...                 -disp command"},
    {"w",       5,      handle_wave,     "w <chanel> a b c d     -disp command"},
    {"t",       3,      handle_start,    "t <chanel> <dt> <step>  -start test"},
    {"s",       1,      handle_stop,     "stop  <chanel>              Stop test"},
    {"c",       1,      handle_continue, "cuntinue <chanel>           continue test"},
    {"p",       1,      handle_pause,    "pause  <chanel>           continue test"},
    {"r",       1,      handle_reverse,  "reverse  <chanel>           continue test"},
    {"d",       3,      handle_pwm_down, "t <chanel> <dt> <step>  -start test"},
    {"k",       1,      handle_open_valve, "k <chanel> -open valve"},
    {"g",       1,      handle_close_valve, "p <chanel> -close valve"}
#endif

};

int strcmp(const char *, const char *);
/*命令行分析任务*/
void CmdAnalyzeExecute( )
{
    uint32_t i;
    int32_t rec_arg_num;
    char cmd_buf[CMD_LEN];

    uint32_t rec_num;
    rec_num = get_true_char_stream(cmd_analyze.processed_buf, cmd_analyze.rec_buf);

    /*从接收数据中提取命令*/
    for(i = 0; i < CMD_LEN; i++)
    {
        if((i > 0) && ((cmd_analyze.processed_buf[i] == ' ') || (cmd_analyze.processed_buf[i] == 0x0D)))
        {
            cmd_buf[i] = '\0';      //字符串结束符
            break;
        }
        else
        {
            cmd_buf[i] = cmd_analyze.processed_buf[i];
        }
    }

    rec_arg_num = cmd_arg_analyze(&cmd_analyze.processed_buf[i], rec_num);

    for(i = 0; i < sizeof(cmd_list) / sizeof(cmd_list[0]); i++)
    {
        if(!strcmp((const char *)cmd_buf, (const char *)cmd_list[i].cmd_name))    //字符串相等
        {
            if(rec_arg_num < 0 || rec_arg_num > cmd_list[i].max_args)
            {
                do_clear_diaplay_actions();
                printf("too many arguments!\r\n");
            }
            else
            {
                do_clear_diaplay_actions();
                cmd_list[i].handle(rec_arg_num, (void *)cmd_analyze.cmd_arg);
            }
            break;
        }

    }
    if(i >= sizeof(cmd_list) / sizeof(cmd_list[0]))
    {
        do_clear_diaplay_actions();
        printf("unknow command!\r\n");
    }
}

/*   超级中断颜色定义
     前景               背景               颜色
     ---------------------------------------
     30                40              黑色
     31                41              紅色
     32                42              綠色
     33                43              黃色
     34                44              藍色
     35                45              紫紅色
     36                46              青藍色
     37                47              白色
     代码              意义
     -------------------------
     0                终端默认设置（一般的默认为黑底白字）
     1                高亮显示
     4                使用下划线
     5                闪烁
     7                反白显示
     8                不可见
 */




