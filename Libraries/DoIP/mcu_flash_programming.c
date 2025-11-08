/******************************************************************************
 * @file mcu_flash_programming.c
 * @brief MCU PFLASH Programming Module Implementation
 * 
 * Based on Infineon's Flash_Programming_1_KIT_TC375_LK example
 ******************************************************************************/

#include "mcu_flash_programming.h"
#include "software_package.h"
#include "IfxFlash.h"
#include "IfxCpu.h"
#include "IfxScuWdt.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* External UART function */
extern void sendUARTMessage(const char *msg, uint32 len);

/******************************************************************************
 * Private Types
 ******************************************************************************/

/* Function pointers for PSPR execution */
typedef struct
{
    void (*eraseSectors)(uint32 sectorAddr, uint32 numSector);
    uint8 (*waitUnbusy)(uint32 flash, IfxFlash_FlashType flashType);
    uint8 (*enterPageMode)(uint32 pageAddr);
    void (*load2X32bits)(uint32 pageAddr, uint32 wordL, uint32 wordU);
    void (*writePage)(uint32 pageAddr);
    void (*eraseFlash)(uint32 sectorAddr, uint32 numSectors);
    void (*writeFlash)(uint32 startingAddr, const uint8 *data, uint32 size);
} MCU_Flash_Functions;

/******************************************************************************
 * Private Variables
 ******************************************************************************/

static MCU_Flash_Functions g_flashFunctions;
static boolean g_is_initialized = FALSE;

/* Buffer for reading SPI Flash (page-aligned) */
static uint8 g_page_buffer[MCU_PFLASH_PAGE_LENGTH];

/******************************************************************************
 * Private Function Prototypes (will be copied to PSPR)
 ******************************************************************************/

static void erasePFLASH_impl(uint32 sectorAddr, uint32 numSectors);
static void writePFLASH_impl(uint32 startingAddr, const uint8 *data, uint32 size);

/******************************************************************************
 * Copy functions to PSPR
 ******************************************************************************/

static void copyFunctionsToPSPR(void)
{
    char msg[128];
    
    sendUARTMessage("\r\n[MCU PFLASH] Copying flash routines to PSPR...\r\n", 51);
    
    /* Copy iLLD functions to PSPR */
    memcpy((void *)MCU_ERASESECTOR_ADDR, (const void *)IfxFlash_eraseMultipleSectors, MCU_ERASESECTOR_LEN);
    g_flashFunctions.eraseSectors = (void *)MCU_ERASESECTOR_ADDR;
    
    memcpy((void *)MCU_WAITUNBUSY_ADDR, (const void *)IfxFlash_waitUnbusy, MCU_WAITUNBUSY_LEN);
    g_flashFunctions.waitUnbusy = (void *)MCU_WAITUNBUSY_ADDR;
    
    memcpy((void *)MCU_ENTERPAGEMODE_ADDR, (const void *)IfxFlash_enterPageMode, MCU_ENTERPAGEMODE_LEN);
    g_flashFunctions.enterPageMode = (void *)MCU_ENTERPAGEMODE_ADDR;
    
    memcpy((void *)MCU_LOAD2X32_ADDR, (const void *)IfxFlash_loadPage2X32, MCU_LOADPAGE2X32_LEN);
    g_flashFunctions.load2X32bits = (void *)MCU_LOAD2X32_ADDR;
    
    memcpy((void *)MCU_WRITEPAGE_ADDR, (const void *)IfxFlash_writePage, MCU_WRITEPAGE_LEN);
    g_flashFunctions.writePage = (void *)MCU_WRITEPAGE_ADDR;
    
    /* Copy our custom functions to PSPR */
    memcpy((void *)MCU_ERASEPFLASH_ADDR, (const void *)erasePFLASH_impl, MCU_ERASEPFLASH_LEN);
    g_flashFunctions.eraseFlash = (void *)MCU_ERASEPFLASH_ADDR;
    
    memcpy((void *)MCU_WRITEPFLASH_ADDR, (const void *)writePFLASH_impl, MCU_WRITEPFLASH_LEN);
    g_flashFunctions.writeFlash = (void *)MCU_WRITEPFLASH_ADDR;
    
    sprintf(msg, "[MCU PFLASH] ✓ Flash routines ready at PSPR (0x%08X)\r\n", MCU_PSPR_START_ADDR);
    sendUARTMessage(msg, strlen(msg));
}

