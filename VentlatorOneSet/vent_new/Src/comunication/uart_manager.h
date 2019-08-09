/*
   File: uart_manager.h
 */

//#include <stdio.h>
#ifndef _UART_MANAGER_H
#define _UART_MANAGER_H
typedef enum _ENUM_UART_TYPE_DEF
{
    ENUM_UART1,
    ENUM_UART2,
    ENUM_UART3,
    ENUM_UART4,
    ENUM_UART5,
    ENUM_UART6,
    ENUM_UART7,
    ENUM_UART8
} ENUM_UART_TYPE_DEF;

typedef struct _ST_UART_STRUCT_DEF
{
    unsigned char send_buf[256];
    unsigned char recv_buf[256];
    volatile unsigned char send_addr_r;
    volatile unsigned char send_addr_w;
    volatile unsigned char recv_addr_r;
    volatile unsigned char recv_addr_w;
} ST_UART_STRUCT_DEF;


extern ST_UART_STRUCT_DEF uart3_data;
#endif
// Flie uart_manager.h end
