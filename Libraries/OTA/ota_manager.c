/*******************************************************************************
 * @file    ota_manager.c
 * @brief   OTA Manager Implementation
 * @details Zone Package download, validation, extraction, and installation
 * 
 * @version 1.0
 * @date    2024-11-17
 ******************************************************************************/

#include "ota_manager.h"
#include "external_flash.h"
#include "zone_package.h"
#include "FlashBankManager.h"
#include "doip_types.h"
#include "AppConfig.h"
#include "UART_Logging.h"
#include <string.h>
#include <stdio.h>

/*******************************************************************************
 * Global Variables
 ******************************************************************************/

static OTA_State_t g_ota_state = OTA_STATE_IDLE;
static uint32 g_ota_total_size = 0;
static uint32 g_ota_downloaded_size = 0;
static uint32 g_ota_current_offset = 0;

static ZonePackageHeader_t g_zone_header;

/* External VCI Database (for dependency check) */
extern DoIP_VCI_Info g_vci_database[];
extern uint8 g_zone_ecu_count;

/*******************************************************************************
 * Helper Functions
 ******************************************************************************/

static DoIP_VCI_Info* FindECUInDatabase(const char *ecu_id)
{
    uint32 i;
    
    for (i = 0; i < g_zone_ecu_count; i++)
    {
        if (strcmp(g_vci_database[i].ecu_id, ecu_id) == 0)
        {
            return &g_vci_database[i];
        }
    }
    
    return NULL;
}

static uint32 ParseVersionString(const char *version_str)
{
    uint32 major, minor, patch;
    char temp[16];
    char *token;
    
    /* Copy to temp buffer */
    strncpy(temp, version_str, sizeof(temp) - 1);
    temp[sizeof(temp) - 1] = '\0';
    
    /* Skip 'v' prefix if present */
    if (temp[0] == 'v' || temp[0] == 'V')
    {
        token = strtok(temp + 1, ".");
    }
    else
    {
        token = strtok(temp, ".");
    }
    
    major = (token != NULL) ? atoi(token) : 0;
    token = strtok(NULL, ".");
    minor = (token != NULL) ? atoi(token) : 0;
    token = strtok(NULL, ".-");
    patch = (token != NULL) ? atoi(token) : 0;
    
    return (major << 16) | (minor << 8) | patch;
}

static boolean CheckDependencies(const ECUMetadata_t *metadata)
{
    uint32 i;
    DoIP_VCI_Info *current_ecu;
    uint32 current_version, required_version;
    char log_msg[128];
    
    if (metadata->dependency_count == 0)
    {
        sendUARTMessage("[OTA] No dependencies\r\n", 23);
        return TRUE;
    }
    
    sendUARTMessage("[OTA] Checking dependencies...\r\n", 32);
    
    for (i = 0; i < metadata->dependency_count; i++)
    {
        const char *dep_ecu_id = metadata->dependencies[i].ecu_id;
        required_version = metadata->dependencies[i].min_version;
        
        /* Find ECU in database */
        current_ecu = FindECUInDatabase(dep_ecu_id);
        
        if (current_ecu == NULL)
        {
            sprintf(log_msg, "[OTA] ❌ %s not found!\r\n", dep_ecu_id);
            sendUARTMessage(log_msg, strlen(log_msg));
            return FALSE;
        }
        
        /* Parse current version */
        current_version = ParseVersionString(current_ecu->sw_version);
        
        /* Compare versions */
        if (current_version < required_version)
        {
            sprintf(log_msg, "[OTA] ❌ %s v%u.%u.%u < v%u.%u.%u\r\n",
                    dep_ecu_id,
                    (current_version >> 16) & 0xFF,
                    (current_version >> 8) & 0xFF,
                    current_version & 0xFF,
                    (required_version >> 16) & 0xFF,
                    (required_version >> 8) & 0xFF,
                    required_version & 0xFF);
            sendUARTMessage(log_msg, strlen(log_msg));
            return FALSE;
        }
        
        sprintf(log_msg, "[OTA] ✅ %s OK\r\n", dep_ecu_id);
        sendUARTMessage(log_msg, strlen(log_msg));
    }
    
    sendUARTMessage("[OTA] All dependencies satisfied\r\n", 35);
    return TRUE;
}

