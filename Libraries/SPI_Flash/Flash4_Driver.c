/*********************************************************************************************************************/
/*-----------------------------------------------------Includes------------------------------------------------------*/
/*********************************************************************************************************************/
#include "Flash4_Driver.h"
#include "Flash4_Config.h"
#include "IfxQspi_SpiMaster.h"
#include "IfxPort.h"
#include "IfxStm.h"
#include "IfxScuWdt.h"
#include "IfxCpu.h"
#include <string.h>
#include <stdio.h>

/* External UART function */
extern void sendUARTMessage(const char *msg, uint32 len);

/*********************************************************************************************************************/
/*-------------------------------------------------Global variables--------------------------------------------------*/
/*********************************************************************************************************************/

/* QSPI2 SPI Master handle for Flash communication */
static IfxQspi_SpiMaster g_qspiFlash;
static IfxQspi_SpiMaster_Channel g_qspiFlashChannel;

/*********************************************************************************************************************/
/*---------------------------------------------Function Implementations----------------------------------------------*/
/*********************************************************************************************************************/

/* QSPI2 SPI Interrupt Service Routines (Infineon Official Example Method) */
IFX_INTERRUPT(qspi2TxISR, 0, IFX_INTPRIO_QSPI2_TX)
{
    IfxCpu_enableInterrupts();  /* Enable nested interrupts */
    IfxQspi_SpiMaster_isrTransmit(&g_qspiFlash);
}

IFX_INTERRUPT(qspi2RxISR, 0, IFX_INTPRIO_QSPI2_RX)
{
    IfxCpu_enableInterrupts();  /* Enable nested interrupts */
    IfxQspi_SpiMaster_isrReceive(&g_qspiFlash);
}

IFX_INTERRUPT(qspi2ErISR, 0, IFX_INTPRIO_QSPI2_ER)
{
    IfxCpu_enableInterrupts();  /* Enable nested interrupts */
    IfxQspi_SpiMaster_isrError(&g_qspiFlash);
}

