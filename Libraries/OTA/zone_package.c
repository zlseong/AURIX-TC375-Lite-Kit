/*******************************************************************************
 * @file    zone_package.c
 * @brief   Zone Package Management Implementation
 * @details Parse, validate, and extract ECU packages from Zone OTA package
 * 
 * @version 1.0
 * @date    2024-11-17
 ******************************************************************************/

#include "zone_package.h"
#include "external_flash.h"
#include "UART_Logging.h"
#include <string.h>
#include <stdio.h>

/*******************************************************************************
 * Public Functions
 ******************************************************************************/

boolean ZonePackage_ParseHeader(uint32 addr, ZonePackageHeader_t *header)
{
    char log_msg[128];
    
    /* Read Zone Package Header (1KB) from External Flash */
    if (!ExtFlash_Read(addr, (uint8*)header, sizeof(ZonePackageHeader_t)))
    {
        sendUARTMessage("[ZonePkg] ERROR: Failed to read header\r\n", 42);
        return FALSE;
    }
    
    /* Validate magic number */
    if (header->magic_number != ZONE_PACKAGE_MAGIC)
    {
        sprintf(log_msg, "[ZonePkg] ERROR: Invalid magic 0x%08X\r\n", header->magic_number);
        sendUARTMessage(log_msg, strlen(log_msg));
        return FALSE;
    }
    
    sprintf(log_msg, "[ZonePkg] Valid header found: %s\r\n", header->zone_name);
    sendUARTMessage(log_msg, strlen(log_msg));
    
    return TRUE;
}

boolean ZonePackage_FindECUMetadata(const ZonePackageHeader_t *zone_header, 
                                     const char *ecu_id, 
                                     ECUMetadata_t *metadata)
{
    uint32 i;
    uint32 meta_addr;
    char log_msg[128];
    
    /* Search ECU Table */
    for (i = 0; i < zone_header->package_count; i++)
    {
        if (strcmp(zone_header->ecu_table[i].ecu_id, ecu_id) == 0)
        {
            /* Found! Read ECU Metadata from External Flash */
            meta_addr = ZONE_PACKAGE_START_ADDR + zone_header->ecu_table[i].offset;
            
            if (!ExtFlash_Read(meta_addr, (uint8*)metadata, sizeof(ECUMetadata_t)))
            {
                sprintf(log_msg, "[ZonePkg] ERROR: Failed to read %s metadata\r\n", ecu_id);
                sendUARTMessage(log_msg, strlen(log_msg));
                return FALSE;
            }
            
            /* Validate ECU Metadata magic */
            if (metadata->magic_number != ECU_METADATA_MAGIC)
            {
                sprintf(log_msg, "[ZonePkg] ERROR: %s metadata invalid\r\n", ecu_id);
                sendUARTMessage(log_msg, strlen(log_msg));
                return FALSE;
            }
            
            sprintf(log_msg, "[ZonePkg] Found %s - v%u.%u.%u\r\n",
                    ecu_id,
                    (metadata->firmware_version >> 16) & 0xFF,
                    (metadata->firmware_version >> 8) & 0xFF,
                    metadata->firmware_version & 0xFF);
            sendUARTMessage(log_msg, strlen(log_msg));
            
            return TRUE;
        }
    }
    
    sprintf(log_msg, "[ZonePkg] ERROR: %s not found in package\r\n", ecu_id);
    sendUARTMessage(log_msg, strlen(log_msg));
    return FALSE;
}

