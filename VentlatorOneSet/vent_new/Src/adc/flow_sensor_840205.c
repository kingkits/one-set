/********************************************************************************
 * version: V1.0
 *
 * date: 2018/09/03
 *
 * Note:
 *
********************************************************************************/

#include "stm32f4xx_hal.h"
#include "arm_math.h"
#include "cmsis_os.h"
#include "flow_sensor.h"
#include "flow_sensor_840205.h"
#include "../adc/adc_manager.h"
#include "../lib/insert_data.h"

//
//long flow_temp[12];					// 流量数据缓存
uint8_t tsi_param_flag;				// TSI读取结果标志
TSI_FLOW_TYPEDEF tsi_flow_para;		// 读取的TSI校准系数
extern SPI_HandleTypeDef hspi4;
extern TSI_sensor_flow_DATA tsi_sensor_flow_tmp;
extern ST_INSERT_DATA tsi_sensor_flow_T_data;

void Tsi840205_ParaAlign(void);
uint32_t Tsi840205_CalcCRC(uint8_t *ubuff, uint32_t num);

const TSI_FLOW_TYPEDEF tsi_def_para =  			// 当参数读取错误时使用此数据
{
    0x1729, 		        //  uint16_t crc;//2 byte
    2051640062, 	        //  uint32_t sn;//serial_number
    840205, 		        //  uint32_t mn;// model number
    65, 		            //  int8_t revision;
    0,			            //  uint8_t reserve1;
    2016,		            //  uint16_t year;
    9,			            //  uint8_t month;
    26, 		            //  uint8_t day;
    0.939643025,	        //  float32_t k0; //Tcal;
    0.0021033301,	        //  float32_t k1;
    0.00243301992,		    //	float32_t k2;
    9.2836699 / 1000000,    //  float32_t k3;
    1.37396995 / 10000000,	//	float32_t k4;
    21.1100006, 	        //  float32_t Tcal;
    1.70200002, 	        //  float32_t S;
    1.24399996, 	        //  float32_t Z;
    -0.0693553388,		    //	float32_t Tcorr;
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //  uint8_t reserve[10];
    9,			            //  uint8_t coeffs;
    0,			            //  uint8_t reserve2;
    0.352431625, -0.27447927, 2.05818772, 3.46425009,   //  coef[0] TSI_COEFF_TYPEDEF
    0.649367988, -0.526049912, 3.24082232, 1.32404304,  //  coef[1]
    0.996623755, -1.09358084, 4.31522942, 0.815889001,	//	coef[2]
    1.33283246,  -3.05236721, 6.16203833, 0.501591027,	//	coef[3]
    1.78996408,  -9.94849968, 9.35221863, 0.320630014,	//	coef[4]
    2.17561555,  -11.484416,  10.9189396, 0.200000003,	//	coef[5]
    2.81093645,   34.8370132, 3.54618359, 0.268000007,	//	coef[6]
    3.30801487,  -43.5505714, 14.6686811, 0.158628002,	//	coef[7]
    3.81769991,  -43.5505714, 14.6686811, 0.158628002,	//	coef[8]
    0.0, 0.0, 0.0, 0.0, 	//  coef[9]
    0.0, 0.0, 0.0, 0.0, 	//  coef[10]
    0.0, 0.0, 0.0, 0.0, 	//  coef[11]
};

/**************************************************************
 * Function :
 * param    :
 * return   :
 * Note     :
**************************************************************/
/**
 * [Tsi840205_Periph_Init description]
 * @method Tsi840205_Periph_Init
 */
void Tsi840205_Periph_Init(void)
{
    ;
}

/**
 * [copy_tsi_default_data description]
 * @method copy_tsi_default_data
 * @param  tsi_src               [description]
 * @param  tsi_tar               [description]
 */
void copy_tsi_default_data(TSI_FLOW_TYPEDEF *tsi_src, TSI_FLOW_TYPEDEF *tsi_tar)
{
    unsigned char *srcptr, *tarptr;
    int i;
    srcptr = (unsigned char *)tsi_src;
    tarptr = (unsigned char *)tsi_tar;
    for(i = 0; i < (sizeof(TSI_FLOW_TYPEDEF)); i++)
    {
        * tarptr = * srcptr;
        tarptr ++;
        srcptr ++;
    }
}

