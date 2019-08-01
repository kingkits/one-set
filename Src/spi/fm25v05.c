/*
 ***************************************Copyright (c)**************************************************

 **--------------File Info-------------------------------------------------------------------------------
 ** File name:			fm25v02.h
 ** Note:				1)本程序为铁电FM25V05驱动程序.
 **						2)FM25V05为512KBit,即64KByte.
 **
 **------------------------------------------------------------------------------------------------------
 ** Created by:			zzx
 ** Created date:		2018.3.5
 ** Version:				1.0
 **
 *******************************************************************************************************
 */


/*
 ********************************************************************************************************
 ** Function name:		DelayNS
 ** Descriptions:		长软件延时
 ** input parameters:    uiDly延时控制值，值越大，延时越长
 ** output parameters:   无
 ** Returned value:      无
 ********************************************************************************************************
 */
#include "fm25v05.h"
//以下定义FRAM的命令字
//Name Op-code  Description
#define F_RAM_WREN  0X06 // F_RAM_WREN Set Write Enable Latch 0000 0110b
#define F_RAM_WRDI  0X04 // F_RAM_WRDI Write Disable          0000 0100b
#define F_RAM_RDSR  0X05 // F_RAM_RDSR Read Status Register   0000 0101b
#define F_RAM_WRSR  0X01 // F_RAM_WRSR Write Status Register  0000 0001b
#define F_RAM_READ  0X03 // F_RAM_READ Read Memory Data       0000 0011b
#define F_RAM_FSTRD 0X0B // F_RAM_FSTRD Fast Read Memory Data 0000 1011b
#define F_RAM_WRITE 0X02 // F_RAM_WRITE Write Memory Data     0000 0010b
#define F_RAM_SLEEP 0XB9 // F_RAM_SLEEP Enter Sleep Mode      1011 1001b
#define F_RAM_RDID  0X9F // F_RAM_RDID Read Device ID         1001 1111b
#define F_RAM_SNR   0XC3 // F_RAM_SNR Read S/N                1100 0011b

#define SPI_CS_EN() HAL_GPIO_WritePin(SPI1_CS_GPIO_Port, SPI1_CS_Pin,GPIO_PIN_RESET)
#define SPI_CS_DIS() HAL_GPIO_WritePin(SPI1_CS_GPIO_Port, SPI1_CS_Pin,GPIO_PIN_SET)
#define SPI_TRANSMIT_TIMEOUT 100

extern SPI_HandleTypeDef hspi1;

extern uint8_t spi_cmd_buf[]; // only for test


void start_fram(void)
{
    SPI_CS_EN();
    __HAL_SPI_ENABLE(&hspi1);
}
void stop_fram(void)
{
    __HAL_SPI_DISABLE(&hspi1);
    SPI_CS_DIS();
}

static void Delay (uint32_t uiDly)
{
    uint32_t i;

    for(; uiDly > 0; uiDly--)
    {
        for(i = 0; i < 10; i++);
    }
}

//static uint8_t spi_commang_buf[8];
/*
 ********************************************************************************************************
 ** Function name:		MSPISendData
 ** Descriptions:		SPI主模式发送数据
 ** input parameters:    uiData： 将要发送的数据
 ** output parameters:   无
 ** Returned value:      无
 ********************************************************************************************************
 */
static uint8_t MSPISendData(uint8_t uiData)
{
    hspi1.Instance->DR = uiData;
    while ((hspi1.Instance->SR & SPI_FLAG_TXE) == 0); /*  等待数据发送完毕      */
    return(hspi1.Instance->DR);
}

/*
 ********************************************************************************************************
 ** Function name:		FM25V05_WREN()
 ** Descriptions:		F25V05写允许命令
 ** input parameters:    无
 ** output parameters:   无
 ** Returned value:      无
 ********************************************************************************************************
 */
static void FM25V05_WREN(void)
{
    start_fram();
    //spi_commang_buf[0] = F_RAM_WREN;
    //HAL_SPI_Transmit(&hspi1,&spi_commang_buf,1,SPI_TRANSMIT_TIMEOUT);
    MSPISendData(F_RAM_WREN);
    stop_fram();
}

/*
 ********************************************************************************************************
 ** Function name:		FM25V05_WRDI()
 ** Descriptions:		F25V05写禁止命令
 ** input parameters:    无
 ** output parameters:   无
 ** Returned value:      无
 ********************************************************************************************************
 */
static void FM25V05_WRDI(void)
{
    start_fram();
    //spi_commang_buf[0] = F_RAM_WRDI;
    //HAL_SPI_Transmit(&hspi1,&spi_commang_buf,1,SPI_TRANSMIT_TIMEOUT);
    MSPISendData(F_RAM_WRDI);
    Delay(1);
    stop_fram();
}

/*
 ********************************************************************************************************
 ** Function name:		FM25V05_WRSR()
 ** Descriptions:		F25V05寄存器配置
 ** input parameters:    寄存器配置内容(uint8_t)
 ** output parameters:   无 spi_commang_buf
 ** Returned value:      无 HAL_SPI_TransmitReceive
 ********************************************************************************************************
 */
