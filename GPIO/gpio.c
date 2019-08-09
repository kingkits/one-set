//File:GPIO.c

#include "stm32f4xx_hal.h"
#include "gpio.h"

//#define Valve4_GPIO_Port GPIOA
//#define Valve8_Pin GPIO_PIN_12
//#define Valve8_GPIO_Port GPIOA
//#define Valve3_Pin GPIO_PIN_13
//#define Valve3_GPIO_Port GPIOH
//#define Valve7_Pin GPIO_PIN_14
//#define Valve7_GPIO_Port GPIOH
//#define Valve2_Pin GPIO_PIN_15
//#define Valve2_GPIO_Port GPIOH
//#define Valve6_Pin GPIO_PIN_0
//#define Valve6_GPIO_Port GPIOI
//#define Valve1_Pin GPIO_PIN_1
//#define Valve1_GPIO_Port GPIOI
//#define Valve5_Pin GPIO_PIN_2
//#define Valve5_GPIO_Port GPIOI



void open_valve(uint8_t chanel)
{
    switch(chanel)
    {
    default:
        break;
    case EM_VALVE_CHANEL_0:
        break;
    case EM_VALVE_CHANEL_1:
        VALVE1_LOW();
        break;
    case EM_VALVE_CHANEL_2:
        VALVE2_LOW();
        break;
    case EM_VALVE_CHANEL_3:
        VALVE3_LOW();
        break;
    case EM_VALVE_CHANEL_4:
        VALVE4_LOW();
        break;
    case EM_VALVE_CHANEL_5:
        VALVE5_LOW();
        break;
    case EM_VALVE_CHANEL_6:
        VALVE6_LOW();
        break;
    case EM_VALVE_CHANEL_7:
        VALVE7_LOW();
        break;
    case EM_VALVE_CHANEL_8:
        VALVE8_LOW();
        break;
    case EM_VALVE_CHANEL_9:
        VALVE9_LOW();
        break;
    case EM_VALVE_CHANEL_10:
        VALVE10_LOW();
        break;
    case EM_VALVE_CHANEL_11:
        VALVE11_LOW();
        break;
    case EM_VALVE_CHANEL_12:
        VALVE12_LOW();
        break;
    }
}
void close_valve(uint8_t chanel)
{
    switch(chanel)
    {
    default:
        break;
    case EM_VALVE_CHANEL_0:
        break;
    case EM_VALVE_CHANEL_1:
        VALVE1_HIGH();
        break;
    case EM_VALVE_CHANEL_2:
        VALVE2_HIGH();
        break;
    case EM_VALVE_CHANEL_3:
        VALVE3_HIGH();
        break;
    case EM_VALVE_CHANEL_4:
        VALVE4_HIGH();
        break;
    case EM_VALVE_CHANEL_5:
        VALVE5_HIGH();
        break;
    case EM_VALVE_CHANEL_6:
        VALVE6_HIGH();
        break;
    case EM_VALVE_CHANEL_7:
        VALVE7_HIGH();
        break;
    case EM_VALVE_CHANEL_8:
        VALVE8_HIGH();
        break;
    case EM_VALVE_CHANEL_9:
        VALVE9_HIGH();
        break;
    case EM_VALVE_CHANEL_10:
        VALVE10_HIGH();
        break;
    case EM_VALVE_CHANEL_11:
        VALVE11_HIGH();
        break;
    case EM_VALVE_CHANEL_12:
        VALVE12_HIGH();
        break;
    }

}


void set_valve_base(uint8_t status)
{
    if(status & 0x1)
    {
        VALVE1_LOW(); // open
    }
    else
    {
        VALVE1_HIGH();// close
    }

    if(status &  0x2)
    {
        VALVE2_LOW(); // open
    }
    else
    {
        VALVE2_HIGH();// close
    }
    if(status &  0x4)
    {
        VALVE3_LOW(); // open
    }
    else
    {
        VALVE3_HIGH();// close
    }
    if(status &  0x8)
    {
        VALVE4_LOW(); // open
    }
    else
    {
        VALVE4_HIGH();// close
    }
    if(status &  0x10)
    {
        VALVE5_LOW(); // open
    }
    else
    {
        VALVE5_HIGH();// close
    }

    if(status &  0x20)
    {
        VALVE6_LOW(); // open
    }
    else
    {
        VALVE6_HIGH();// close
    }
    if(status &  0x40)
    {
        VALVE7_LOW(); // open
    }
    else
    {
        VALVE7_HIGH();// close
    }
    if(status &  0x80)
    {
        VALVE8_LOW(); // open
    }
    else
    {
        VALVE8_HIGH();// close
    }
}