/******************************************************************************
 * Erase PFLASH (executed from PSPR)
 ******************************************************************************/

static void erasePFLASH_impl(uint32 sectorAddr, uint32 numSectors)
{
    /* Get Safety Watchdog password */
    uint16 endInitSafetyPassword = IfxScuWdt_getSafetyWatchdogPasswordInline();
    
    /* Disable EndInit protection and erase */
    IfxScuWdt_clearSafetyEndinitInline(endInitSafetyPassword);
    g_flashFunctions.eraseSectors(sectorAddr, numSectors);
    IfxScuWdt_setSafetyEndinitInline(endInitSafetyPassword);
    
    /* Wait until erase completes */
    g_flashFunctions.waitUnbusy(MCU_FLASH_MODULE, MCU_PROGRAM_FLASH_BANK_A);
}

/******************************************************************************
 * Write PFLASH (executed from PSPR)
 ******************************************************************************/

static void writePFLASH_impl(uint32 startingAddr, const uint8 *data, uint32 size)
{
    uint32 page;
    uint32 offset;
    uint32 num_pages = (size + MCU_PFLASH_PAGE_LENGTH - 1) / MCU_PFLASH_PAGE_LENGTH;
    
    /* Get Safety Watchdog password */
    uint16 endInitSafetyPassword = IfxScuWdt_getSafetyWatchdogPasswordInline();
    
    /* Write all pages */
    for (page = 0; page < num_pages; page++)
    {
        uint32 pageAddr = startingAddr + (page * MCU_PFLASH_PAGE_LENGTH);
        const uint32 *src = (const uint32 *)(data + (page * MCU_PFLASH_PAGE_LENGTH));
        
        /* Enter page mode */
        g_flashFunctions.enterPageMode(pageAddr);
        g_flashFunctions.waitUnbusy(MCU_FLASH_MODULE, MCU_PROGRAM_FLASH_BANK_A);
        
        /* Load 32 bytes (8 double words) into assembly buffer */
        for (offset = 0; offset < MCU_PFLASH_PAGE_LENGTH; offset += 8)
        {
            uint32 wordL = src[offset / 4];
            uint32 wordU = src[offset / 4 + 1];
            g_flashFunctions.load2X32bits(pageAddr, wordL, wordU);
        }
        
        /* Write the page */
        IfxScuWdt_clearSafetyEndinitInline(endInitSafetyPassword);
        g_flashFunctions.writePage(pageAddr);
        IfxScuWdt_setSafetyEndinitInline(endInitSafetyPassword);
        
        /* Wait until write completes */
        g_flashFunctions.waitUnbusy(MCU_FLASH_MODULE, MCU_PROGRAM_FLASH_BANK_A);
    }
}

/******************************************************************************
 * Public Functions
 ******************************************************************************/

MCU_Flash_Result MCU_Flash_Init(void)
{
    g_is_initialized = TRUE;
    
    sendUARTMessage("[MCU PFLASH] Module initialized\r\n", 34);
    
    return MCU_FLASH_OK;
}

MCU_Flash_Result MCU_Flash_EraseSectors(uint32 start_address, uint32 size)
{
    char msg[128];
    
    if (!g_is_initialized)
    {
        return MCU_FLASH_ERROR_INVALID_PARAM;
    }
    
    /* Calculate number of sectors to erase */
    uint32 num_sectors = (size + MCU_PFLASH_SECTOR_SIZE - 1) / MCU_PFLASH_SECTOR_SIZE;
    
    sprintf(msg, "[MCU PFLASH] Erasing %u sectors @ 0x%08X...\r\n", num_sectors, start_address);
    sendUARTMessage(msg, strlen(msg));
    
    /* Copy functions to PSPR */
    copyFunctionsToPSPR();
    
    /* Disable interrupts */
    boolean interruptState = IfxCpu_disableInterrupts();
    
    /* Erase from PSPR */
    g_flashFunctions.eraseFlash(start_address, num_sectors);
    
    /* Restore interrupts */
    IfxCpu_restoreInterrupts(interruptState);
    
    sendUARTMessage("[MCU PFLASH] ✓ Erase complete\r\n", 33);
    
    return MCU_FLASH_OK;
}

