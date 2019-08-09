// File:cpap_control.c
#include "stm32f4xx_hal.h"
#include "../ControlTask/base_control.h"
#include "../global.h"
#include "breath_rap.h"
#include "../blower/blower.h"
#include "../adc/adc_manager.h"
#include "../lib/leak_flow_count.h"
#include "../lib/VentLIB.h"
#include "breath_Atomizer.h"
#include "breath_cycle_adjust.h"
#include "pcv_press_increase_data.c"
#include "cpap_control.h"
#include "trigger_breath.h"
#include "../alarm/alarm.h"

typedef struct __ST_CPAP_CONTROL_DAT
{
} ST_CPAP_CONTROL_DAT;

ST_CPAP_CONTROL_DAT cpap_control_dat;

// file end

