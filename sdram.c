#include "stm32f4xx_hal.h"
#include "cmsis_os.h"
extern SDRAM_HandleTypeDef hsdram1;
#define SDRAM_BANK 2

#define BUFFER_SIZE                              ((uint32_t)0x2000)
#define WRITE_READ_OFFSET                        ((uint32_t)0x0800)
#define SDRAM_BANK_ADDR                          ((uint32_t)0xD0000000)
#define SDRAM_SWAP_BANK_ADDR                     ((uint32_t)0x80000000)
#define SDRAM_REMAP_BANK_ADDR                    ((uint32_t)0x00000000)

#define SDRAM_MODEREG_BURST_LENGTH_1             ((uint16_t)0x0000)
#define SDRAM_MODEREG_BURST_LENGTH_2             ((uint16_t)0x0001)
#define SDRAM_MODEREG_BURST_LENGTH_4             ((uint16_t)0x0002)
#define SDRAM_MODEREG_BURST_LENGTH_8             ((uint16_t)0x0004)
#define SDRAM_MODEREG_BURST_TYPE_SEQUENTIAL      ((uint16_t)0x0000)
#define SDRAM_MODEREG_BURST_TYPE_INTERLEAVED     ((uint16_t)0x0008)
#define SDRAM_MODEREG_CAS_LATENCY_2              ((uint16_t)0x0020)
#define SDRAM_MODEREG_CAS_LATENCY_3              ((uint16_t)0x0030)
#define SDRAM_MODEREG_OPERATING_MODE_STANDARD    ((uint16_t)0x0000)
#define SDRAM_MODEREG_WRITEBURST_MODE_PROGRAMMED ((uint16_t)0x0000)
#define SDRAM_MODEREG_WRITEBURST_MODE_SINGLE     ((uint16_t)0x0200)

static void SDRAM_InitSequence(void);
void Memory_ReadBuffer(uint32_t *pBuffer, uint32_t uwReadAddress, uint32_t uwBufferSize);
void Memory_WriteBuffer(uint32_t *pBuffer, uint32_t uwWriteAddress, uint32_t uwBufferSize);
static void delay(__IO uint32_t nCount);

#define TEST_SDRAM_DEVICE 0
#if TEST_SDRAM_DEVICE
uint32_t aTxBuffer[BUFFER_SIZE];
uint32_t aRxBuffer[BUFFER_SIZE];
/**
 * @brief  Fills buffer with user predefined data.
 * @param  pBuffer: pointer on the buffer to fill
 * @param  BufferLenght: size of the buffer to fill
 * @param  Offset: first value to fill on the buffer
 * @retval None
 */
static void Fill_Buffer(uint32_t *pBuffer, uint32_t uwBufferLenght, uint32_t uwOffset)
{
    uint32_t tmpIndex = 0;

    /* Put in global buffer different values */
    for (tmpIndex = 0; tmpIndex < uwBufferLenght; tmpIndex++ )
    {
        pBuffer[tmpIndex] = tmpIndex + uwOffset;
    }
}
#endif

void start_sdram(void)
{
    SDRAM_InitSequence();
}

void ram_w(uint32_t addr, uint16_t dat)
{
    uint16_t *add = (uint16_t *)SDRAM_BANK_ADDR;
    add[addr] = dat;
}

uint16_t ram_r(uint32_t addr)
{
    uint16_t *add = (uint16_t *)SDRAM_BANK_ADDR;
    uint16_t date = 0;

    date = add[addr];
    return date;

}

//uint16_t test_data;
void test_sdram(void)
{
    // single data test
    // test_data = 0x5555;
    // ram_w(0,test_data);
    // test_data = 0;
    // test_data = ram_r(0);
#if TEST_SDRAM_DEVICE
    /* Fill the buffer to send */
    Fill_Buffer(aTxBuffer, BUFFER_SIZE, 0x250F);

    /* Write data to the SDRAM memory */
    Memory_WriteBuffer(aTxBuffer, SDRAM_BANK_ADDR + WRITE_READ_OFFSET, BUFFER_SIZE);
    /* Read back data from the SDRAM memory */
    Memory_ReadBuffer(aRxBuffer, SDRAM_BANK_ADDR + WRITE_READ_OFFSET, BUFFER_SIZE);
#endif
}

