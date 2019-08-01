//Flie:breath_cycle_adjust.h
#ifndef BREATH_CYCLE_ADJUST_H
#define BREATH_CYCLE_ADJUST_H

typedef enum
{
    EM_BREATH_ADJUST_SAVE_NOTHING,
    EM_BREATH_ADJUST_SAVE_E_CONTROL,
    EM_BREATH_ADJUST_SAVE_I_CONTROL,
    EM_BREATH_ADJUST_SAVE_T_CONTROL
} ENUM_BREATH_ADJUST_SAVE_TYPES_DEFINES;

void     init_breath_cycles_adjust_data(void);
void     breath_cycles_save_peep_data(uint8_t types, uint16_t peep_val, int32_t flow_val);
uint16_t breath_cycles_adjust_peep_data(uint8_t types, int32_t target_flow);
void     breath_cycles_save_blower_speed_data(uint8_t types, uint16_t speed_val, int32_t press_val);
uint16_t breath_cycles_adjust_speed_data(uint8_t types, int32_t target_press);
uint8_t is_breath_cycles_adjust_data_ready(void);
void breath_cycles_set_direct(int16_t last_peep, int16_t P_peak);
#endif //BREATH_CYCLE_ADJUST_H