void Flash4_Init(void)
{
    char msg[128];
    
    sendUARTMessage("\r\n========================================\r\n", 43);
    sendUARTMessage("Flash4_Init: Start (QSPI2)\r\n", 29);
    sendUARTMessage("========================================\r\n", 41);
    
    /* Step 0: Configure WP# and HOLD# pins FIRST (before QSPI init) */
    sendUARTMessage("Flash4_Init: Configuring control pins (WP#, HOLD#)...\r\n", 57);
    
    /* Configure WP# (P2.8) - Write Protect */
    IfxPort_setPinModeOutput(&MODULE_P02, 8, IfxPort_OutputMode_pushPull, IfxPort_OutputIdx_general);
    IfxPort_setPinPadDriver(&MODULE_P02, 8, IfxPort_PadDriver_cmosAutomotiveSpeed4);
    IfxPort_setPinHigh(&MODULE_P02, 8);  /* WP# HIGH = disabled */
    
    /* Configure HOLD# (P10.7) - Hold */
    IfxPort_setPinModeOutput(&MODULE_P10, 7, IfxPort_OutputMode_pushPull, IfxPort_OutputIdx_general);
    IfxPort_setPinPadDriver(&MODULE_P10, 7, IfxPort_PadDriver_cmosAutomotiveSpeed4);
    IfxPort_setPinHigh(&MODULE_P10, 7);  /* HOLD# HIGH = disabled */
    
    /* Wait for flash to power up (min 10ms) */
    IfxStm_waitTicks(&MODULE_STM0, IfxStm_getTicksFromMilliseconds(&MODULE_STM0, 50));
    
    sendUARTMessage("Flash4_Init: Control pins ready\r\n", 34);
    
    /* Step 1: Initialize QSPI2 Master Module (Infineon Official Example Method) */
    IfxQspi_SpiMaster_Config spiMasterConfig;
    IfxQspi_SpiMaster_initModuleConfig(&spiMasterConfig, &MODULE_QSPI2);
    
    /* ISR priorities and interrupt target (Infineon Official Example Method) */
    spiMasterConfig.txPriority = IFX_INTPRIO_QSPI2_TX;
    spiMasterConfig.rxPriority = IFX_INTPRIO_QSPI2_RX;
    spiMasterConfig.erPriority = IFX_INTPRIO_QSPI2_ER;
    spiMasterConfig.isrProvider = IfxSrc_Tos_cpu0;
    
    /* Pin configuration for mikroBUS */
    const IfxQspi_SpiMaster_Pins pins = {
        &IfxQspi2_SCLK_P15_8_OUT,    /* SCK - P15.8 */
        IfxPort_OutputMode_pushPull,
        &IfxQspi2_MTSR_P15_6_OUT,    /* MOSI - P15.6 */
        IfxPort_OutputMode_pushPull,
        &IfxQspi2_MRSTB_P15_7_IN,    /* MISO - P15.7 (Route B) */
        IfxPort_InputMode_noPullDevice, /* noPull - avoid tri-state pull to GND */
        IfxPort_PadDriver_cmosAutomotiveSpeed3
    };
    spiMasterConfig.pins = &pins;
    
    sendUARTMessage("Flash4_Init: Initializing QSPI2 module...\r\n", 44);
    
    /* Initialize QSPI2 module */
    IfxQspi_SpiMaster_initModule(&g_qspiFlash, &spiMasterConfig);
    
    sendUARTMessage("Flash4_Init: QSPI2 module initialized\r\n", 40);
    
    /* Step 2: Initialize QSPI2 Channel (Infineon Official Example Method) */
    IfxQspi_SpiMaster_ChannelConfig spiMasterChannelConfig;
    IfxQspi_SpiMaster_initChannelConfig(&spiMasterChannelConfig, &g_qspiFlash);
    
    /* Set baudrate for this channel */
    spiMasterChannelConfig.ch.baudrate = FLASH4_BAUDRATE;
    
    /* SPI Mode: CPOL=0 (idle LOW), CPHA=1 (trailing/second edge) - S25FL512S requirement */
    spiMasterChannelConfig.ch.mode.clockPolarity = IfxQspi_ClockPolarity_idleLow;
    spiMasterChannelConfig.ch.mode.shiftClock = IfxQspi_ShiftClock_shiftTransmitDataOnTrailingEdge;
    spiMasterChannelConfig.ch.mode.dataWidth = 8;
    
    /* Hardware CS configuration (SLSO4 = P14.7) - no manual CS! */
    const IfxQspi_SpiMaster_Output slsOutput = {
        &IfxQspi2_SLSO4_P14_7_OUT,
        IfxPort_OutputMode_pushPull,
        IfxPort_PadDriver_cmosAutomotiveSpeed4
    };
    spiMasterChannelConfig.sls.output = slsOutput;
    
    sendUARTMessage("Flash4_Init: Initializing QSPI2 channel...\r\n", 45);
    
    /* Initialize channel */
    IfxQspi_SpiMaster_initChannel(&g_qspiFlashChannel, &spiMasterChannelConfig);
    
    sendUARTMessage("Flash4_Init: QSPI2 channel initialized (Hardware CS - SLSO4)\r\n", 63);
    
    /* Step 3: Test JEDEC ID read - Hardware CS handles CS automatically */
    sendUARTMessage("Flash4_Init: Testing JEDEC ID read (0x9F)...\r\n", 47);
    
    uint8 tx[4] = {0x9F, 0x00, 0x00, 0x00};
    uint8 rx[4] = {0xAA, 0xAA, 0xAA, 0xAA};
    
    /* Hardware CS will toggle automatically during exchange */
    IfxQspi_SpiMaster_exchange(&g_qspiFlashChannel, tx, rx, 4);
    
    while (IfxQspi_SpiMaster_getStatus(&g_qspiFlashChannel) == IfxQspi_Status_busy)
    {
        /* Wait for hardware ISRs to complete */
    }
    
    sprintf(msg, "Flash4_Init: JEDEC ID: 0x%02X 0x%02X 0x%02X\r\n", 
            rx[1], rx[2], rx[3]);
    sendUARTMessage(msg, strlen(msg));
    
    if (rx[1] == FLASH4_MANUFACTURER_ID && 
        rx[2] == FLASH4_DEVICE_ID_MSB && 
        rx[3] == FLASH4_DEVICE_ID_LSB)
    {
        sendUARTMessage("Flash4_Init: Complete! S25FL512S detected (64MB)\r\n", 52);
    }
    else
    {
        sendUARTMessage("Flash4_Init: WARNING - Unexpected JEDEC ID!\r\n", 46);
    }
    
    sendUARTMessage("========================================\r\n\r\n", 43);
}