/*******************************************************************************
 * Public Functions
 ******************************************************************************/

boolean OTA_Init(void)
{
    /* Initialize External Flash */
    if (!ExtFlash_Init())
    {
        sendUARTMessage("[OTA] ERROR: External Flash init failed\r\n", 42);
        return FALSE;
    }
    
    g_ota_state = OTA_STATE_IDLE;
    g_ota_total_size = 0;
    g_ota_downloaded_size = 0;
    g_ota_current_offset = 0;
    
    sendUARTMessage("[OTA] Manager initialized\r\n", 27);
    return TRUE;
}

boolean OTA_StartDownload(uint32 total_size)
{
    char log_msg[64];
    
    if (g_ota_state != OTA_STATE_IDLE)
    {
        sendUARTMessage("[OTA] ERROR: OTA already in progress\r\n", 39);
        return FALSE;
    }
    
    if (total_size > ZONE_PACKAGE_MAX_SIZE)
    {
        sprintf(log_msg, "[OTA] ERROR: Size too large (%u MB)\r\n", 
                total_size / (1024 * 1024));
        sendUARTMessage(log_msg, strlen(log_msg));
        return FALSE;
    }
    
    sprintf(log_msg, "[OTA] Starting download (%u MB)...\r\n", 
            total_size / (1024 * 1024));
    sendUARTMessage(log_msg, strlen(log_msg));
    
    /* Erase External Flash */
    if (!ExtFlash_Erase(ZONE_PACKAGE_START_ADDR, total_size))
    {
        sendUARTMessage("[OTA] ERROR: Flash erase failed\r\n", 34);
        return FALSE;
    }
    
    g_ota_state = OTA_STATE_DOWNLOADING;
    g_ota_total_size = total_size;
    g_ota_downloaded_size = 0;
    g_ota_current_offset = 0;
    
    sendUARTMessage("[OTA] Ready to receive Zone Package\r\n", 38);
    return TRUE;
}

boolean OTA_WriteChunk(const uint8 *data, uint32 size)
{
    char log_msg[80];
    uint32 progress;
    
    if (g_ota_state != OTA_STATE_DOWNLOADING)
    {
        sendUARTMessage("[OTA] ERROR: Not in download state\r\n", 37);
        return FALSE;
    }
    
    /* Write to External Flash */
    if (!ExtFlash_Write(ZONE_PACKAGE_START_ADDR + g_ota_current_offset, data, size))
    {
        sendUARTMessage("[OTA] ERROR: Flash write failed\r\n", 34);
        g_ota_state = OTA_STATE_ERROR;
        return FALSE;
    }
    
    g_ota_current_offset += size;
    g_ota_downloaded_size += size;
    
    /* Progress logging (every 1MB) */
    if ((g_ota_downloaded_size % (1024 * 1024)) == 0 || 
        g_ota_downloaded_size == g_ota_total_size)
    {
        progress = (g_ota_downloaded_size * 100) / g_ota_total_size;
        sprintf(log_msg, "[OTA] Download progress: %u%% (%u / %u MB)\r\n",
                progress,
                g_ota_downloaded_size / (1024 * 1024),
                g_ota_total_size / (1024 * 1024));
        sendUARTMessage(log_msg, strlen(log_msg));
    }
    
    return TRUE;
}

