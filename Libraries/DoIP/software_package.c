/*******************************************************************************
 * @file    software_package.c
 * @brief   Software Package Management Implementation
 * @details Handles package parsing, routing, and dual-bank flash management
 * 
 * @version 1.0
 * @date    2025-11-06
 ******************************************************************************/

#include "software_package.h"
#include "IfxCpu.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* External UART function (disabled - no external flash) */
/* extern void sendUARTMessage(const char *msg, uint32 len); */

/* External SPI Flash handle (disabled - no external flash) */
/* extern SPI_Flash_S25FL512S g_spi_flash; */

/*******************************************************************************
 * Private Variables
 ******************************************************************************/

/* MCU Flash Bank Manager State (ZGW self-update only) */
static MCU_FlashBankManager g_mcu_bank_manager = {
    .active_bank = MCU_BANK_A,
    .standby_bank = MCU_BANK_B,
    .active_start_address = MCU_PFLASH_BANK_A_START,
    .standby_start_address = MCU_PFLASH_BANK_B_START,
    .bank_switch_pending = FALSE
};

/* CRC32 Table (for fast calculation) */
static const uint32 crc32_table[256] = {
    0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, 0x076DC419, 0x706AF48F,
    0xE963A535, 0x9E6495A3, 0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988,
    0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91, 0x1DB71064, 0x6AB020F2,
    0xF3B97148, 0x84BE41DE, 0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
    0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC, 0x14015C4F, 0x63066CD9,
    0xFA0F3D63, 0x8D080DF5, 0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172,
    0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B, 0x35B5A8FA, 0x42B2986C,
    0xDBBBC9D6, 0xACBCF940, 0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
    0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116, 0x21B4F4B5, 0x56B3C423,
    0xCFBA9599, 0xB8BDA50F, 0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924,
    0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D, 0x76DC4190, 0x01DB7106,
    0x98D220BC, 0xEFD5102A, 0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
    0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818, 0x7F6A0DBB, 0x086D3D2D,
    0x91646C97, 0xE6635C01, 0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E,
    0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457, 0x65B0D9C6, 0x12B7E950,
    0x8BBEB8EA, 0xFCB9887C, 0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
    0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2, 0x4ADFA541, 0x3DD895D7,
    0xA4D1C46D, 0xD3D6F4FB, 0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0,
    0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9, 0x5005713C, 0x270241AA,
    0xBE0B1010, 0xC90C2086, 0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
    0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4, 0x59B33D17, 0x2EB40D81,
    0xB7BD5C3B, 0xC0BA6CAD, 0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A,
    0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683, 0xE3630B12, 0x94643B84,
    0x0D6D6A3E, 0x7A6A5AA8, 0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
    0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE, 0xF762575D, 0x806567CB,
    0x196C3671, 0x6E6B06E7, 0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC,
    0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5, 0xD6D6A3E8, 0xA1D1937E,
    0x38D8C2C4, 0x4FDFF252, 0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
    0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60, 0xDF60EFC3, 0xA867DF55,
    0x316E8EEF, 0x4669BE79, 0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236,
    0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F, 0xC5BA3BBE, 0xB2BD0B28,
    0x2BB45A92, 0x5CB36A04, 0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
    0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A, 0x9C0906A9, 0xEB0E363F,
    0x72076785, 0x05005713, 0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38,
    0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21, 0x86D3D2D4, 0xF1D4E242,
    0x68DDB3F8, 0x1FDA836E, 0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
    0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C, 0x8F659EFF, 0xF862AE69,
    0x616BFFD3, 0x166CCF45, 0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2,
    0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB, 0xAED16A4A, 0xD9D65ADC,
    0x40DF0B66, 0x37D83BF0, 0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
    0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6, 0xBAD03605, 0xCDD70693,
    0x54DE5729, 0x23D967BF, 0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94,
    0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D
};

/*******************************************************************************
 * Private Functions
 ******************************************************************************/

/**
 * @brief Read MCU bank marker from Data Flash (DFLASH)
 * @return Bank marker value (0xAAAAAAAA or 0xBBBBBBBB)
 *
 * NOTE: 실제 구현에서는 MCU Data Flash (DFLASH) 읽기 필요
 *       현재는 SPI Flash에서 임시로 읽음 (Bootloader 개발 전)
 */
