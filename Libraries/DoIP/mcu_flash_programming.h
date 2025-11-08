/******************************************************************************
 * @file mcu_flash_programming.h
 * @brief MCU PFLASH Programming Module (for ZGW self-update)
 * 
 * This module implements MCU internal PFLASH programming based on Infineon's
 * official Flash Programming example (Flash_Programming_1_KIT_TC375_LK).
 * 
 * Key Features:
 * - PSPR (Program Scratch-Pad RAM) execution for safety
 * - Safety Watchdog (EndInit) handling
 * - Interrupt protection during flash operations
 * - SPI Flash to MCU PFLASH copy with verification
 * 
 * Reference: AURIX_code_examples/Flash_Programming_1_KIT_TC375_LK
 ******************************************************************************/

#ifndef MCU_FLASH_PROGRAMMING_H
#define MCU_FLASH_PROGRAMMING_H

#include "Ifx_Types.h"
/* Flash4 driver removed - MCU Flash Programming uses direct methods */

/******************************************************************************
 * Constants
 ******************************************************************************/

/* PFLASH Page Configuration */
#define MCU_PFLASH_PAGE_LENGTH      32          /* 32 bytes per page */
#define MCU_PFLASH_SECTOR_SIZE      0x00004000  /* 16KB per sector */

/* Flash Module Selection */
#define MCU_FLASH_MODULE            0           /* PMU0 */
#define MCU_PROGRAM_FLASH_BANK_A    IfxFlash_FlashType_P0
#define MCU_PROGRAM_FLASH_BANK_B    IfxFlash_FlashType_P1

/* PSPR (Program Scratch-Pad RAM) Configuration */
/* Note: Functions must execute from PSPR when programming PFLASH */
#define MCU_PSPR_START_ADDR         0x70100000U /* CPU0 PSPR base */

/* Reserved space for flash routines in PSPR (in bytes) */
#define MCU_ERASESECTOR_LEN         110
#define MCU_WAITUNBUSY_LEN          110
#define MCU_ENTERPAGEMODE_LEN       110
#define MCU_LOADPAGE2X32_LEN        110
#define MCU_WRITEPAGE_LEN           110
#define MCU_ERASEPFLASH_LEN         0x200
#define MCU_WRITEPFLASH_LEN         0x300

/* PSPR function relocation addresses */
#define MCU_ERASESECTOR_ADDR        (MCU_PSPR_START_ADDR)
#define MCU_WAITUNBUSY_ADDR         (MCU_ERASESECTOR_ADDR + MCU_ERASESECTOR_LEN)
#define MCU_ENTERPAGEMODE_ADDR      (MCU_WAITUNBUSY_ADDR + MCU_WAITUNBUSY_LEN)
#define MCU_LOAD2X32_ADDR           (MCU_ENTERPAGEMODE_ADDR + MCU_ENTERPAGEMODE_LEN)
#define MCU_WRITEPAGE_ADDR          (MCU_LOAD2X32_ADDR + MCU_LOADPAGE2X32_LEN)
#define MCU_ERASEPFLASH_ADDR        (MCU_WRITEPAGE_ADDR + MCU_WRITEPAGE_LEN)
#define MCU_WRITEPFLASH_ADDR        (MCU_ERASEPFLASH_ADDR + MCU_ERASEPFLASH_LEN)

/******************************************************************************
 * Type Definitions
 ******************************************************************************/

/* Result codes */
typedef enum
{
    MCU_FLASH_OK = 0,
    MCU_FLASH_ERROR_INVALID_PARAM,
    MCU_FLASH_ERROR_ERASE_FAILED,
    MCU_FLASH_ERROR_WRITE_FAILED,
    MCU_FLASH_ERROR_VERIFY_FAILED,
    MCU_FLASH_ERROR_SPI_READ_FAILED
} MCU_Flash_Result;

/******************************************************************************
 * Public Functions
 ******************************************************************************/

/**
 * @brief Initialize MCU Flash Programming Module
 * @return MCU_FLASH_OK if successful
 */
MCU_Flash_Result MCU_Flash_Init(void);

/**
 * @brief Copy software from SPI Flash to MCU PFLASH
 * 
 * This function performs the following steps:
 * 1. Copy flash routines to PSPR (for safe execution)
 * 2. Disable interrupts
 * 3. Erase target MCU PFLASH sectors
 * 4. Read from SPI Flash page-by-page
 * 5. Write to MCU PFLASH page-by-page
 * 6. Restore interrupts
 * 7. Verify written data with CRC32
 * 
 * @param spi_source SPI Flash source address
 * @param mcu_target MCU PFLASH target address (Bank A or B)
 * @param size Software size in bytes
 * @return MCU_FLASH_OK if successful, error code otherwise
 */
MCU_Flash_Result MCU_Flash_CopyFromSPI(uint32 spi_source, uint32 mcu_target, uint32 size);

/**
 * @brief Erase MCU PFLASH sectors
 * @param start_address Start address (must be sector-aligned)
 * @param size Size to erase (rounded up to sector boundary)
 * @return MCU_FLASH_OK if successful
 */
MCU_Flash_Result MCU_Flash_EraseSectors(uint32 start_address, uint32 size);

/**
 * @brief Write data to MCU PFLASH
 * @param target_address Target address in MCU PFLASH
 * @param source_data Source data buffer (in RAM)
 * @param size Size to write (must be page-aligned)
 * @return MCU_FLASH_OK if successful
 */
MCU_Flash_Result MCU_Flash_WritePages(uint32 target_address, const uint8 *source_data, uint32 size);

/**
 * @brief Verify MCU PFLASH data with CRC32
 * @param address MCU PFLASH address to verify
 * @param size Size to verify
 * @param expected_crc Expected CRC32 value
 * @return MCU_FLASH_OK if CRC matches, MCU_FLASH_ERROR_VERIFY_FAILED otherwise
 */
MCU_Flash_Result MCU_Flash_VerifyCRC32(uint32 address, uint32 size, uint32 expected_crc);

#endif /* MCU_FLASH_PROGRAMMING_H */