void FM25V05_WRSR(uint8_t Reg_Status)
{
    start_fram();
    MSPISendData(F_RAM_WRSR);
    MSPISendData(Reg_Status);
    Delay(1);
    stop_fram();
}

/*
 ********************************************************************************************************
 ** Function name:		FM25V05_RDSR()
 ** Descriptions:		F25V05寄存器配置内容读取
 ** input parameters:    无
 ** output parameters:   无
 ** Returned value:      寄存器配置值
 ********************************************************************************************************
 */
uint8_t FM25V05_RDSR(void)
{
    uint8_t Reg_Status;

    start_fram();
    MSPISendData(F_RAM_RDSR);
    Reg_Status = MSPISendData(0x00);
    Delay(1);
    stop_fram();

    return(Reg_Status);
}

/*
 ********************************************************************************************************
 ** Function name:		FM25V05_PWDN()
 ** Descriptions:		F25V05进入睡眠模式
 ** input parameters:    无
 ** output parameters:   无
 ** Returned value:      无
 ********************************************************************************************************
 */
void FM25V05_PWDN(void)
{
    start_fram();
    MSPISendData(F_RAM_SLEEP);
    Delay(2);
    stop_fram();
}

/*
 ********************************************************************************************************
 ** Function name:		FM25V05_WKUP()
 ** Descriptions:		F25V05进入睡眠模式
 ** input parameters:    无
 ** output parameters:   无
 ** Returned value:      无
 ********************************************************************************************************
 */
void FM25V05_WKUP(void)
{
    stop_fram();
    start_fram();
    Delay(4);        /* wait for a while */
    stop_fram();
}


/*
 ********************************************************************************************************
 ** Function name:		FM25V05_WRITE()
 ** Descriptions:		对F25V05内固定地址写入一个字节内容
 ** input parameters:    写入地址18位，用三字节表示其中高6位无效；写入的数据
 ** output parameters:   无
 ** Returned value:      无
 ********************************************************************************************************
 */
void FM25V05_WRITE(uint32_t WAddr, uint8_t *pBuf, uint32_t num)
{
    uint8_t WAddrL, WAddrM, WAddrH;

    // FM25V02 ADDR: 0x000000 ~ 0x007FFF.
    // FM25V05 ADDR: 0x000000 ~ 0x00FFFF.
    // FM25V10 ADDR: 0x000000 ~ 0x01FFFF.
    // FM25H10 ADDR: 0x000000 ~ 0x03FFFF.
    WAddrL =   WAddr & 0x000000FF;
    WAddrM = ((WAddr >>  8) & 0x000000FF);
    WAddrH = ((WAddr >> 16) & 0x00000003);                          /* 18 bit effective addr        */

    FM25V05_WREN();                                                 /* step 1 .  WEL = 1            */
    Delay(1);
    start_fram();

    MSPISendData(F_RAM_WRITE);
    MSPISendData(WAddrH);                                           /* step 3 . send address        */
    MSPISendData(WAddrM);
    MSPISendData(WAddrL);

    for (; num > 0; num--)                                          /* step 4 . send out bytes      */
    {
        MSPISendData(*pBuf++);
    }
    while (SPI_FLAG_TXE == (FM25V05_RDSR() & SPI_FLAG_TXE));        /* optional step 5. check WIP bit status*/
    stop_fram();
    Delay(1);
    FM25V05_WRDI();
}

/*
 ********************************************************************************************************
 ** Function name:		FM25V05_READ()
 ** Descriptions:		从F25V05内固定地址读出一个字节内容
 ** input parameters:    读数地址18位，用三字节表示其中高6位无效
 ** output parameters:   无
 ** Returned value:      无
 ********************************************************************************************************
 */
void FM25V05_READ(uint32_t RAddr, uint8_t *pBuf, uint32_t num)
{
    uint8_t RAddrL, RAddrM, RAddrH;

    // FM25V02 ADDR: 0x000000 ~ 0x007FFF.
    // FM25V05 ADDR: 0x000000 ~ 0x00FFFF.
    // FM25V10 ADDR: 0x000000 ~ 0x01FFFF.
    // FM25H10 ADDR: 0x000000 ~ 0x03FFFF.
    RAddrL = (uint8_t) (RAddr & 0x000000FF);
    RAddrM = (uint8_t) ((RAddr >>  8) & 0x000000FF);
    RAddrH = (uint8_t) ((RAddr >> 16) & 0x00000003);

    start_fram();
    MSPISendData(F_RAM_READ);
    MSPISendData(RAddrH);
    MSPISendData(RAddrM);
    MSPISendData(RAddrL);
    //MSPISendData(0x00); //dump read--- for unused data (zzx: no reason!)

    for (; num > 0; num--)
    {
        *pBuf++ = (MSPISendData(0x00));
    }
    Delay(1);
    stop_fram();
}

void spi_test(void)
{
    uint8_t i;
    for(i = 0; i < 16; i++)
    {
        spi_cmd_buf[i] = i;
    }

    // write data to Fram
    FM25V05_WRITE(0x100, (uint8_t *) &spi_cmd_buf, 0x10);

    // read data from
    FM25V05_READ(0x100 - 1, (uint8_t *) &spi_cmd_buf, 0x10);
}
/*
 ********************************************************************************************************
 **                            End Of File
 *******************************************************************************************************
 */
