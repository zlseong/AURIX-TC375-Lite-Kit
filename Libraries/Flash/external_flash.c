/*******************************************************************************
 * @file    external_flash.c
 * @brief   External Flash Driver Implementation (S25FL512S via QSPI2)
 * @details Uses Flash4_Driver (MIKROE-3191 Flash 4 Click) for external memory
 * 
 * @version 1.0
 * @date    2024-11-17
 ******************************************************************************/

#include "external_flash.h"
#include "Flash4_Driver.h"
#include "UART_Logging.h"
#include <string.h>
#include <stdio.h>

/*******************************************************************************
 * Configuration
 ******************************************************************************/

static boolean g_ext_flash_initialized = FALSE;

/* S25FL512S Sector Size: 256KB (0x40000) */
#define FLASH4_SECTOR_SIZE  0x00040000  /* 256KB */

/*******************************************************************************
 * CRC32 Helper
 ******************************************************************************/

static uint32 CRC32_Calculate(const uint8 *data, uint32 size)
{
    uint32 crc = 0xFFFFFFFF;
    uint32 i, j;
    
    for (i = 0; i < size; i++)
    {
        crc ^= data[i];
        for (j = 0; j < 8; j++)
        {
            if (crc & 1)
            {
                crc = (crc >> 1) ^ 0xEDB88320;
            }
            else
            {
                crc >>= 1;
            }
        }
    }
    
    return ~crc;
}

/*******************************************************************************
 * Public Functions
 ******************************************************************************/

boolean ExtFlash_Init(void)
{
    if (g_ext_flash_initialized)
    {
        return TRUE;
    }
    
    /* Initialize Flash4 Driver (QSPI2 + S25FL512S) */
    Flash4_Init();
    
    g_ext_flash_initialized = TRUE;
    
    sendUARTMessage("[ExtFlash] S25FL512S ready (64MB) for Zone Package storage\r\n", 61);
    return TRUE;
}

boolean ExtFlash_Erase(uint32 addr, uint32 size)
{
    uint32 sector_count;
    uint32 i;
    uint32 current_addr;
    char log_msg[80];
    
    if (!g_ext_flash_initialized)
    {
        sendUARTMessage("[ExtFlash] ERROR: Not initialized\r\n", 36);
        return FALSE;
    }
    
    if (addr + size > EXTERNAL_FLASH_SIZE)
    {
        sendUARTMessage("[ExtFlash] ERROR: Address out of range\r\n", 41);
        return FALSE;
    }
    
    /* Calculate number of sectors to erase (S25FL512S: 256KB sectors) */
    sector_count = (size + FLASH4_SECTOR_SIZE - 1) / FLASH4_SECTOR_SIZE;
    
    sprintf(log_msg, "[ExtFlash] Erasing %u sectors (%u KB)...\r\n", 
            sector_count, sector_count * 256);
    sendUARTMessage(log_msg, strlen(log_msg));
    
    for (i = 0; i < sector_count; i++)
    {
        current_addr = addr + (i * FLASH4_SECTOR_SIZE);
        
        /* Erase sector */
        Flash4_SectorErase(current_addr);
        
        /* Wait for erase to complete */
        if (Flash4_WaitReady(5000) != FLASH4_OK)
        {
            sprintf(log_msg, "[ExtFlash] ERROR: Sector erase timeout at 0x%08X\r\n", current_addr);
            sendUARTMessage(log_msg, strlen(log_msg));
            return FALSE;
        }
        
        /* Progress indicator */
        if ((i % 10) == 0 || i == (sector_count - 1))
        {
            sprintf(log_msg, "[ExtFlash] Erase progress: %u / %u\r\n", i + 1, sector_count);
            sendUARTMessage(log_msg, strlen(log_msg));
        }
    }
    
    sendUARTMessage("[ExtFlash] Erase complete\r\n", 27);
    return TRUE;
}