boolean ZonePackage_ValidateCRC(const ZonePackageHeader_t *zone_header)
{
    uint32 calculated_crc;
    char log_msg[64];
    
    sendUARTMessage("[ZonePkg] Validating CRC32...\r\n", 32);
    
    /* Calculate CRC32 of Zone Package (excluding header's own CRC field) */
    /* Start after CRC field (offset 0x100) to end of package */
    calculated_crc = ExtFlash_CalculateCRC32(
        ZONE_PACKAGE_START_ADDR + 0x100,
        zone_header->total_size - 0x100
    );
    
    if (calculated_crc == zone_header->zone_crc32)
    {
        sprintf(log_msg, "[ZonePkg] ✅ CRC32 valid: 0x%08X\r\n", calculated_crc);
        sendUARTMessage(log_msg, strlen(log_msg));
        return TRUE;
    }
    else
    {
        sprintf(log_msg, "[ZonePkg] ❌ CRC32 mismatch: calc=0x%08X, expect=0x%08X\r\n",
                calculated_crc, zone_header->zone_crc32);
        sendUARTMessage(log_msg, strlen(log_msg));
        return FALSE;
    }
}

void ZonePackage_PrintInfo(const ZonePackageHeader_t *zone_header)
{
    uint32 i, j;
    ECUMetadata_t ecu_meta;
    char log_msg[128];
    
    sendUARTMessage("\r\n========== Zone Package Info ==========\r\n", 43);
    
    sprintf(log_msg, "Zone Name:     %s\r\n", zone_header->zone_name);
    sendUARTMessage(log_msg, strlen(log_msg));
    
    sprintf(log_msg, "Total Size:    %u bytes (%u MB)\r\n", 
            zone_header->total_size, zone_header->total_size / (1024 * 1024));
    sendUARTMessage(log_msg, strlen(log_msg));
    
    sprintf(log_msg, "Package Count: %u\r\n", zone_header->package_count);
    sendUARTMessage(log_msg, strlen(log_msg));
    
    sprintf(log_msg, "CRC32:         0x%08X\r\n", zone_header->zone_crc32);
    sendUARTMessage(log_msg, strlen(log_msg));
    
    sendUARTMessage("\r\n========== ECU Table ==========\r\n", 34);
    
    for (i = 0; i < zone_header->package_count; i++)
    {
        sprintf(log_msg, "\r\n[%u] %s\r\n", i, zone_header->ecu_table[i].ecu_id);
        sendUARTMessage(log_msg, strlen(log_msg));
        
        sprintf(log_msg, "    Version:  v%u.%u.%u\r\n",
                (zone_header->ecu_table[i].firmware_version >> 16) & 0xFF,
                (zone_header->ecu_table[i].firmware_version >> 8) & 0xFF,
                zone_header->ecu_table[i].firmware_version & 0xFF);
        sendUARTMessage(log_msg, strlen(log_msg));
        
        sprintf(log_msg, "    Offset:   0x%08X\r\n", zone_header->ecu_table[i].offset);
        sendUARTMessage(log_msg, strlen(log_msg));
        
        sprintf(log_msg, "    Size:     %u bytes\r\n", zone_header->ecu_table[i].size);
        sendUARTMessage(log_msg, strlen(log_msg));
        
        sprintf(log_msg, "    Priority: %u\r\n", zone_header->ecu_table[i].priority);
        sendUARTMessage(log_msg, strlen(log_msg));
        
        /* Read and display dependencies */
        if (ZonePackage_FindECUMetadata(zone_header, zone_header->ecu_table[i].ecu_id, &ecu_meta))
        {
            if (ecu_meta.dependency_count > 0)
            {
                sendUARTMessage("    Dependencies:\r\n", 19);
                for (j = 0; j < ecu_meta.dependency_count; j++)
                {
                    sprintf(log_msg, "      - %s >= v%u.%u.%u\r\n",
                            ecu_meta.dependencies[j].ecu_id,
                            (ecu_meta.dependencies[j].min_version >> 16) & 0xFF,
                            (ecu_meta.dependencies[j].min_version >> 8) & 0xFF,
                            ecu_meta.dependencies[j].min_version & 0xFF);
                    sendUARTMessage(log_msg, strlen(log_msg));
                }
            }
        }
    }
    
    sendUARTMessage("\r\n========================================\r\n\r\n", 44);
}