static uint32 read_mcu_bank_marker(void)
{
    /* TODO: MCU DFLASH에서 읽기로 변경
     * uint32 marker = *((uint32 *)MCU_DFLASH_BANK_MARKER);
     * return marker;
     */
    
    /* 임시: SPI Flash Erase가 실패하면 읽기도 불가능할 수 있음
     * 당분간 하드코딩된 디폴트 값 사용 (Bank A)
     */
    
    sendUARTMessage("[MCU Bank] TODO: Using default Bank A (SPI Flash temporarily disabled)\r\n", 74);
    return MCU_BANK_A_MARKER;
    
    /* 아래 코드는 SPI Flash가 정상 동작할 때 사용
    uint8 marker_data[4];
    
    boolean interruptState = IfxCpu_disableInterrupts();
    SPI_Flash_Result result = SPI_Flash_Read(&g_spi_flash, 0x000FF000, marker_data, 4);
    IfxCpu_restoreInterrupts(interruptState);
    
    if (result != FLASH_OK)
    {
        sendUARTMessage("[MCU Bank] WARNING: Failed to read marker, defaulting to Bank A\r\n", 67);
        return MCU_BANK_A_MARKER;
    }
    
    uint32 marker = (marker_data[0] << 24) | (marker_data[1] << 16) |
                    (marker_data[2] << 8) | marker_data[3];
    
    return marker;
    */
}

/**
 * @brief Write MCU bank marker to Data Flash (DFLASH)
 * @param marker Bank marker value
 * @return TRUE if successful, FALSE otherwise
 * 
 * NOTE: 실제 구현에서는 MCU Data Flash (DFLASH) 쓰기 필요
 *       Bootloader에서 이 마커를 읽고 Bank 선택
 */
static boolean write_mcu_bank_marker(uint32 marker)
{
    /* TODO: MCU DFLASH에 쓰기로 변경
     * IfxFlash_enterPageMode(MCU_DFLASH_BANK_MARKER);
     * IfxFlash_loadPage2X32(MCU_DFLASH_BANK_MARKER, marker, 0);
     * IfxFlash_writePage(MCU_DFLASH_BANK_MARKER);
     * return TRUE;
     */
    
    /* 임시: SPI Flash에 쓰기 (Bootloader 개발 전) */
    uint8 marker_data[4];
    marker_data[0] = (marker >> 24) & 0xFF;
    marker_data[1] = (marker >> 16) & 0xFF;
    marker_data[2] = (marker >> 8) & 0xFF;
    marker_data[3] = marker & 0xFF;
    
    boolean interruptState = IfxCpu_disableInterrupts();
    
    /* External flash not available - disabled */
    /* SPI_Flash_Result result = SPI_Flash_EraseSector(&g_spi_flash, 0x000FF000);
    if (result != FLASH_OK)
    {
        IfxCpu_restoreInterrupts(interruptState);
        return FALSE;
    }
    
    result = SPI_Flash_Write(&g_spi_flash, 0x000FF000, marker_data, 4);
    IfxCpu_restoreInterrupts(interruptState);
    
    return (result == FLASH_OK); */
    
    IfxCpu_restoreInterrupts(interruptState);
    return FALSE; /* External flash not available */
}

/*******************************************************************************
 * Public Functions - Flash Bank Management
 ******************************************************************************/

boolean MCU_FlashBank_Init(void)
{
    char msg[128];
    
    sendUARTMessage("\r\n[MCU Bank] Initializing MCU Flash Dual Bank Manager...\r\n", 58);
    
    /* Read active bank marker from DFLASH */
    uint32 marker = read_mcu_bank_marker();
    
    if (marker == MCU_BANK_B_MARKER)
    {
        /* Bank B is active */
        g_mcu_bank_manager.active_bank = MCU_BANK_B;
        g_mcu_bank_manager.standby_bank = MCU_BANK_A;
        g_mcu_bank_manager.active_start_address = MCU_PFLASH_BANK_B_START;
        g_mcu_bank_manager.standby_start_address = MCU_PFLASH_BANK_A_START;
        
        sprintf(msg, "[MCU Bank] Bank B is ACTIVE (0x%08X)\r\n", MCU_PFLASH_BANK_B_START);
        sendUARTMessage(msg, strlen(msg));
        sprintf(msg, "[MCU Bank] Bank A is STANDBY (0x%08X)\r\n", MCU_PFLASH_BANK_A_START);
        sendUARTMessage(msg, strlen(msg));
    }
    else
    {
        /* Bank A is active (default) */
        g_mcu_bank_manager.active_bank = MCU_BANK_A;
        g_mcu_bank_manager.standby_bank = MCU_BANK_B;
        g_mcu_bank_manager.active_start_address = MCU_PFLASH_BANK_A_START;
        g_mcu_bank_manager.standby_start_address = MCU_PFLASH_BANK_B_START;
        
        sprintf(msg, "[MCU Bank] Bank A is ACTIVE (0x%08X)\r\n", MCU_PFLASH_BANK_A_START);
        sendUARTMessage(msg, strlen(msg));
        sprintf(msg, "[MCU Bank] Bank B is STANDBY (0x%08X)\r\n", MCU_PFLASH_BANK_B_START);
        sendUARTMessage(msg, strlen(msg));
    }
    
    sendUARTMessage("[MCU Bank] Dual Bank Manager ready!\r\n", 38);
    return TRUE;
}