/**************************************************************
 * Function : spi 发送/接受一个字节
 * param    : 发送的字节
 * return   : 接受的字节
 * Note     :
**************************************************************/
uint8_t Tsi840205_SendData(uint8_t uiData)
{
    // hspi4.Instance->DR = uiData;
    while ((hspi4.Instance->SR & SPI_FLAG_TXE) == 0);	/*  等待数据发送完毕 			*/
    hspi4.Instance->DR = uiData;
    while ((hspi4.Instance->SR & SPI_FLAG_RXNE) == 0);
    return(hspi4.Instance->DR);
}

/**************************************************************
 * Function : 从TSI传感器的EEPROM中加载参数
 * param    : 无
 * return   : 无
 * Note     :
**************************************************************/
void Tsi840205_Load_Param(void)
{
#if 0
    uint32_t crc, tmp;
    __HAL_SPI_ENABLE(&hspi4);
    Tsi840205_SendData(TSI_READ_DATA);
    Tsi840205_SendData(0x00);	// 0x00 - flash地址
    tsi_param_flag = HAL_SPI_Receive(&hspi4, (uint8_t *)&tsi_flow_para.crc, sizeof(tsi_flow_para), 0xff);

    __HAL_SPI_DISABLE(&hspi4);
    crc = Tsi840205_CalcCRC((uint8_t *)&tsi_flow_para.sn, sizeof(tsi_flow_para) - 2);
    Tsi840205_ParaAlign();
    crc = tsi_flow_para.crc - crc;
    if(crc)
    {
        //tsi_flow_para = tsi_def_para;
        copy_tsi_default_data(tsi_def_para, tsi_flow_para);
        tsi_param_flag = HAL_ERROR;//报错
    }
    else
    {
        tmp = tsi_flow_para.sn / 1000000;
        if(tmp != 205 || tsi_flow_para.mn != 840205)
        {
            tsi_param_flag = HAL_ERROR;//报错
        }
    }
#else
    copy_tsi_default_data((TSI_FLOW_TYPEDEF *)&tsi_def_para, (TSI_FLOW_TYPEDEF *)&tsi_flow_para);
#endif
}

/**************************************************************
 * Function : 大端对齐 与 小端对齐 转行
 * param    : 数据长度 数据地址
 * return   : 无
 * Note     :
**************************************************************/
void Alignment_Change(uint8_t num, uint8_t *pdata)
{
    uint8_t i, tmp[8];
    if((num < 2) || (num > 8))
    {
        return;
    }
    for(i = 0; i < num; i++)
    {
        tmp[i] = *(pdata + i);
    }
    for(i = 0; i < num; i++)
    {
        *(pdata + i) = tmp[num - 1 - i];
    }
}

/**************************************************************
 * Function : 将TSI中的数据由大端对齐改为小端对齐
 * param    : 无
 * return   : 无
 * Note     :
**************************************************************/
void Tsi840205_ParaAlign(void)
{
    uint8_t i;
    Alignment_Change(sizeof(tsi_flow_para.crc), (uint8_t *)&tsi_flow_para.crc);
    Alignment_Change(sizeof(tsi_flow_para.sn), (uint8_t *)&tsi_flow_para.sn);
    Alignment_Change(sizeof(tsi_flow_para.mn), (uint8_t *)&tsi_flow_para.mn);
    Alignment_Change(sizeof(tsi_flow_para.year), (uint8_t *)&tsi_flow_para.year);
    Alignment_Change(sizeof(tsi_flow_para.k0), (uint8_t *)&tsi_flow_para.k0);
    Alignment_Change(sizeof(tsi_flow_para.k1), (uint8_t *)&tsi_flow_para.k1);
    Alignment_Change(sizeof(tsi_flow_para.k2), (uint8_t *)&tsi_flow_para.k2);
    Alignment_Change(sizeof(tsi_flow_para.k3), (uint8_t *)&tsi_flow_para.k3);
    Alignment_Change(sizeof(tsi_flow_para.k4), (uint8_t *)&tsi_flow_para.k4);
    Alignment_Change(sizeof(tsi_flow_para.Tcal), (uint8_t *)&tsi_flow_para.Tcal);
    Alignment_Change(sizeof(tsi_flow_para.S), (uint8_t *)&tsi_flow_para.S);
    Alignment_Change(sizeof(tsi_flow_para.Z), (uint8_t *)&tsi_flow_para.Z);
    Alignment_Change(sizeof(tsi_flow_para.Tcorr), (uint8_t *)&tsi_flow_para.Tcorr);

    for(i = 0; (i < tsi_flow_para.coeffs) && (i < 12); i++)
    {
        Alignment_Change(sizeof(tsi_flow_para.coef[i].Vf), (uint8_t *)&tsi_flow_para.coef[i].Vf);
        Alignment_Change(sizeof(tsi_flow_para.coef[i].Ai), (uint8_t *)&tsi_flow_para.coef[i].Ai);
        Alignment_Change(sizeof(tsi_flow_para.coef[i].Bi), (uint8_t *)&tsi_flow_para.coef[i].Bi);
        Alignment_Change(sizeof(tsi_flow_para.coef[i].Ci), (uint8_t *)&tsi_flow_para.coef[i].Ci);
    }
}