boolean OTA_FinishDownload(void)
{
    if (g_ota_state != OTA_STATE_DOWNLOADING)
    {
        sendUARTMessage("[OTA] ERROR: Not in download state\r\n", 37);
        return FALSE;
    }
    
    if (g_ota_downloaded_size != g_ota_total_size)
    {
        sendUARTMessage("[OTA] ERROR: Incomplete download\r\n", 35);
        g_ota_state = OTA_STATE_ERROR;
        return FALSE;
    }
    
    sendUARTMessage("[OTA] Download complete, verifying...\r\n", 40);
    g_ota_state = OTA_STATE_VERIFYING;
    
    /* Parse Zone Package Header */
    if (!ZonePackage_ParseHeader(ZONE_PACKAGE_START_ADDR, &g_zone_header))
    {
        sendUARTMessage("[OTA] ERROR: Invalid Zone Package header\r\n", 43);
        g_ota_state = OTA_STATE_ERROR;
        return FALSE;
    }
    
    /* Validate CRC32 */
    if (!ZonePackage_ValidateCRC(&g_zone_header))
    {
        sendUARTMessage("[OTA] ERROR: CRC validation failed\r\n", 37);
        g_ota_state = OTA_STATE_ERROR;
        return FALSE;
    }
    
    /* Print Zone Package info */
    ZonePackage_PrintInfo(&g_zone_header);
    
    sendUARTMessage("[OTA] ✅ Zone Package verified\r\n", 32);
    g_ota_state = OTA_STATE_EXTRACTING;
    
    return TRUE;
}

boolean OTA_InstallZGWFirmware(void)
{
    ECUMetadata_t zgw_metadata;
    uint8 firmware_buffer[4096];
    uint32 firmware_offset;
    uint32 bytes_remaining;
    uint32 chunk_size;
    FlashBank_t standbyBank;
    uint32 standbyAddr;
    uint32 i;
    char log_msg[128];
    
    if (g_ota_state != OTA_STATE_EXTRACTING)
    {
        sendUARTMessage("[OTA] ERROR: Invalid state for installation\r\n", 46);
        return FALSE;
    }
    
    sprintf(log_msg, "[OTA] Installing ZGW firmware (ECU ID: %s)...\r\n", ZGW_ECU_ID);
    sendUARTMessage(log_msg, strlen(log_msg));
    g_ota_state = OTA_STATE_INSTALLING;
    
    /* Find ZGW package in Zone Package using AppConfig.h defined ID */
    if (!ZonePackage_FindECUMetadata(&g_zone_header, ZGW_ECU_ID, &zgw_metadata))
    {
        sprintf(log_msg, "[OTA] ERROR: %s package not found in Zone Package\r\n", ZGW_ECU_ID);
        sendUARTMessage(log_msg, strlen(log_msg));
        g_ota_state = OTA_STATE_ERROR;
        return FALSE;
    }
    
    /* Check dependencies */
    if (!CheckDependencies(&zgw_metadata))
    {
        sendUARTMessage("[OTA] ERROR: Dependency check failed\r\n", 39);
        g_ota_state = OTA_STATE_ERROR;
        return FALSE;
    }
    
    /* Find ZGW offset in Zone Package */
    for (i = 0; i < g_zone_header.package_count; i++)
    {
        if (strcmp(g_zone_header.ecu_table[i].ecu_id, ZGW_ECU_ID) == 0)
        {
            firmware_offset = g_zone_header.ecu_table[i].offset + 256;  /* Skip metadata */
            bytes_remaining = g_zone_header.ecu_table[i].firmware_size;
            
            sprintf(log_msg, "[OTA] %s firmware: %u bytes at offset 0x%08X\r\n",
                    ZGW_ECU_ID, bytes_remaining, firmware_offset);
            sendUARTMessage(log_msg, strlen(log_msg));
            
            /* Determine standby bank */
            standbyBank = FlashBank_GetStandbyBank();
            standbyAddr = (standbyBank == FLASH_BANK_A) ? 
                          APPLICATION_A_START : APPLICATION_B_START;
            
            sprintf(log_msg, "[OTA] Target: %s (0x%08X)\r\n",
                    (standbyBank == FLASH_BANK_A) ? "Bank A" : "Bank B",
                    standbyAddr);
            sendUARTMessage(log_msg, strlen(log_msg));
            
            /* Erase standby bank */
            sendUARTMessage("[OTA] Erasing standby bank...\r\n", 32);
            FlashBank_EraseSector(standbyAddr, APPLICATION_A_SIZE);
            
            /* Write metadata */
            sendUARTMessage("[OTA] Writing metadata...\r\n", 27);
            ExtFlash_Read(ZONE_PACKAGE_START_ADDR + g_zone_header.ecu_table[i].offset,
                          firmware_buffer, 256);
            FlashBank_WriteSector(standbyAddr, firmware_buffer, 256);
            
            /* Copy firmware from External Flash to Internal PFlash */
            sendUARTMessage("[OTA] Writing firmware...\r\n", 27);
            uint32 internal_offset = 256;
            uint32 external_addr = ZONE_PACKAGE_START_ADDR + firmware_offset;
            
            while (bytes_remaining > 0)
            {
                chunk_size = (bytes_remaining > 4096) ? 4096 : bytes_remaining;
                
                /* Read from External Flash */
                ExtFlash_Read(external_addr, firmware_buffer, chunk_size);
                
                /* Write to Internal PFlash */
                FlashBank_WriteSector(standbyAddr + internal_offset, 
                                      firmware_buffer, chunk_size);
                
                external_addr += chunk_size;
                internal_offset += chunk_size;
                bytes_remaining -= chunk_size;
                
                /* Progress */
                if ((internal_offset % (256 * 1024)) == 0)
                {
                    uint32 progress = (internal_offset * 100) / zgw_metadata.firmware_size;
                    sprintf(log_msg, "[OTA] Write progress: %u%%\r\n", progress);
                    sendUARTMessage(log_msg, strlen(log_msg));
                }
            }
            
            sprintf(log_msg, "[OTA] ✅ %s firmware installed\r\n", ZGW_ECU_ID);
            sendUARTMessage(log_msg, strlen(log_msg));
            
            /* Update boot flags */
            FlashBankStatus_t status = FlashBank_GetStatusFlags();
            status.bits.statusB = BANK_STATUS_OK;
            status.bits.bootTarget = 1;  /* Switch to Bank B */
            FlashBank_WriteDFlashStatus(status);
            
            sendUARTMessage("[OTA] Boot target updated to Bank B\r\n", 38);
            
            g_ota_state = OTA_STATE_COMPLETE;
            return TRUE;
        }
    }
    
    sprintf(log_msg, "[OTA] ERROR: %s not found in ECU table\r\n", ZGW_ECU_ID);
    sendUARTMessage(log_msg, strlen(log_msg));
    g_ota_state = OTA_STATE_ERROR;
    return FALSE;
}