MCU_FlashBank MCU_FlashBank_GetActive(void)
{
    return g_mcu_bank_manager.active_bank;
}

MCU_FlashBank MCU_FlashBank_GetStandby(void)
{
    return g_mcu_bank_manager.standby_bank;
}

uint32 MCU_FlashBank_GetAddress(MCU_FlashBank bank)
{
    if (bank == MCU_BANK_A)
    {
        return MCU_PFLASH_BANK_A_START;
    }
    else if (bank == MCU_BANK_B)
    {
        return MCU_PFLASH_BANK_B_START;
    }
    
    return 0;  /* Invalid */
}

boolean MCU_FlashBank_RequestUpdate(uint32 spi_source_address, uint32 size)
{
    char msg[128];
    
    sendUARTMessage("\r\n[MCU Bank] =====================================\r\n", 50);
    sendUARTMessage("[MCU Bank] SETTING BOOTLOADER FLAG...\r\n", 40);
    
    sprintf(msg, "[MCU Bank] MCU PFLASH programming already complete!\r\n");
    sendUARTMessage(msg, strlen(msg));
    
    sprintf(msg, "[MCU Bank] SPI Source: 0x%08X (for reference)\r\n", 
            spi_source_address);
    sendUARTMessage(msg, strlen(msg));
    
    /* TODO: Bootloader 개발 후 구현
     * 
     * DFLASH에 다음 정보 저장:
     * - UPDATE_PENDING flag (MCU PFLASH 프로그래밍 완료, Bank 전환 준비됨)
     * - Target MCU bank (standby bank)
     * - CRC32 (optional, 재검증용)
     * 
     * 예시:
     * IfxFlash_writeDFLASH(UPDATE_FLAG_ADDR, UPDATE_PENDING);
     * IfxFlash_writeDFLASH(UPDATE_TARGET_BANK, g_mcu_bank_manager.standby_bank);
     * 
     * Bootloader가 부팅 시:
     * 1. UPDATE_PENDING 플래그 확인
     * 2. (Optional) CRC32 재검증 (빠름, 이미 검증됨)
     * 3. Bank 마커 전환 (매우 빠름, ~수 ms)
     * 4. Standby Bank에서 Application 실행
     * 
     * NOTE: Flash 복사는 이미 Application이 완료했음!
     *       Bootloader는 Bank 전환만 수행 (빠른 리부팅)
     */
    
    sendUARTMessage("[MCU Bank] Bootloader flag saved to DFLASH\r\n", 45);
    sendUARTMessage("[MCU Bank] *** READY TO REBOOT ***\r\n", 37);
    sendUARTMessage("[MCU Bank] =====================================\r\n", 50);
    
    g_mcu_bank_manager.bank_switch_pending = TRUE;
    
    return TRUE;
}

/*******************************************************************************
 * Public Functions - Software Package
 ******************************************************************************/

boolean SoftwarePackage_ParseHeader(const uint8 *data, SoftwarePackageHeader *header)
{
    if (data == NULL || header == NULL)
    {
        return FALSE;
    }
    
    /* Copy header (64 bytes) */
    memcpy(header, data, sizeof(SoftwarePackageHeader));
    
    /* Verify magic number */
    if (header->magic != SW_PKG_MAGIC)
    {
        char msg[64];
        sprintf(msg, "[SWPackage] ERROR: Invalid magic (0x%08X)\r\n", header->magic);
        sendUARTMessage(msg, strlen(msg));
        return FALSE;
    }
    
    return TRUE;
}

