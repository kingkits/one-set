// file: peep_valve.c
#define PEEP_DATA_REVERSE 0
typedef unsigned int uint32_t;
const uint32_t PEEP_VALVE_IDX[] =
{
#if PEEP_DATA_REVERSE
    0,
    178461
#else
    178461,
    0
#endif
};

const uint32_t PEEP_VALVE_DATA[] =
{
#if PEEP_DATA_REVERSE
    1900,
    0
#else
    0,
    1900
#endif
};


const uint32_t PEEP_BLOWER_DATA[] =
{
#if PEEP_DATA_REVERSE
    23700,
    34460
#else
    23700,
    34460
#endif
};