boolean ExtFlash_Write(uint32 addr, const uint8 *data, uint32 size)
{
    uint32 page_count;
    uint32 i;
    uint32 current_addr;
    uint32 bytes_remaining;
    uint32 chunk_size;
    char log_msg[80];
    
    if (!g_ext_flash_initialized)
    {
        sendUARTMessage("[ExtFlash] ERROR: Not initialized\r\n", 36);
        return FALSE;
    }
    
    if (addr + size > EXTERNAL_FLASH_SIZE)
    {
        sendUARTMessage("[ExtFlash] ERROR: Write out of range\r\n", 39);
        return FALSE;
    }
    
    /* Write in pages (512 bytes per page for S25FL512S) */
    page_count = (size + FLASH4_MAX_PAGE_SIZE - 1) / FLASH4_MAX_PAGE_SIZE;
    bytes_remaining = size;
    
    sprintf(log_msg, "[ExtFlash] Writing %u bytes (%u pages)...\r\n", size, page_count);
    sendUARTMessage(log_msg, strlen(log_msg));
    
    for (i = 0; i < page_count; i++)
    {
        current_addr = addr + (i * FLASH4_MAX_PAGE_SIZE);
        chunk_size = (bytes_remaining > FLASH4_MAX_PAGE_SIZE) ? FLASH4_MAX_PAGE_SIZE : bytes_remaining;
        
        /* Program page */
        Flash4_PageProgram(current_addr, &data[i * FLASH4_MAX_PAGE_SIZE], (uint16)chunk_size);
        
        /* Wait for write to complete */
        if (Flash4_WaitReady(1000) != FLASH4_OK)
        {
            sprintf(log_msg, "[ExtFlash] ERROR: Write timeout at 0x%08X\r\n", current_addr);
            sendUARTMessage(log_msg, strlen(log_msg));
            return FALSE;
        }
        
        bytes_remaining -= chunk_size;
        
        /* Progress indicator (every 1MB) */
        if ((i % 2048) == 0 || i == (page_count - 1))
        {
            uint32 progress = ((i + 1) * 100) / page_count;
            sprintf(log_msg, "[ExtFlash] Write progress: %u%%\r\n", progress);
            sendUARTMessage(log_msg, strlen(log_msg));
        }
    }
    
    sendUARTMessage("[ExtFlash] Write complete\r\n", 27);
    return TRUE;
}

boolean ExtFlash_Read(uint32 addr, uint8 *buffer, uint32 size)
{
    if (!g_ext_flash_initialized)
    {
        sendUARTMessage("[ExtFlash] ERROR: Not initialized\r\n", 36);
        return FALSE;
    }
    
    if (addr + size > EXTERNAL_FLASH_SIZE)
    {
        sendUARTMessage("[ExtFlash] ERROR: Read out of range\r\n", 38);
        return FALSE;
    }
    
    /* Read from Flash4 */
    Flash4_ReadFlash4(addr, buffer, (uint16)size);
    
    return TRUE;
}

uint32 ExtFlash_CalculateCRC32(uint32 addr, uint32 size)
{
    uint32 crc;
    uint32 chunk_size;
    uint32 offset;
    uint8 buffer[4096];
    char log_msg[64];
    
    if (!g_ext_flash_initialized)
    {
        return 0;
    }
    
    if (addr + size > EXTERNAL_FLASH_SIZE)
    {
        return 0;
    }
    
    sendUARTMessage("[ExtFlash] Calculating CRC32...\r\n", 33);
    
    crc = 0xFFFFFFFF;
    
    for (offset = 0; offset < size; offset += 4096)
    {
        chunk_size = (size - offset > 4096) ? 4096 : (size - offset);
        Flash4_ReadFlash4(addr + offset, buffer, (uint16)chunk_size);
        
        /* Update CRC incrementally */
        crc = CRC32_Calculate(buffer, chunk_size);
        
        /* Progress indicator (every 1MB) */
        if ((offset % (1024 * 1024)) == 0)
        {
            uint32 progress = (offset * 100) / size;
            sprintf(log_msg, "[ExtFlash] CRC32 progress: %u%%\r\n", progress);
            sendUARTMessage(log_msg, strlen(log_msg));
        }
    }
    
    sprintf(log_msg, "[ExtFlash] CRC32: 0x%08X\r\n", crc);
    sendUARTMessage(log_msg, strlen(log_msg));
    
    return crc;
}

boolean ExtFlash_IsReady(void)
{
    if (!g_ext_flash_initialized)
    {
        return FALSE;
    }
    
    /* Check if Flash4 is ready (WIP bit) */
    return !Flash4_CheckWIP();
}