/**************************************************************
 * Function : 获取TSI的系数
 * param    : 对比的测量电压 A系数 B系数 C系数
 * return   : 无
 * Note     : 无
**************************************************************/
void Tsi840205_CalcCoeff(float32_t Vstd, float32_t *Ai, float32_t *Bi, float32_t *Ci)
{
    uint8_t i;
    if(Vstd <= tsi_flow_para.coef[0].Vf)
    {
        *Ai = tsi_flow_para.coef[0].Ai;
        *Bi = tsi_flow_para.coef[0].Bi;
        *Ci = tsi_flow_para.coef[0].Ci;
    }
    else if(Vstd >= tsi_flow_para.coef[tsi_flow_para.coeffs - 1].Vf)
    {
        *Ai = tsi_flow_para.coef[tsi_flow_para.coeffs - 1].Ai;
        *Bi = tsi_flow_para.coef[tsi_flow_para.coeffs - 1].Bi;
        *Ci = tsi_flow_para.coef[tsi_flow_para.coeffs - 1].Ci;
    }
    else
    {
        for(i = 0; i < (tsi_flow_para.coeffs - 1); i++)
        {
            if(Vstd < tsi_flow_para.coef[i + 1].Vf)
            {
                break;
            }
        }
        *Ai = tsi_flow_para.coef[i].Ai;
        *Bi = tsi_flow_para.coef[i].Bi;
        *Ci = tsi_flow_para.coef[i].Ci;
    }
}

/**************************************************************
 * Function : 流量数据排序取平均
 * param    : 流量数据地址  数据个数
 * return   : 计算的平均值
 * Note     :
**************************************************************/
long Tsi840205_CalcAverage(long *pflow, uint8_t num)
{
    uint8_t i;
    long tmp, dat[12]; // 12这个值需要跟flow_temp的数据一致
    for(i = 0; i < num; i++)
    {
        dat[i] = *(pflow + i);
    }

    for(i = 1; i < num; i++)
    {
        if(dat[0] > dat[i])
        {
            tmp = dat[0];
            dat[0] = dat[i];
            dat[i] = tmp;
        }
    }
    for(i = 2; i < num; i++)
    {
        if(dat[1] > dat[i])
        {
            tmp = dat[1];
            dat[1] = dat[i];
            dat[i] = tmp;
        }
    }
    for(i = 2; i < num - 1; i++)
    {
        if(dat[num - 1] < dat[i])
        {
            tmp = dat[num - 1];
            dat[num - 1] = dat[i];
            dat[i] = tmp;
        }
    }
    for(i = 2; i < num - 2; i++)
    {
        if(dat[num - 2] < dat[i])
        {
            tmp = dat[num - 2];
            dat[num - 2] = dat[i];
            dat[i] = tmp;
        }
    }
    tmp = 0;
    for(i = 2; i < num - 2; i++)
    {
        tmp += dat[i];
    }
    return (tmp >> 3);
}