/**
 * @brief  defines the SDRAM Memory Refresh rate.
 * @param  FMC_Count: specifies the Refresh timer count.
 * @retval None
 */
void FMC_SetRefreshCount(uint32_t FMC_Count)
{
    /* check the parameters */
    assert_param(IS_FMC_REFRESH_RATE(FMC_Count));
    FMC_Bank5_6->SDRTR |= (FMC_Count << 1);

}
static FlagStatus is_sdram_status(uint32_t FMC_FLAG)
{
    FlagStatus bitstatus = RESET;
    uint32_t tmpsr = 0x00000000;

    // make sure sdram is work
    tmpsr = FMC_Bank5_6->SDSR;

    /* Get the flag status */
    if ((tmpsr & FMC_FLAG) != FMC_FLAG )
    {
        bitstatus = RESET;
    }
    else
    {
        bitstatus = SET;
    }
    return bitstatus;
}
void FMC_SDRAMCmdConfig(FMC_SDRAM_CommandTypeDef *FMC_SDRAMCommandStruct)
{
    uint32_t tmpr = 0x0;

    /* check parameters */
    assert_param(IS_FMC_COMMAND_MODE(FMC_SDRAMCommandStruct->CommandMode));
    assert_param(IS_FMC_COMMAND_TARGET(FMC_SDRAMCommandStruct->CommandTarget));
    assert_param(IS_FMC_AUTOREFRESH_NUMBER(FMC_SDRAMCommandStruct->AutoRefreshNumber));
    assert_param(IS_FMC_MODE_REGISTER(FMC_SDRAMCommandStruct->ModeRegisterDefinition));

    tmpr =   (uint32_t)(FMC_SDRAMCommandStruct->CommandMode |
                        FMC_SDRAMCommandStruct->CommandTarget |
                        (((FMC_SDRAMCommandStruct->AutoRefreshNumber) - 1) << 5) |
                        ((FMC_SDRAMCommandStruct->ModeRegisterDefinition) << 9));

    FMC_Bank5_6->SDCMR = tmpr;
}

