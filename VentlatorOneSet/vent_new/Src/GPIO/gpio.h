// File GPIO.h
#ifndef __GPIO_H
#define __GPIO_H
#include "stm32f4xx_hal.h"
#include "../../core/inc/main.h"
typedef enum
{
    EM_VALVE_CHANEL_0  = 0,
    EM_VALVE_CHANEL_1  = 1,
    EM_VALVE_CHANEL_2  = 2,
    EM_VALVE_CHANEL_3  = 3,
    EM_VALVE_CHANEL_4  = 4,
    EM_VALVE_CHANEL_5  = 5,
    EM_VALVE_CHANEL_6  = 6,
    EM_VALVE_CHANEL_7  = 7,
    EM_VALVE_CHANEL_8  = 8,
    EM_VALVE_CHANEL_9  = 9,
    EM_VALVE_CHANEL_10 = 10,
    EM_VALVE_CHANEL_11 = 11,
    EM_VALVE_CHANEL_12 = 12
} __ENUM_VALVE_CHANEL_TYPES;

// Valve 1
#define VALVE1_HIGH() HAL_GPIO_WritePin(Valve1_GPIO_Port, Valve1_Pin, GPIO_PIN_SET)
#define VALVE1_LOW() HAL_GPIO_WritePin(Valve1_GPIO_Port, Valve1_Pin, GPIO_PIN_RESET)

// Valve 2
#define VALVE2_HIGH() HAL_GPIO_WritePin(Valve2_GPIO_Port, Valve2_Pin, GPIO_PIN_SET)
#define VALVE2_LOW() HAL_GPIO_WritePin(Valve2_GPIO_Port, Valve2_Pin, GPIO_PIN_RESET)

// Valve3
#define VALVE3_HIGH() HAL_GPIO_WritePin(Valve3_GPIO_Port, Valve3_Pin, GPIO_PIN_SET)
#define VALVE3_LOW() HAL_GPIO_WritePin(Valve3_GPIO_Port, Valve3_Pin, GPIO_PIN_RESET)

// Valve4
#define VALVE4_HIGH() HAL_GPIO_WritePin(Valve4_GPIO_Port, Valve4_Pin, GPIO_PIN_SET)
#define VALVE4_LOW() HAL_GPIO_WritePin(Valve4_GPIO_Port, Valve4_Pin, GPIO_PIN_RESET)

// Valve5
#define VALVE5_HIGH() HAL_GPIO_WritePin(Valve5_GPIO_Port, Valve5_Pin, GPIO_PIN_SET)
#define VALVE5_LOW() HAL_GPIO_WritePin(Valve5_GPIO_Port, Valve5_Pin, GPIO_PIN_RESET)

// Valve6
#define VALVE6_HIGH() HAL_GPIO_WritePin(Valve6_GPIO_Port, Valve6_Pin, GPIO_PIN_SET)
#define VALVE6_LOW() HAL_GPIO_WritePin(Valve6_GPIO_Port, Valve6_Pin, GPIO_PIN_RESET)


// Valve7
#define VALVE7_HIGH() HAL_GPIO_WritePin(Valve7_GPIO_Port, Valve7_Pin, GPIO_PIN_SET)
#define VALVE7_LOW() HAL_GPIO_WritePin(Valve7_GPIO_Port, Valve7_Pin, GPIO_PIN_RESET)


// Valve8
#define VALVE8_HIGH() HAL_GPIO_WritePin(Valve8_GPIO_Port, Valve8_Pin, GPIO_PIN_SET)
#define VALVE8_LOW() HAL_GPIO_WritePin(Valve8_GPIO_Port, Valve8_Pin, GPIO_PIN_RESET)


// Valve9
#define VALVE9_HIGH() HAL_GPIO_WritePin(Valve9_GPIO_Port, Valve9_Pin, GPIO_PIN_SET)
#define VALVE9_LOW() HAL_GPIO_WritePin(Valve9_GPIO_Port, Valve9_Pin, GPIO_PIN_RESET)



// Valve10
#define VALVE10_HIGH() HAL_GPIO_WritePin(Valve10_GPIO_Port, Valve10_Pin, GPIO_PIN_SET)
#define VALVE10_LOW() HAL_GPIO_WritePin(Valve10_GPIO_Port, Valve10_Pin, GPIO_PIN_RESET)


// Valve11
#define VALVE11_HIGH() HAL_GPIO_WritePin(Valve11_GPIO_Port, Valve11_Pin, GPIO_PIN_SET)
#define VALVE11_LOW() HAL_GPIO_WritePin(Valve11_GPIO_Port, Valve11_Pin, GPIO_PIN_RESET)


// Valve12
#define VALVE12_HIGH() HAL_GPIO_WritePin(Valve12_GPIO_Port, Valve12_Pin, GPIO_PIN_SET)
#define VALVE12_LOW() HAL_GPIO_WritePin(Valve12_GPIO_Port, Valve12_Pin, GPIO_PIN_RESET)


// Valve Group A 9-12 enable
#define VALVE_GROUP_A_HIGH() HAL_GPIO_WritePin(Valve_Enable_A_GPIO_Port, Valve_Enable_A_Pin, GPIO_PIN_SET)
#define VALVE_GROUP_A_LOW() HAL_GPIO_WritePin(Valve_Enable_A_GPIO_Port, Valve_Enable_A_Pin, GPIO_PIN_RESET)
// Valve Group B
#define VALVE_GROUP_B_HIGH() HAL_GPIO_WritePin(Valve_Enable_B_GPIO_Port, Valve_Enable_B_Pin, GPIO_PIN_SET)
#define VALVE_GROUP_B_LOW() HAL_GPIO_WritePin(Valve_Enable_B_GPIO_Port, Valve_Enable_B_Pin, GPIO_PIN_RESET)

#define VALVE_B1_HIGH() HAL_GPIO_WritePin(H_Valve_1_GPIO_Port, H_Valve_1_Pin, GPIO_PIN_SET)
#define VALVE_B1_LOW() HAL_GPIO_WritePin(H_Valve_1_GPIO_Port, H_Valve_1_Pin, GPIO_PIN_RESET)


#define VALVE_B2_HIGH() HAL_GPIO_WritePin(H_Valve_2_GPIO_Port, H_Valve_2_Pin, GPIO_PIN_SET)
#define VALVE_B2_LOW() HAL_GPIO_WritePin(H_Valve_2_GPIO_Port, H_Valve_2_Pin, GPIO_PIN_RESET)

#define sensor_flow_WP_HIGH() HAL_GPIO_WritePin(F1_EEPROM_WP_GPIO_Port, F1_EEPROM_WP_Pin, GPIO_PIN_SET)
#define sensor_flow_WP_LOW() HAL_GPIO_WritePin(F1_EEPROM_WP_GPIO_Port, F1_EEPROM_WP_Pin, GPIO_PIN_RESET)

typedef struct __ST_VALVE_STATUS
{
    unsigned char base;
    unsigned char GroupA;
    unsigned char GroupB;
} ST_VALVE_STATUS;
extern ST_VALVE_STATUS st_Valve_status;

/// function defines
void close_all_valve(void);
void set_valve_base(uint8_t status);
void set_valve_groupA(uint8_t status);
void set_valve_groupB(uint8_t status);
void close_valve(uint8_t chanel);
void open_valve(uint8_t chanel);


#endif // __GPIO_H

// end of file GPIO.h

