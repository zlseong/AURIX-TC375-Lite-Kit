/**********************************************************************************************************************
 * \file Flash4_Driver.h
 * \copyright Copyright (C) Infineon Technologies AG 2019
 * 
 * MIKROE-3191 Flash 4 Click Driver for TC375 Lite Kit
 * This driver provides interface to external flash memory via QSPI
 *********************************************************************************************************************/

#ifndef FLASH4_DRIVER_H_
#define FLASH4_DRIVER_H_

/*********************************************************************************************************************/
/*-----------------------------------------------------Includes------------------------------------------------------*/
/*********************************************************************************************************************/
#include "Ifx_Types.h"
#include "IfxQspi_SpiMaster.h"
#include "IfxPort.h"
#include "Flash4_Config.h"

/*********************************************************************************************************************/
/*------------------------------------------------------Macros-------------------------------------------------------*/
/*********************************************************************************************************************/
/* FLASH4 Commands */
#define FLASH4_CMD_READ_IDENTIFICATION           0x9F
#define FLASH4_CMD_READ_STATUS_REG_1             0x05
#define FLASH4_CMD_WRITE_ENABLE_WREN             0x06
#define FLASH4_CMD_WRITE_DISABLE_WRDI            0x04
#define FLASH4_CMD_READ_FLASH                    0x03
#define FLASH4_CMD_PAGE_PROGRAM                  0x02
#define FLASH4_CMD_SECTOR_ERASE                  0xD8

/* Flash device IDs (per S25FL512S datasheet Table 50) */
#define FLASH4_MANUFACTURER_ID                   0x01    /* Spansion/Cypress (correct JEDEC ID!) */
#define FLASH4_DEVICE_ID_MSB                     0x02    /* 512 Mb - Most Significant Byte */
#define FLASH4_DEVICE_ID_LSB                     0x20    /* 512 Mb - Least Significant Byte */

/* Configuration */
#define FLASH4_MAX_PAGE_SIZE                     256

/* Return values */
#define FLASH4_OK                                0
#define FLASH4_TIMEOUT                           3

/*********************************************************************************************************************/
/*------------------------------------------------Function Prototypes------------------------------------------------*/
/*********************************************************************************************************************/

/**
 * \brief Initialize Flash4 QSPI interface
 * \return None
 */
void Flash4_Init(void);

/**
 * \brief Write a command to the flash
 * \param cmd Command byte
 */
void Flash4_WriteCommand(uint8 cmd);

/**
 * \brief Read manufacturer and device ID
 * \param deviceId Output buffer (3 bytes: Manufacturer ID, Device ID MSB, Device ID LSB)
 */
void Flash4_ReadManufacturerId(uint8 *deviceId);

/**
 * \brief Read flash memory
 * \param address Start address (32-bit)
 * \param outData Output buffer
 * \param nData Number of bytes to read
 */
void Flash4_ReadFlash4(uint32 address, uint8 *outData, uint16 nData);

/**
 * \brief Write data to flash memory (page program)
 * \param address Start address (32-bit)
 * \param data Input data buffer
 * \param length Number of bytes to write (max 256)
 */
void Flash4_PageProgram(uint32 address, const uint8 *data, uint16 length);

/**
 * \brief Erase a sector
 * \param address Sector address (32-bit)
 */
void Flash4_SectorErase(uint32 address);

/**
 * \brief Enable write operations
 */
void Flash4_WriteEnable(void);

/**
 * \brief Check if Write In Progress (WIP) bit is set
 * \return TRUE if busy, FALSE if ready
 */
boolean Flash4_CheckWIP(void);

/**
 * \brief Read Status Register 1
 * \return Status register byte (WIP, WEL, BP bits)
 */
uint8 Flash4_ReadStatusReg(void);

/**
 * \brief Wait for flash operation to complete
 * \param timeoutMs Timeout in milliseconds
 * \return FLASH4_OK if ready, FLASH4_TIMEOUT if timeout
 */
uint8 Flash4_WaitReady(uint32 timeoutMs);

#endif /* FLASH4_DRIVER_H_ */
