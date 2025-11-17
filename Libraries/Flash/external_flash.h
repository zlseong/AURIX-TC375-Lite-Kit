/*******************************************************************************
 * @file    external_flash.h
 * @brief   External Flash Driver for Zone Package Storage (SPI/QSPI)
 * @details Supports 32MB ~ 64MB external flash memory via QSPI interface
 * 
 * @version 1.0
 * @date    2024-11-17
 ******************************************************************************/

#ifndef EXTERNAL_FLASH_H
#define EXTERNAL_FLASH_H

#include "Ifx_Types.h"

/*******************************************************************************
 * Configuration - S25FL512S (MIKROE-3191 Flash 4 Click)
 ******************************************************************************/

#define EXTERNAL_FLASH_BASE         0x00000000
#define EXTERNAL_FLASH_SIZE         0x04000000  /* 64MB */
#define EXTERNAL_FLASH_SECTOR_SIZE  0x00040000  /* 256KB sector (S25FL512S) */
#define EXTERNAL_FLASH_PAGE_SIZE    0x00000200  /* 512 bytes page (S25FL512S) */

/* Zone Package Storage */
#define ZONE_PACKAGE_START_ADDR     0x00000000
#define ZONE_PACKAGE_MAX_SIZE       0x02000000  /* 32MB */

/*******************************************************************************
 * Function Prototypes
 ******************************************************************************/

/**
 * @brief Initialize External Flash (QSPI)
 * @return TRUE if success, FALSE otherwise
 */
boolean ExtFlash_Init(void);

/**
 * @brief Erase sectors in external flash
 * @param addr Start address (must be sector-aligned)
 * @param size Size to erase (will be rounded up to sector boundary)
 * @return TRUE if success, FALSE otherwise
 */
boolean ExtFlash_Erase(uint32 addr, uint32 size);

/**
 * @brief Write data to external flash
 * @param addr Start address
 * @param data Pointer to data buffer
 * @param size Size to write
 * @return TRUE if success, FALSE otherwise
 */
boolean ExtFlash_Write(uint32 addr, const uint8 *data, uint32 size);

/**
 * @brief Read data from external flash
 * @param addr Start address
 * @param buffer Pointer to output buffer
 * @param size Size to read
 * @return TRUE if success, FALSE otherwise
 */
boolean ExtFlash_Read(uint32 addr, uint8 *buffer, uint32 size);

/**
 * @brief Calculate CRC32 for data in external flash
 * @param addr Start address
 * @param size Size to calculate
 * @return CRC32 value
 */
uint32 ExtFlash_CalculateCRC32(uint32 addr, uint32 size);

/**
 * @brief Get external flash status
 * @return TRUE if ready, FALSE if busy
 */
boolean ExtFlash_IsReady(void);

#endif /* EXTERNAL_FLASH_H */