void set_valve_groupA(uint8_t status)
{
    if(status & 0x1)
    {
        VALVE9_LOW(); // open
    }
    else
    {
        VALVE9_HIGH();// close
    }

    if(status &  0x2)
    {
        VALVE10_LOW(); // open
    }
    else
    {
        VALVE10_HIGH();// close
    }
    if(status &  0x4)
    {
        VALVE11_LOW(); // open
    }
    else
    {
        VALVE11_HIGH();// close
    }
    if(status &  0x8)
    {
        VALVE12_LOW(); // open
    }
    else
    {
        VALVE12_HIGH();// close
    }
    if(status &  0x80)
    {
        VALVE_GROUP_A_LOW(); // open
    }
    else
    {
        VALVE_GROUP_A_HIGH();// close
    }
}



void set_valve_groupB(uint8_t status)
{
    if(status & 0x1)
    {
        VALVE_B1_LOW(); // open
    }
    else
    {
        VALVE_B1_HIGH();// close
    }

    if(status &  0x2)
    {
        VALVE_B2_LOW(); // open
    }
    else
    {
        VALVE_B2_HIGH();// close
    }


    if(status &  0x80)
    {
        VALVE_GROUP_B_LOW(); // open
    }
    else
    {
        VALVE_GROUP_B_HIGH();// close
    }
}

void get_valve_status(void)
{
    //HAL_GPIO_ReadPin();
    // base
    st_Valve_status.base = 0;
    if(!HAL_GPIO_ReadPin(Valve1_GPIO_Port, Valve1_Pin)) st_Valve_status.base |= 0x1;
    if(!HAL_GPIO_ReadPin(Valve2_GPIO_Port, Valve2_Pin)) st_Valve_status.base |= 0x2;
    if(!HAL_GPIO_ReadPin(Valve3_GPIO_Port, Valve3_Pin)) st_Valve_status.base |= 0x4;
    if(!HAL_GPIO_ReadPin(Valve4_GPIO_Port, Valve4_Pin)) st_Valve_status.base |= 0x8;
    if(!HAL_GPIO_ReadPin(Valve5_GPIO_Port, Valve5_Pin)) st_Valve_status.base |= 0x10;
    if(!HAL_GPIO_ReadPin(Valve6_GPIO_Port, Valve6_Pin)) st_Valve_status.base |= 0x20;
    if(!HAL_GPIO_ReadPin(Valve7_GPIO_Port, Valve7_Pin)) st_Valve_status.base |= 0x40;
    if(!HAL_GPIO_ReadPin(Valve8_GPIO_Port, Valve8_Pin)) st_Valve_status.base |= 0x80;

    // group A

    st_Valve_status.GroupA = 0;
    if(!HAL_GPIO_ReadPin(Valve9_GPIO_Port, Valve9_Pin)) st_Valve_status.GroupA                 |= 0x1;
    if(!HAL_GPIO_ReadPin(Valve10_GPIO_Port, Valve10_Pin)) st_Valve_status.GroupA               |= 0x2;
    if(!HAL_GPIO_ReadPin(Valve11_GPIO_Port, Valve11_Pin)) st_Valve_status.GroupA               |= 0x4;
    if(!HAL_GPIO_ReadPin(Valve12_GPIO_Port, Valve12_Pin)) st_Valve_status.GroupA               |= 0x8;
    if(!HAL_GPIO_ReadPin(Valve_Enable_A_GPIO_Port, Valve_Enable_A_Pin)) st_Valve_status.GroupA |= 0x80;

    // group B
    st_Valve_status.GroupB = 0;
    if(!HAL_GPIO_ReadPin(H_Valve_1_GPIO_Port, H_Valve_1_Pin)) st_Valve_status.GroupB           |= 0x1;
    if(!HAL_GPIO_ReadPin(H_Valve_2_GPIO_Port, H_Valve_2_Pin)) st_Valve_status.GroupB           |= 0x2;
    if(!HAL_GPIO_ReadPin(Valve_Enable_B_GPIO_Port, Valve_Enable_B_Pin)) st_Valve_status.GroupB |= 0x80;
}

void open_all_valve(void)
{
    VALVE1_LOW();
    VALVE2_LOW();
    VALVE3_LOW();
    VALVE4_LOW();
    VALVE5_LOW();
    VALVE6_LOW();
    VALVE7_LOW();
    VALVE8_LOW();
    VALVE9_LOW();
    VALVE10_LOW();
    VALVE11_LOW();
    VALVE12_LOW();
    VALVE_GROUP_A_LOW();
    VALVE_GROUP_B_LOW();
}


void close_all_valve(void)
{
    VALVE1_HIGH();
    VALVE2_HIGH();
    VALVE3_HIGH();
    VALVE4_HIGH();
    VALVE5_HIGH();
    VALVE6_HIGH();
    VALVE7_HIGH();
    VALVE8_HIGH();
    VALVE9_HIGH();
    VALVE10_HIGH();
    VALVE11_HIGH();
    VALVE12_HIGH();
    VALVE_GROUP_A_HIGH();
    VALVE_GROUP_B_HIGH();
}

// END of file GPIO.c