MCU_Flash_Result MCU_Flash_WritePages(uint32 target_address, const uint8 *source_data, uint32 size)
{
    char msg[128];
    
    if (!g_is_initialized || source_data == NULL)
    {
        return MCU_FLASH_ERROR_INVALID_PARAM;
    }
    
    sprintf(msg, "[MCU PFLASH] Writing %u bytes @ 0x%08X...\r\n", size, target_address);
    sendUARTMessage(msg, strlen(msg));
    
    /* Copy functions to PSPR if not already done */
    copyFunctionsToPSPR();
    
    /* Disable interrupts */
    boolean interruptState = IfxCpu_disableInterrupts();
    
    /* Write from PSPR */
    g_flashFunctions.writeFlash(target_address, source_data, size);
    
    /* Restore interrupts */
    IfxCpu_restoreInterrupts(interruptState);
    
    sendUARTMessage("[MCU PFLASH] ✓ Write complete\r\n", 33);
    
    return MCU_FLASH_OK;
}

MCU_Flash_Result MCU_Flash_CopyFromSPI(uint32 spi_source, uint32 mcu_target, uint32 size)
{
    char msg[128];
    uint32 bytes_copied = 0;
    uint32 chunk_size = 4096; /* Read 4KB at a time */
    uint8 *transfer_buffer = NULL;
    MCU_Flash_Result result = MCU_FLASH_OK;
    
    /* External flash not available - skip */
    /* if (!g_is_initialized || g_spi_flash == NULL)
    {
        return MCU_FLASH_ERROR_INVALID_PARAM;
    } */
    return MCU_FLASH_ERROR_SPI_READ_FAILED;
    
    sendUARTMessage("\r\n[MCU PFLASH] ========================================\r\n", 54);
    sendUARTMessage("[MCU PFLASH] Starting MCU Flash Programming\r\n", 46);
    sendUARTMessage("[MCU PFLASH] (Vehicle can continue operating)\r\n", 48);
    sendUARTMessage("[MCU PFLASH] ========================================\r\n", 54);
    
    sprintf(msg, "[MCU PFLASH] Source: SPI Flash @ 0x%08X\r\n", spi_source);
    sendUARTMessage(msg, strlen(msg));
    sprintf(msg, "[MCU PFLASH] Target: MCU PFLASH @ 0x%08X\r\n", mcu_target);
    sendUARTMessage(msg, strlen(msg));
    sprintf(msg, "[MCU PFLASH] Size: %u bytes (%u KB)\r\n", size, size / 1024);
    sendUARTMessage(msg, strlen(msg));
    
    /* Allocate transfer buffer (using malloc as temporary solution) */
    transfer_buffer = (uint8 *)malloc(chunk_size);
    if (transfer_buffer == NULL)
    {
        sendUARTMessage("[MCU PFLASH] ERROR: Failed to allocate buffer!\r\n", 49);
        return MCU_FLASH_ERROR_INVALID_PARAM;
    }
    
    /* Step 1: Copy functions to PSPR */
    sendUARTMessage("\r\n[MCU PFLASH] Step 1: Preparing PSPR routines...\r\n", 51);
    copyFunctionsToPSPR();
    
    /* Step 2: Erase target sectors */
    sendUARTMessage("\r\n[MCU PFLASH] Step 2: Erasing target sectors...\r\n", 50);
    result = MCU_Flash_EraseSectors(mcu_target, size);
    if (result != MCU_FLASH_OK)
    {
        free(transfer_buffer);
        sendUARTMessage("[MCU PFLASH] ERROR: Erase failed!\r\n", 36);
        return MCU_FLASH_ERROR_ERASE_FAILED;
    }
    
    /* Step 3: Copy SPI Flash → MCU PFLASH (chunk by chunk) */
    sendUARTMessage("\r\n[MCU PFLASH] Step 3: Copying data...\r\n", 40);
    
    boolean interruptState = IfxCpu_disableInterrupts();
    
    while (bytes_copied < size)
    {
        uint32 remaining = size - bytes_copied;
        uint32 to_copy = (remaining < chunk_size) ? remaining : chunk_size;
        
        /* Read from SPI Flash (DISABLED - no external flash support) */
        /* SPI_Flash_Result spi_result = SPI_Flash_Read(g_spi_flash, 
                                                      spi_source + bytes_copied,
                                                      transfer_buffer,
                                                      to_copy);
        if (spi_result != FLASH_OK)
        {
            IfxCpu_restoreInterrupts(interruptState);
            free(transfer_buffer);
            sendUARTMessage("[MCU PFLASH] ERROR: SPI Flash read failed!\r\n", 45);
            return MCU_FLASH_ERROR_SPI_READ_FAILED;
        } */
        
        /* External flash not available - return error */
        IfxCpu_restoreInterrupts(interruptState);
        free(transfer_buffer);
        return MCU_FLASH_ERROR_SPI_READ_FAILED;
        
        /* Write to MCU PFLASH (from PSPR) */
        g_flashFunctions.writeFlash(mcu_target + bytes_copied, transfer_buffer, to_copy);
        
        bytes_copied += to_copy;
        
        /* Progress indicator every 64KB */
        if ((bytes_copied % 65536) == 0)
        {
            sprintf(msg, "[MCU PFLASH] Progress: %u / %u KB\r\n", 
                    bytes_copied / 1024, size / 1024);
            sendUARTMessage(msg, strlen(msg));
        }
    }
    
    IfxCpu_restoreInterrupts(interruptState);
    
    free(transfer_buffer);
    
    /* Step 4: Write Software Metadata to fixed location (0x100 offset) */
    sendUARTMessage("\r\n[MCU PFLASH] Step 4: Writing metadata...\r\n", 44);
    
    SoftwarePackageHeader metadata;
    /* Read header from SPI Flash (first 64 bytes) - DISABLED */
    /* SPI_Flash_Result spi_result = SPI_Flash_Read(g_spi_flash, spi_source, (uint8*)&metadata, sizeof(metadata));
    if (spi_result != FLASH_OK)
    {
        sendUARTMessage("[MCU PFLASH] ERROR: Failed to read metadata from SPI!\r\n", 56);
        return MCU_FLASH_ERROR_SPI_READ_FAILED;
    } */
    
    /* External flash not available - skip metadata write */
    return MCU_FLASH_ERROR_SPI_READ_FAILED;
    
    /* Verify metadata magic */
    if (metadata.magic == SW_PKG_MAGIC)
    {
        /* Write to MCU PFLASH metadata area (0x100 offset) */
        uint32 metadata_addr = mcu_target + MCU_SW_METADATA_OFFSET;
        result = MCU_Flash_WritePages(metadata_addr, (const uint8*)&metadata, sizeof(metadata));
        
        if (result == MCU_FLASH_OK)
        {
            sprintf(msg, "[MCU PFLASH] ✓ Metadata written to 0x%08X\r\n", metadata_addr);
            sendUARTMessage(msg, strlen(msg));
            sprintf(msg, "[MCU PFLASH]   Version: %d.%d.%d-%d\r\n",
                    metadata.version_major,
                    metadata.version_minor,
                    metadata.version_patch,
                    metadata.version_build);
            sendUARTMessage(msg, strlen(msg));
        }
        else
        {
            sendUARTMessage("[MCU PFLASH] WARNING: Failed to write metadata!\r\n", 50);
        }
    }
    else
    {
        sendUARTMessage("[MCU PFLASH] WARNING: Invalid metadata magic, skipping...\r\n", 60);
    }
    
    sendUARTMessage("\r\n[MCU PFLASH] ========================================\r\n", 54);
    sendUARTMessage("[MCU PFLASH] ✓ MCU Flash Programming Complete!\r\n", 49);
    sendUARTMessage("[MCU PFLASH] ========================================\r\n", 54);
    
    return MCU_FLASH_OK;
}

MCU_Flash_Result MCU_Flash_VerifyCRC32(uint32 address, uint32 size, uint32 expected_crc)
{
    char msg[128];
    
    sendUARTMessage("\r\n[MCU PFLASH] Verifying CRC32...\r\n", 35);
    
    uint32 calculated_crc = SoftwarePackage_CalculateCRC32((const uint8 *)address, size);
    
    sprintf(msg, "[MCU PFLASH] Expected CRC:   0x%08X\r\n", expected_crc);
    sendUARTMessage(msg, strlen(msg));
    sprintf(msg, "[MCU PFLASH] Calculated CRC: 0x%08X\r\n", calculated_crc);
    sendUARTMessage(msg, strlen(msg));
    
    if (calculated_crc == expected_crc)
    {
        sendUARTMessage("[MCU PFLASH] ✓ CRC verification passed!\r\n", 42);
        return MCU_FLASH_OK;
    }
    else
    {
        sendUARTMessage("[MCU PFLASH] ✗ CRC verification FAILED!\r\n", 42);
        return MCU_FLASH_ERROR_VERIFY_FAILED;
    }
}

