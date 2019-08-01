/********************************************************************************
 * version: V1.0
 *
 * date: 2018/09/03
 *
 * Note:
 *
********************************************************************************/

//
#define TSI_WRITE_DATA	0x0A	// 写tsi存储命令
#define TSI_READ_DATA	0x0B	// 读tsi存储命令

//
__packed typedef struct
{
    float32_t Vf;
    float32_t Ai;
    float32_t Bi;
    float32_t Ci;
} TSI_COEFF_TYPEDEF;

//
__packed typedef struct
{
    uint16_t crc;//2 byte
    uint32_t sn;//serial_number
    uint32_t mn;// model number
    int8_t revision;
    uint8_t reserve1;
    uint16_t year;
    uint8_t month;
    uint8_t day;
    float32_t k0; //Tcal;
    float32_t k1;
    float32_t k2;
    float32_t k3;
    float32_t k4;
    float32_t Tcal;
    float32_t S;
    float32_t Z;
    float32_t Tcorr;
    uint8_t reserve[10];
    uint8_t coeffs;
    uint8_t reserve2;
    TSI_COEFF_TYPEDEF coef[12];
} TSI_FLOW_TYPEDEF;




void Tsi840205_Load_Param(void);
void Tsi840205_Get_Flow(void);




/*********************************END******************************************/
