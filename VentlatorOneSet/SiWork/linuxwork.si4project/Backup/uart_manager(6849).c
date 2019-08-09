/*
    File uart_manager.c
 */
#include "stm32f4xx_hal.h"
#include "cmsis_os.h"
#include "stm32f4xx_it.h"

#include "../global.h"


extern UART_HandleTypeDef huart1, huart3;
#define extern
#include "uart_manager.h"
#define TEST_LOOPBACK_UART1 1

uint8_t uart3_get_char(void)
{
    unsigned char tmp;
    while(uart3_data.recv_addr_r == uart3_data.recv_addr_w)
    {
        //osDelay(1);
        return 0;
    }
    tmp = uart3_data.recv_buf[uart3_data.recv_addr_r];
    uart3_data.recv_addr_r ++;
    return tmp;
}


uint8_t get_ui_byte(char *byteptr)
{
    while(uart3_data.recv_addr_r == uart3_data.recv_addr_w)
    {
        //osDelay(1);
        *byteptr = 0;
        return 0;
    }
    *byteptr = uart3_data.recv_buf[uart3_data.recv_addr_r];
    uart3_data.recv_addr_r ++;
    return 1;
}

__weak __INLINE void uart3_put_char(uint8_t ch)
{
#if 1
    uint8_t	 ptr_old;
    uart3_data.send_buf[uart3_data.send_addr_w] = ch;
    ptr_old = uart3_data.send_addr_w;
    ptr_old ++;
    if(ptr_old == uart3_data.send_addr_r) // buffer is overflow
    {
        osDelay(1); // transmitter speed is about 10bytes/ms
    }
    uart3_data.send_addr_w ++;
    //uart3_data.send_addr_w &=0x3f;
    SET_BIT(huart3.Instance->CR1, USART_CR1_TXEIE);
#else
    while(!__HAL_UART_GET_FLAG(&huart3, UART_FLAG_TXE));
    huart3.Instance->DR = (uint8_t) ch;
#endif
}

void reset_uart_data(ST_UART_STRUCT_DEF *uart_data)
{
    int i;
    for(i = 0; i < 256; i++)
    {
        uart_data->recv_buf[i] = 0;
        if(i < 64)
        {
            uart_data->send_buf[i] = 0;
        }
    }
    uart_data->recv_addr_r = 0;
    uart_data->recv_addr_w = 0;
    uart_data->send_addr_r = 0;
    uart_data->send_addr_w = 0;
}

#define USRT_TX_IS_BUSY_BIT 1
void simple_uart3_actions(void)
{
    // read
    while(__HAL_UART_GET_FLAG(&huart3, UART_FLAG_RXNE))
    {
        uart3_data.recv_buf[uart3_data.recv_addr_w] = huart3.Instance->DR;
        //huart3.Instance->DR = uart3_data.recv_buf[uart3_data.recv_addr_w];
        uart3_data.recv_addr_w ++;
    }

    if(uart3_data.send_addr_r != uart3_data.send_addr_w)
    {
        if(__HAL_UART_GET_FLAG(&huart3, UART_FLAG_TXE))
        {
            huart3.Instance->DR = uart3_data.send_buf[uart3_data.send_addr_r];
            uart3_data.send_addr_r ++;
        }
    }
    if(__HAL_UART_GET_FLAG(&huart3, UART_FLAG_TC))
    {
        CLEAR_BIT(huart3.Instance->CR1, USART_CR1_TXEIE);
    }
}



void test_loopback_uart3(void)
{
    while(uart3_data.recv_addr_r != uart3_data.recv_addr_w)
    {
        uart3_data.send_buf[uart3_data.send_addr_w] = uart3_data.recv_buf[uart3_data.recv_addr_r];
        uart3_data.recv_addr_r ++;
        uart3_data.send_addr_w ++;
        SET_BIT(huart3.Instance->CR1, USART_CR1_TXEIE);
        huart3.gState = HAL_UART_STATE_BUSY_TX;
    }
}

void start_uart3_work(void)
{
    huart3.RxState = HAL_UART_STATE_BUSY_RX;
    /* Enable the UART Data Register not empty Interrupts */
    SET_BIT(huart3.Instance->CR1, USART_CR1_RXNEIE);
}
__weak void do_display_actions(void)
{

}
#define USE_COMMAND_DISP 1
#if USE_COMMAND_DISP
uint8_t usart3_comand_buf[64];
uint8_t uart3_comand_len;
void clear_command(void)
{
#if 0
    for(uart3_comand_len = 63; uart3_comand_len != 0; uart3_comand_len --)
    {
        usart3_comand_buf[uart3_comand_len] = 0;
    }
#endif
    uart3_comand_len     = 0;
    usart3_comand_buf[0] = 0;
}


__weak void append_cmd(uint8_t ch)
{
    if(ch == 0xd || ch == 0xa)
    {
        clear_command();
        return;
    }
    usart3_comand_buf[uart3_comand_len++]      = ch;
    uart3_comand_len                          &= 0x3f;
    usart3_comand_buf[uart3_comand_len]        = 0;
}
#endif

void fill_rec_buf(char data);
void pwm_test(void);
void do_UI_1ms_communication_actions(uint16_t counter);
void decode_communication_data(void);

void do_UI_500ms_communication_actions(void);

void uart3_actions(void)
{
    static uint16_t counter = 0;


    decode_communication_data(); // command decoder

    if(counter == 500)
    {
        do_UI_500ms_communication_actions();
    }
    if(counter >= 1000)
    {
        do_UI_500ms_communication_actions();
        counter = 0;
    }
    do_UI_1ms_communication_actions(counter);

    counter ++;
}


void reset_ui_data(void);

void Comm_Task(void const *argument)
{
    /* USER CODE BEGIN Comm_Task */
    /* Infinite loop */
    reset_ui_data();
    reset_uart_data(&uart3_data);
    // Start uart3
    start_uart3_work();
    for(;;)
    {
        uart3_actions();
        osDelay(1);
    }
    /* USER CODE END Comm_Task */
}