/**************************************************************
 * Function : 获取TSI流量（写入结构体不返回）
 * param    : 无
 * return   : 无
 * Note     :
**************************************************************/
void Tsi840205_Get_Flow(void)
{
    //static uint8_t num;
    uint32_t utmp;
    float32_t Ai, Bi, Ci;
    float64_t vv, vvvvv;
    float64_t ftmp0, ftmp1, ftmp2, ftmp3, vb;
    //	if(!tsi_param_flag)
    {
        // 读取温度电压值（mv）
        tsi_sensor_flow_tmp.Vt = trans_adc3_data_to_volt(adc_voltage.adc_tsi_T);
        utmp = (uint32_t) tsi_sensor_flow_tmp.Vt + 10;	// 测量电压比计算电压高10mv左右
        // 转换成温度 °C
        tsi_sensor_flow_tmp.T = (int32_t)get_insert_dec_data(utmp, &tsi_sensor_flow_T_data);
        // 由Tcorr 修正T=T+Tcorr
        tsi_sensor_flow_tmp.T += tsi_flow_para.Tcorr;

        // 得到流量电压值 Vf： flow voltage （mV）
        tsi_sensor_flow_tmp.Vf = trans_adc3_data_to_volt(adc_voltage.adc_tsi_V);
        tsi_sensor_flow_tmp.Vf += tsi_sensor_flow_tmp.Vf; //*= 2;	// 转接板电压分压，恢复原始电压
        // 计算标准参考值
        // vb = (Vf + Z) / S
        // TempCompFactor = K0 + K1 *Vb + K 2 *T + K3 *T*T + K4 *T*T*T
        // V f Std = (V f + Z) ?TempCompFactor ? Z
        ftmp0 = tsi_sensor_flow_tmp.T * 0.001 - 20;
        tsi_sensor_flow_tmp.Vf = tsi_sensor_flow_tmp.Vf * (float32_t)0.001;	// voltage V
        vb = (tsi_sensor_flow_tmp.Vf + tsi_flow_para.Z) / tsi_flow_para.S;
        ftmp1 = ftmp0 * ftmp0;
        ftmp2 = ftmp0 * ftmp1;
        ftmp3 = tsi_flow_para.k0 + (float64_t)tsi_flow_para.k1 * vb + (float64_t)tsi_flow_para.k2 * ftmp0 + \
                (float64_t)tsi_flow_para.k3 * ftmp1 + (float64_t)tsi_flow_para.k4 * ftmp2;

        tsi_sensor_flow_tmp.Vstd = (tsi_sensor_flow_tmp.Vf + tsi_flow_para.Z) * ftmp3 - \
                                   tsi_flow_para.Z;

        // 获取计算系数Ai、Bi、Ci
        Tsi840205_CalcCoeff(tsi_sensor_flow_tmp.Vstd, &Ai, &Bi, &Ci);

        // 由标准参考值计算流量
        // Q = A + B * V * V + C * V * V * V * V * V
        //            (2次方)        <--- (5次方)---->
        vv = tsi_sensor_flow_tmp.Vstd * tsi_sensor_flow_tmp.Vstd;
        vvvvv = vv * vv;
        vvvvv = vvvvv * tsi_sensor_flow_tmp.Vstd;
        tsi_sensor_flow_tmp.Q = Ai;
        tsi_sensor_flow_tmp.Q += Bi * vv;
        tsi_sensor_flow_tmp.Q += Ci * vvvvv;
        // 转换成整形（ml/min）并返回
        tsi_sensor_flow_tmp.Q = tsi_sensor_flow_tmp.Q * (float32_t)1000; // 由L/Min --> mL/Min
#if 0
        flow_temp[num++] = (long)tsi_sensor_flow_tmp.Q;
        if(num == 12)
        {
            num = 0;
            adc_convert_data.flow_1_val = Tsi840205_CalcAverage(flow_temp, 12);
        }
#else
        adc_convert_data.flow_1_val = (int32_t)tsi_sensor_flow_tmp.Q;
#endif
    }
}


/**************************************************************
 * Function : TSI数据校验
 * param    : 校验数据首地址 数据长度
 * return   : 校验结果
 * Note     : 校验的结果要小端对齐，校验的数据是刚读取的结果
**************************************************************/
uint32_t Tsi840205_CalcCRC(uint8_t *ubuff, uint32_t num)
{
    uint32_t crc;
    uint32_t bit_count;
    uint32_t i;
    crc = 0x0000;
    for(i = 0; i < num; i++)
    {
        crc ^= ubuff[i];
        for(bit_count = 8; bit_count; bit_count--)
        {
            if(crc & 0x0001)
            {
                crc >>= 1;
                crc ^= 0xa001;
            }
            else
            {
                crc >>= 1;
            }
        }
    }
    return crc;
}

/**************************************************************
 * Function :
 * param    :
 * return   :
 * Note     :
**************************************************************/



/*********************************END******************************************/
