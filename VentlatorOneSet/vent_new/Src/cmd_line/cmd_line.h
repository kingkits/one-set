/*
   File: cmd_line.h
 */

//#include <stdio.h>
#ifndef _CMD_LINE_H
#define _CMD_LINE_H
#include "ctype.h"
#include "stdint.h"
#define ARG_NUM     8          //命令中允许的参数个数
#define CMD_LEN     10         //命令名占用的最大字符长度
#define CMD_BUF_LEN 60         //命令缓存的最大长度

typedef struct
{
    char const *cmd_name;                        //命令字符串
    int32_t max_args;                            //最大参数数目
    void (*handle)(int argc, void *cmd_arg);     //命令回调函数
    char  *help;                                 //帮助信息
} cmd_list_struct;

typedef struct
{
    char rec_buf[CMD_BUF_LEN];            //接收命令缓冲区
    char processed_buf[CMD_BUF_LEN];      //存储加工后的命令(去除控制字符)
    int32_t cmd_arg[ARG_NUM];             //保存命令的参数
} cmd_analyze_struct;
#endif

void fill_rec_buf(char data);
static uint32_t get_true_char_stream(char *dest, const char *src);
static int32_t string_to_dec(uint8_t *buf, uint32_t len);
static int32_t cmd_arg_analyze(char *rec_buf, unsigned int len);
void CmdAnalyzeExecute( void );


// Flie cmd_line.h end