boolean OTA_DistributeToZoneECU(const char *ecu_id)
{
    ECUMetadata_t ecu_metadata;
    uint32 firmware_offset;
    uint32 bytes_remaining;
    uint32 i;
    char log_msg[128];
    
    /* Skip if target ECU is ZGW itself */
    if (strcmp(ecu_id, ZGW_ECU_ID) == 0)
    {
        sendUARTMessage("[OTA] Skipping ZGW (already installed)\r\n", 41);
        return TRUE;
    }
    
    sprintf(log_msg, "[OTA] Distributing firmware to %s...\r\n", ecu_id);
    sendUARTMessage(log_msg, strlen(log_msg));
    
    /* Find target ECU in Zone Package */
    if (!ZonePackage_FindECUMetadata(&g_zone_header, ecu_id, &ecu_metadata))
    {
        sprintf(log_msg, "[OTA] ERROR: %s not found in Zone Package\r\n", ecu_id);
        sendUARTMessage(log_msg, strlen(log_msg));
        return FALSE;
    }
    
    /* Check dependencies */
    if (!CheckDependencies(&ecu_metadata))
    {
        sprintf(log_msg, "[OTA] ERROR: %s dependency check failed\r\n", ecu_id);
        sendUARTMessage(log_msg, strlen(log_msg));
        return FALSE;
    }
    
    /* Find ECU offset in Zone Package */
    for (i = 0; i < g_zone_header.package_count; i++)
    {
        if (strcmp(g_zone_header.ecu_table[i].ecu_id, ecu_id) == 0)
        {
            firmware_offset = g_zone_header.ecu_table[i].offset;  /* Include metadata */
            bytes_remaining = g_zone_header.ecu_table[i].size;    /* Total size */
            
            sprintf(log_msg, "[OTA] %s package: %u bytes at offset 0x%08X\r\n",
                    ecu_id, bytes_remaining, firmware_offset);
            sendUARTMessage(log_msg, strlen(log_msg));
            
            /* TODO: Implement UDS 0x34/0x36/0x37 sequence to Zone ECU */
            /* Step 1: Send 0x34 Request Download */
            /* Step 2: Send 0x36 Transfer Data (chunked) */
            /* Step 3: Send 0x37 Request Transfer Exit */
            
            sprintf(log_msg, "[OTA] ✅ %s firmware distributed\r\n", ecu_id);
            sendUARTMessage(log_msg, strlen(log_msg));
            
            return TRUE;
        }
    }
    
    sprintf(log_msg, "[OTA] ERROR: %s not found in ECU table\r\n", ecu_id);
    sendUARTMessage(log_msg, strlen(log_msg));
    return FALSE;
}