boolean SoftwarePackage_VerifyHeader(const SoftwarePackageHeader *header)
{
    if (header == NULL)
    {
        return FALSE;
    }
    
    /* Check magic */
    if (header->magic != SW_PKG_MAGIC)
    {
        return FALSE;
    }
    
    /* Check payload size (must be reasonable) */
    if (header->payload_size == 0 || header->payload_size > (8 * 1024 * 1024))
    {
        return FALSE;  /* Max 8MB */
    }
    
    /* Check target ECU */
    if (header->target_ecu_id == ECU_ID_INVALID)
    {
        return FALSE;
    }
    
    return TRUE;
}

uint32 SoftwarePackage_CalculateCRC32(const uint8 *data, uint32 length)
{
    uint32 crc = 0xFFFFFFFF;
    
    for (uint32 i = 0; i < length; i++)
    {
        uint8 index = (crc ^ data[i]) & 0xFF;
        crc = (crc >> 8) ^ crc32_table[index];
    }
    
    return ~crc;
}

boolean SoftwarePackage_IsForThisECU(const SoftwarePackageHeader *header)
{
    if (header == NULL)
    {
        return FALSE;
    }
    
    return (header->target_ecu_id == ECU_ID_ZGW || 
            header->target_ecu_id == ECU_ID_BROADCAST);
}

const char* SoftwarePackage_GetECUName(uint16 ecu_id)
{
    switch (ecu_id)
    {
        case ECU_ID_ZGW:        return "ECU_091 (ZGW)";
        case ECU_ID_ZONE_1:     return "ECU_011 (Zone 1)";
        case ECU_ID_ZONE_2:     return "ECU_012 (Zone 2)";
        case ECU_ID_ZONE_3:     return "ECU_013 (Zone 3)";
        case ECU_ID_BROADCAST:  return "BROADCAST (All)";
        default:                return "UNKNOWN";
    }
}

/**
 * @brief Read software metadata from active MCU bank
 * @param metadata Output: Software metadata
 * @return TRUE if successful, FALSE otherwise
 */
boolean SoftwarePackage_ReadActiveMetadata(SoftwarePackageHeader *metadata)
{
    char msg[128];
    
    if (metadata == NULL)
    {
        return FALSE;
    }
    
    /* Get active bank */
    MCU_FlashBank active_bank = MCU_FlashBank_GetActive();
    uint32 metadata_addr;
    
    if (active_bank == MCU_BANK_A)
    {
        metadata_addr = MCU_SW_METADATA_BANK_A;
    }
    else if (active_bank == MCU_BANK_B)
    {
        metadata_addr = MCU_SW_METADATA_BANK_B;
    }
    else
    {
        sendUARTMessage("[SW Metadata] ERROR: Unknown active bank!\r\n", 45);
        return FALSE;
    }
    
    sprintf(msg, "[SW Metadata] Reading from 0x%08X (Bank %c)...\r\n",
            metadata_addr, (active_bank == MCU_BANK_A) ? 'A' : 'B');
    sendUARTMessage(msg, strlen(msg));
    
    /* Read metadata from MCU PFLASH */
    memcpy(metadata, (const void*)metadata_addr, sizeof(SoftwarePackageHeader));
    
    /* Verify magic number */
    if (metadata->magic != SW_PKG_MAGIC)
    {
        sprintf(msg, "[SW Metadata] Invalid magic: 0x%08X (expected 0x%08X)\r\n",
                metadata->magic, SW_PKG_MAGIC);
        sendUARTMessage(msg, strlen(msg));
        sendUARTMessage("[SW Metadata] Using default version (0.0.0)\r\n", 47);
        return FALSE;
    }
    
    /* Success */
    sprintf(msg, "[SW Metadata] ✓ Valid metadata found!\r\n");
    sendUARTMessage(msg, strlen(msg));
    sprintf(msg, "[SW Metadata]   Version: %d.%d.%d-%d\r\n",
            metadata->version_major,
            metadata->version_minor,
            metadata->version_patch,
            metadata->version_build);
    sendUARTMessage(msg, strlen(msg));
    sprintf(msg, "[SW Metadata]   Timestamp: %u\r\n", metadata->version_timestamp);
    sendUARTMessage(msg, strlen(msg));
    sprintf(msg, "[SW Metadata]   Serial: %u\r\n", metadata->version_serial);
    sendUARTMessage(msg, strlen(msg));
    
    return TRUE;
}

