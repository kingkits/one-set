//MDK和IAR通用的软件复位代码：
//#include "..\..\vent_new\Drivers\CMSIS\Include\core_cm4.h"
#include "../stm32f4xx_hal.h"
#include "cmsis_os.h"
//#if defined ( __CC_ARM   ) /*------------------RealView Compiler -----------------*/
//__asm void GenerateSystemReset(void)
//{
//    MOV R0, #1           ;
//    MSR FAULTMASK, R0    ; FAULTMASK 禁止一切中断产生
//    LDR R0, = 0xE000ED0C ;
//    LDR R1, = 0x05FA0004 ;
//    STR R1, [R0]         ;

//deadloop
//    B deadloop        //;
//}
//#elif (defined (__ICCARM__)) /*------------------ ICC Compiler -------------------*/
////#pragma diag_suppress=Pe940
//void GenerateSystemReset(void)
//{
//    __ASM("MOV R0, #1");
//    __ASM("MSR FAULTMASK, R0");
//    SCB->AIRCR = 0x05FA0004;
//    for(;;);
//}

//#endif

void GenerateSystemReset(void)
{
    register uint32_t R0; // Only for building warning
    __ASM("MOV R0, #1");
    __ASM("MSR FAULTMASK, R0");
    SCB->AIRCR = 0x05FA0004;
    for(;;);
}

// file end