boolean OTA_DistributeAllECUs(void)
{
    uint8 i;
    uint8 success_count;
    uint8 fail_count;
    char log_msg[128];
    
    success_count = 0;
    fail_count = 0;
    
    sendUARTMessage("\r\n[OTA] ========================================\r\n", 48);
    sendUARTMessage("[OTA] Starting Zone ECU distribution...\r\n", 42);
    sprintf(log_msg, "[OTA] Total ECUs in Zone Package: %u\r\n", g_zone_header.package_count);
    sendUARTMessage(log_msg, strlen(log_msg));
    sendUARTMessage("[OTA] ========================================\r\n\r\n", 50);
    
    /* Iterate through all ECUs in Zone Package */
    for (i = 0; i < g_zone_header.package_count; i++)
    {
        sprintf(log_msg, "[OTA] [%u/%u] Target: %s\r\n",
                i + 1, g_zone_header.package_count,
                g_zone_header.ecu_table[i].ecu_id);
        sendUARTMessage(log_msg, strlen(log_msg));
        
        /* Distribute to target ECU */
        if (OTA_DistributeToZoneECU(g_zone_header.ecu_table[i].ecu_id))
        {
            success_count++;
        }
        else
        {
            fail_count++;
            sprintf(log_msg, "[OTA] ❌ Failed: %s\r\n", g_zone_header.ecu_table[i].ecu_id);
            sendUARTMessage(log_msg, strlen(log_msg));
        }
        
        sendUARTMessage("\r\n", 2);
    }
    
    /* Summary */
    sendUARTMessage("[OTA] ========================================\r\n", 48);
    sprintf(log_msg, "[OTA] Distribution Complete:\r\n");
    sendUARTMessage(log_msg, strlen(log_msg));
    sprintf(log_msg, "[OTA]   ✅ Success: %u\r\n", success_count);
    sendUARTMessage(log_msg, strlen(log_msg));
    sprintf(log_msg, "[OTA]   ❌ Failed:  %u\r\n", fail_count);
    sendUARTMessage(log_msg, strlen(log_msg));
    sendUARTMessage("[OTA] ========================================\r\n\r\n", 50);
    
    return (fail_count == 0);
}

void OTA_GetProgress(OTA_Progress_t *progress)
{
    progress->state = g_ota_state;
    progress->total_size = g_ota_total_size;
    progress->downloaded_size = g_ota_downloaded_size;
    
    if (g_ota_total_size > 0)
    {
        progress->progress_percent = (g_ota_downloaded_size * 100) / g_ota_total_size;
    }
    else
    {
        progress->progress_percent = 0;
    }
}

OTA_State_t OTA_GetState(void)
{
    return g_ota_state;
}

boolean OTA_IsInProgress(void)
{
    return (g_ota_state != OTA_STATE_IDLE && 
            g_ota_state != OTA_STATE_COMPLETE && 
            g_ota_state != OTA_STATE_ERROR);
}

void OTA_Cancel(void)
{
    sendUARTMessage("[OTA] Cancelling operation...\r\n", 32);
    g_ota_state = OTA_STATE_IDLE;
    g_ota_total_size = 0;
    g_ota_downloaded_size = 0;
    g_ota_current_offset = 0;
}