static void SDRAM_InitSequence(void)
{
    FMC_SDRAM_CommandTypeDef FMC_SDRAMCommandStructure;
    uint32_t tmpr = 0;

    /* Step 3 --------------------------------------------------------------------*/
    /* Configure a clock configuration enable command */
    FMC_SDRAMCommandStructure.CommandMode            = FMC_SDRAM_CMD_CLK_ENABLE;//FMC_Command_Mode_CLK_Enabled;
    FMC_SDRAMCommandStructure.CommandTarget          = FMC_SDRAM_CMD_TARGET_BANK2;//FMC_Command_Target_bank1;
    FMC_SDRAMCommandStructure.AutoRefreshNumber      = 1;
    FMC_SDRAMCommandStructure.ModeRegisterDefinition = 0;
    /* Wait until the SDRAM controller is ready */
    while(is_sdram_status(FMC_SDRAM_FLAG_BUSY) != RESET)
    {
    }
    /* Send the command */
    FMC_SDRAMCmdConfig(&FMC_SDRAMCommandStructure);

    /* Step 4 --------------------------------------------------------------------*/
    /* Insert 100 ms delay */
    delay(10);

    /* Step 5 --------------------------------------------------------------------*/
    /* Configure a PALL (precharge all) command */
    FMC_SDRAMCommandStructure.CommandMode            = FMC_SDRAM_CMD_PALL;//FMC_Command_Mode_PALL;
    FMC_SDRAMCommandStructure.CommandTarget          = FMC_SDRAM_CMD_TARGET_BANK2;//FMC_Command_Target_bank1;
    FMC_SDRAMCommandStructure.AutoRefreshNumber      = 1;
    FMC_SDRAMCommandStructure.ModeRegisterDefinition = 0;
    /* Wait until the SDRAM controller is ready */
    while(is_sdram_status(FMC_SDRAM_FLAG_BUSY) != RESET)
    {
    }
    /* Send the command */
    FMC_SDRAMCmdConfig(&FMC_SDRAMCommandStructure);

    /* Step 6 --------------------------------------------------------------------*/
    /* Configure a Auto-Refresh command */
    FMC_SDRAMCommandStructure.CommandMode            = FMC_SDRAM_CMD_AUTOREFRESH_MODE;//FMC_Command_Mode_AutoRefresh;
    FMC_SDRAMCommandStructure.CommandTarget          = FMC_SDRAM_CMD_TARGET_BANK2;//FMC_Command_Target_bank1;
    FMC_SDRAMCommandStructure.AutoRefreshNumber      = 8;
    FMC_SDRAMCommandStructure.ModeRegisterDefinition = 0;
    /* Wait until the SDRAM controller is ready */
    while(is_sdram_status(FMC_SDRAM_FLAG_BUSY) != RESET)
    {
    }

    /* Send the command */
    FMC_SDRAMCmdConfig(&FMC_SDRAMCommandStructure);

    /* Step 7 --------------------------------------------------------------------*/
    /* Program the external memory mode register */
    tmpr = (uint32_t)SDRAM_MODEREG_BURST_LENGTH_1          |
           SDRAM_MODEREG_BURST_TYPE_SEQUENTIAL   |
           SDRAM_MODEREG_CAS_LATENCY_3           |
           SDRAM_MODEREG_OPERATING_MODE_STANDARD |
           SDRAM_MODEREG_WRITEBURST_MODE_SINGLE;

    /* Configure a load Mode register command*/
    FMC_SDRAMCommandStructure.CommandMode            = FMC_SDRAM_CMD_LOAD_MODE;//FMC_Command_Mode_LoadMode;
    FMC_SDRAMCommandStructure.CommandTarget          = FMC_SDRAM_CMD_TARGET_BANK2;
    FMC_SDRAMCommandStructure.AutoRefreshNumber      = 1;
    FMC_SDRAMCommandStructure.ModeRegisterDefinition = tmpr;
    /* Wait until the SDRAM controller is ready */
    while(is_sdram_status(FMC_SDRAM_FLAG_BUSY) != RESET)
    {
    }

    /* Send the command */
    FMC_SDRAMCmdConfig(&FMC_SDRAMCommandStructure);

    /* Step 8 --------------------------------------------------------------------*/

    /* Set the refresh rate counter */
    /* (15.62 us x Freq) - 20 */
    /* Set the device refresh counter */
    FMC_SetRefreshCount(448);
    /* Wait until the SDRAM controller is ready */
    while(is_sdram_status(FMC_SDRAM_FLAG_BUSY) != RESET)
    {
    }
}


/**
 * @brief  Reads data buffer from the memory.
 * @param  pBuffer: pointer to buffer.
 * @param  ReadAddress: memory address from which the data will be
 *         read.
 * @param  uwBufferSize: number of words to write.
 * @retval None.
 */
void Memory_ReadBuffer(uint32_t *pBuffer, uint32_t uwReadAddress, uint32_t uwBufferSize)
{
    __IO uint32_t write_pointer = (uint32_t)uwReadAddress;

    /* Read data */
    for(; uwBufferSize != 0x00; uwBufferSize--)
    {
        *pBuffer++ = *(__IO uint32_t *)(write_pointer );

        /* Increment the address*/
        write_pointer += 4;
    }
}

/**
 * @brief  Writes a Entire-word buffer to the memory.
 * @param  pBuffer: pointer to buffer.
 * @param  uwWriteAddress: memory address from which the data will be
 *         written.
 * @param  uwBufferSize: number of words to write.
 * @retval None.
 */
void Memory_WriteBuffer(uint32_t *pBuffer, uint32_t uwWriteAddress, uint32_t uwBufferSize)
{
    __IO uint32_t write_pointer = (uint32_t)uwWriteAddress;

    /* While there is data to write */
    for (; uwBufferSize != 0; uwBufferSize--)
    {
        /* Transfer data to the memory */
        *(uint32_t *) (write_pointer) = *pBuffer++;

        /* Increment the address*/
        write_pointer += 4;
    }

}

/**
 * @brief  Inserts a delay time.
 * @param  nCount: specifies the delay time length.
 * @retval None
 */
static void delay(__IO uint32_t nCount)
{
    __IO uint32_t index = 0;
    for(index = (100000 * nCount); index != 0; index--)
    {
    }
}

#if  1

/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line)
{
    /* User can add his own implementation to report the file name and line number,
       ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

    /* Infinite loop */
    while (1)
    {
    }
}
#endif


// ==================File End