void Flash4_WriteCommand(uint8 cmd)
{
    /* Hardware CS handles CS automatically */
    IfxQspi_SpiMaster_exchange(&g_qspiFlashChannel, &cmd, NULL_PTR, 1);
    
    while (IfxQspi_SpiMaster_getStatus(&g_qspiFlashChannel) == IfxQspi_Status_busy)
    {
        /* Wait */
    }
}

void Flash4_WriteEnable(void)
{
    uint8 cmd = FLASH4_CMD_WRITE_ENABLE_WREN;
    
    Flash4_WriteCommand(cmd);
    
    /* Wait for Flash to process Write Enable command (tWREN = 1.5us typical) */
    IfxStm_waitTicks(&MODULE_STM0, IfxStm_getTicksFromMicroseconds(&MODULE_STM0, 10));
}

void Flash4_SectorErase(uint32 address)
{
    uint8 txData[4];
    
    Flash4_WriteEnable();
    
    /* Prepare erase command with address */
    txData[0] = FLASH4_CMD_SECTOR_ERASE;
    txData[1] = (uint8)((address >> 16) & 0xFF);
    txData[2] = (uint8)((address >> 8) & 0xFF);
    txData[3] = (uint8)(address & 0xFF);
    
    /* Hardware CS handles CS automatically */
    IfxQspi_SpiMaster_exchange(&g_qspiFlashChannel, txData, NULL_PTR, 4);
    
    while (IfxQspi_SpiMaster_getStatus(&g_qspiFlashChannel) == IfxQspi_Status_busy)
    {
        /* Wait */
    }
}

void Flash4_PageProgram(uint32 address, const uint8 *data, uint16 length)
{
    uint8 txBuffer[260];  /* Command (4) + max data (256) per page */
    uint16 i;
    uint16 pageSize = 256;
    uint16 offset = 0;
    
    /* Write data in 256-byte pages */
    while (offset < length)
    {
        uint16 chunkSize = (length - offset) > pageSize ? pageSize : (length - offset);
        uint16 totalLength = 4 + chunkSize;
        
        Flash4_WriteEnable();
        
        /* Prepare program command with address + data in single buffer */
        txBuffer[0] = FLASH4_CMD_PAGE_PROGRAM;
        txBuffer[1] = (uint8)(((address + offset) >> 16) & 0xFF);
        txBuffer[2] = (uint8)(((address + offset) >> 8) & 0xFF);
        txBuffer[3] = (uint8)((address + offset) & 0xFF);
        
        /* Copy data chunk to buffer */
        for (i = 0; i < chunkSize; i++)
        {
            txBuffer[4 + i] = data[offset + i];
        }
        
        /* Hardware CS handles CS automatically - send everything in single transaction */
        IfxQspi_SpiMaster_exchange(&g_qspiFlashChannel, txBuffer, NULL_PTR, totalLength);
        
        while (IfxQspi_SpiMaster_getStatus(&g_qspiFlashChannel) == IfxQspi_Status_busy)
        {
            /* Wait */
        }
        
        /* Wait for page program to complete (tPP = 0.7ms typical, 3ms max) */
        Flash4_WaitReady(10);
        
        offset += chunkSize;
    }
}

void Flash4_ReadFlash4(uint32 address, uint8 *outData, uint16 nData)
{
    uint8 txBuffer[260];  /* Command (4) + max data (256) per chunk */
    uint8 rxBuffer[260];
    uint16 i;
    uint16 chunkSize = 256;
    uint16 offset = 0;
    
    /* Read data in 256-byte chunks */
    while (offset < nData)
    {
        uint16 readSize = (nData - offset) > chunkSize ? chunkSize : (nData - offset);
        uint16 totalLength = 4 + readSize;
        
        /* Prepare read command with address */
        txBuffer[0] = FLASH4_CMD_READ_FLASH;
        txBuffer[1] = (uint8)(((address + offset) >> 16) & 0xFF);
        txBuffer[2] = (uint8)(((address + offset) >> 8) & 0xFF);
        txBuffer[3] = (uint8)((address + offset) & 0xFF);
        
        /* Fill rest with 0xFF (dummy bytes for read) */
        for (i = 4; i < totalLength; i++)
        {
            txBuffer[i] = 0xFF;
        }
        
        /* Hardware CS handles CS automatically - send command + read data in single transaction */
        IfxQspi_SpiMaster_exchange(&g_qspiFlashChannel, txBuffer, rxBuffer, totalLength);
        
        while (IfxQspi_SpiMaster_getStatus(&g_qspiFlashChannel) == IfxQspi_Status_busy)
        {
            /* Wait */
        }
        
        /* Copy received data (skip first 4 bytes which are command/address echo) */
        for (i = 0; i < readSize; i++)
        {
            outData[offset + i] = rxBuffer[4 + i];
        }
        
        offset += readSize;
    }
}

void Flash4_ReadManufacturerId(uint8 *deviceId)
{
    uint8 txData[4] = {FLASH4_CMD_READ_IDENTIFICATION, 0x00, 0x00, 0x00};
    uint8 rxData[4] = {0xAA, 0xAA, 0xAA, 0xAA};
    
    /* Hardware CS handles CS automatically */
    IfxQspi_SpiMaster_exchange(&g_qspiFlashChannel, txData, rxData, 4);
    
    while (IfxQspi_SpiMaster_getStatus(&g_qspiFlashChannel) == IfxQspi_Status_busy)
    {
        /* Wait */
    }
    
    /* Store response */
    deviceId[0] = rxData[1];  /* Manufacturer ID */
    deviceId[1] = rxData[2];  /* Device ID MSB */
    deviceId[2] = rxData[3];  /* Device ID LSB */
}

uint8 Flash4_ReadStatusReg(void)
{
    uint8 txData[2] = {FLASH4_CMD_READ_STATUS_REG_1, 0x00};
    uint8 rxData[2] = {0xAA, 0xAA};
    
    /* Hardware CS handles CS automatically */
    IfxQspi_SpiMaster_exchange(&g_qspiFlashChannel, txData, rxData, 2);
    
    while (IfxQspi_SpiMaster_getStatus(&g_qspiFlashChannel) == IfxQspi_Status_busy)
    {
        /* Wait */
    }
    
    /* Return full status register byte */
    return rxData[1];
}

boolean Flash4_CheckWIP(void)
{
    uint8 status = Flash4_ReadStatusReg();
    
    /* Check WIP bit (bit 0) */
    return (status & 0x01) != 0;
}

uint8 Flash4_WaitReady(uint32 timeoutMs)
{
    uint32 startTick = IfxStm_get(&MODULE_STM0);
    uint32 timeoutTicks = (uint32)IfxStm_getTicksFromMilliseconds(&MODULE_STM0, timeoutMs);
    
    /* Wait until WIP (Write In Progress) bit is 0 */
    while (Flash4_CheckWIP())
    {
        /* Check timeout */
        if ((IfxStm_get(&MODULE_STM0) - startTick) > timeoutTicks)
        {
            return FLASH4_TIMEOUT;
        }
        
        /* Small delay to avoid excessive polling */
        IfxStm_waitTicks(&MODULE_STM0, IfxStm_getTicksFromMicroseconds(&MODULE_STM0, 100));
    }
    
    return FLASH4_OK;
}
