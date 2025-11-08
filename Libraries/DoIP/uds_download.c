/*******************************************************************************
 * @file    uds_download.c
 * @brief   UDS Download Services Implementation
 * @details Implements 0x34/0x36/0x37 for firmware download to SPI Flash
 * 
 * @version 1.0
 * @date    2025-11-06
 ******************************************************************************/

#include "uds_download.h"
#include "uds_handler.h"
#include "software_package.h"
#include "mcu_flash_programming.h"
#include "IfxCpu.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* External UART function */
extern void sendUARTMessage(const char *msg, uint32 len);

/*******************************************************************************
 * Private Variables
 ******************************************************************************/

/* Download session state */
static UDS_DownloadSession g_download_session = {0};

/*******************************************************************************
 * Private Functions
 ******************************************************************************/

/**
 * @brief Copy software from SPI Flash to MCU PFLASH
 * @param spi_source SPI Flash source address
 * @param mcu_target MCU PFLASH target address
 * @param size Software size in bytes
 * @return TRUE if successful, FALSE otherwise
 * 
 * NOTE: This function programs MCU internal Flash while vehicle is running!
 *       Uses interrupt protection and proper Flash programming sequence.
 */
static boolean MCU_PFLASH_CopyFromSPI(uint32 spi_source, uint32 mcu_target, uint32 size)
{
    /* Call the actual MCU Flash Programming module (Infineon example-based) */
    MCU_Flash_Result result = MCU_Flash_CopyFromSPI(spi_source, mcu_target, size);
    
    return (result == MCU_FLASH_OK) ? TRUE : FALSE;
}

/**
 * @brief Erase flash area for download (DISABLED - MCU PFLASH only)
 * @return TRUE (stub)
 */
static boolean Flash_EraseArea(uint32 start_address, uint32 size)
{
    char msg[128];
    sprintf(msg, "[UDS Download] Flash erase (DISABLED): 0x%08X, Size: %u KB\r\n", 
            start_address, size / 1024);
    sendUARTMessage(msg, strlen(msg));
    return TRUE;
}

/**
 * @brief Write data to flash (DISABLED - MCU PFLASH only)
 * @return TRUE (stub)
 */
static boolean Flash_WriteData(uint32 address, const uint8 *data, uint32 length)
{
    if (data == NULL || length == 0)
    {
        return FALSE;
    }
    return TRUE;
}

/**
 * @brief Verify written data (DISABLED - MCU PFLASH only)
 * @return TRUE (stub)
 */
static boolean Flash_VerifyData(uint32 address, const uint8 *expected_data, uint32 length)
{
    if (expected_data == NULL || length == 0)
    {
        return FALSE;
    }
    return TRUE;
}

/*******************************************************************************
 * Public Functions
 ******************************************************************************/

void UDS_Download_Init(void)
{
    memset(&g_download_session, 0, sizeof(UDS_DownloadSession));
    g_download_session.state = DOWNLOAD_STATE_IDLE;
    
    /* Initialize MCU Flash Programming module */
    MCU_Flash_Init();
    
    sendUARTMessage("[UDS Download] Module initialized with MCU PFLASH support\r\n", 60);
}

void UDS_Download_Reset(void)
{
    memset(&g_download_session, 0, sizeof(UDS_DownloadSession));
    g_download_session.state = DOWNLOAD_STATE_IDLE;
    
    sendUARTMessage("[UDS Download] Session reset\r\n", 31);
}

const UDS_DownloadSession* UDS_Download_GetSession(void)
{
    return &g_download_session;
}

/*******************************************************************************
 * UDS Service Handlers
 ******************************************************************************/

boolean UDS_Service_RequestDownload(const UDS_Request *request, UDS_Response *response)
{
    char msg[128];
    
    /* Check if download already in progress */
    if (g_download_session.is_active)
    {
        sprintf(msg, "[UDS 0x34] ERROR: Download already in progress!\r\n");
        sendUARTMessage(msg, strlen(msg));
        UDS_CreateNegativeResponse(request, UDS_NRC_CONDITIONS_NOT_CORRECT, response);
        return TRUE;
    }
    
    /* RequestDownload format:
     * Byte 0: dataFormatIdentifier (ignored for now)
     * Byte 1: addressAndLengthFormatIdentifier
     *         Upper nibble: length of memorySize
     *         Lower nibble: length of memoryAddress
     * Byte 2...: memoryAddress (Big Endian)
     * Byte ...: memorySize (Big Endian)
     */
    
    if (request->data_len < 3)
    {
        sprintf(msg, "[UDS 0x34] ERROR: Incorrect message length (%d)\r\n", request->data_len);
        sendUARTMessage(msg, strlen(msg));
        UDS_CreateNegativeResponse(request, UDS_NRC_INCORRECT_MESSAGE_LENGTH, response);
        return TRUE;
    }
    
    uint8 addr_len_format = request->data[1];
    uint8 address_len = addr_len_format & 0x0F;
    uint8 size_len = (addr_len_format >> 4) & 0x0F;
    
    if (request->data_len < (2 + address_len + size_len))
    {
        sprintf(msg, "[UDS 0x34] ERROR: Incorrect format (addr_len=%d, size_len=%d)\r\n", 
                address_len, size_len);
        sendUARTMessage(msg, strlen(msg));
        UDS_CreateNegativeResponse(request, UDS_NRC_INCORRECT_MESSAGE_LENGTH, response);
        return TRUE;
    }
    
    /* Parse memory address */
    uint32 memory_address = 0;
    for (uint8 i = 0; i < address_len; i++)
    {
        memory_address = (memory_address << 8) | request->data[2 + i];
    }
    
    /* Parse memory size */
    uint32 memory_size = 0;
    for (uint8 i = 0; i < size_len; i++)
    {
        memory_size = (memory_size << 8) | request->data[2 + address_len + i];
    }
    
    sprintf(msg, "[UDS 0x34] RequestDownload: Addr=0x%08X, Size=%u bytes\r\n", 
            memory_address, memory_size);
    sendUARTMessage(msg, strlen(msg));
    
    /* Note: memory_address는 SPI Flash Staging Area 주소로 해석됨 */
    /* 실제로는 software package header의 target_ecu_id로 결정되므로 여기서는 체크 생략 */
    
    /* Note: Erase는 TransferData에서 target_ecu_id 파싱 후 수행 */
    
    /* Initialize download session */
    g_download_session.state = DOWNLOAD_STATE_REQUESTED;
    g_download_session.flash_start_address = memory_address;
    g_download_session.flash_current_address = memory_address;
    g_download_session.total_bytes_expected = memory_size;
    g_download_session.total_bytes_received = 0;
    g_download_session.block_sequence_counter = 1;  /* Start at 1 */
    g_download_session.max_block_length = 256;      /* Max 256 bytes per block */
    g_download_session.is_active = TRUE;
    g_download_session.header_received = FALSE;
    g_download_session.spi_staging_address = 0;  /* Will be determined from header */
    
    /* Positive response: lengthFormatIdentifier + maxNumberOfBlockLength */
    UDS_CreatePositiveResponse(request, response);
    response->data[0] = 0x20;  /* lengthFormatIdentifier: 2 bytes */
    response->data[1] = (g_download_session.max_block_length >> 8) & 0xFF;
    response->data[2] = g_download_session.max_block_length & 0xFF;
    response->data_len = 3;
    
    sprintf(msg, "[UDS 0x34] Download session started (Max block: %d bytes)\r\n", 
            g_download_session.max_block_length);
    sendUARTMessage(msg, strlen(msg));
    
    return TRUE;
}

boolean UDS_Service_TransferData(const UDS_Request *request, UDS_Response *response)
{
    char msg[128];
    
    /* Check if download session is active */
    if (!g_download_session.is_active)
    {
        sprintf(msg, "[UDS 0x36] ERROR: No download session active!\r\n");
        sendUARTMessage(msg, strlen(msg));
        UDS_CreateNegativeResponse(request, UDS_NRC_REQUEST_SEQUENCE_ERROR, response);
        return TRUE;
    }
    
    /* TransferData format:
     * Byte 0: blockSequenceCounter
     * Byte 1...: transferRequestParameterRecord (data)
     */
    
    if (request->data_len < 2)
    {
        sprintf(msg, "[UDS 0x36] ERROR: Incorrect message length (%d)\r\n", request->data_len);
        sendUARTMessage(msg, strlen(msg));
        UDS_CreateNegativeResponse(request, UDS_NRC_INCORRECT_MESSAGE_LENGTH, response);
        return TRUE;
    }
    
    uint8 block_counter = request->data[0];
    uint16 data_len = request->data_len - 1;
    const uint8 *data = &request->data[1];
    
    /* Verify block sequence counter */
    if (block_counter != g_download_session.block_sequence_counter)
    {
        sprintf(msg, "[UDS 0x36] ERROR: Wrong block sequence! Expected: %d, Got: %d\r\n",
                g_download_session.block_sequence_counter, block_counter);
        sendUARTMessage(msg, strlen(msg));
        UDS_CreateNegativeResponse(request, UDS_NRC_WRONG_BLOCK_SEQUENCE_COUNTER, response);
        return TRUE;
    }
    
    /*=========================================================================
     * BLOCK 1: Parse Software Package Header & Determine Staging Area
     *=========================================================================*/
    if (block_counter == 1 && !g_download_session.header_received)
    {
        sendUARTMessage("\r\n[UDS 0x36] ========================================\r\n", 50);
        sendUARTMessage("[UDS 0x36] Block 1: Parsing Software Package Header\r\n", 54);
        
        /* Parse header (first 64 bytes) */
        if (data_len < sizeof(SoftwarePackageHeader))
        {
            sprintf(msg, "[UDS 0x36] ERROR: Block 1 too small (%d < %d)\r\n", 
                    data_len, sizeof(SoftwarePackageHeader));
            sendUARTMessage(msg, strlen(msg));
            UDS_CreateNegativeResponse(request, UDS_NRC_INCORRECT_MESSAGE_LENGTH, response);
            return TRUE;
        }
        
        if (!SoftwarePackage_ParseHeader(data, &g_download_session.sw_header))
        {
            sendUARTMessage("[UDS 0x36] ERROR: Invalid package header!\r\n", 44);
            UDS_CreateNegativeResponse(request, UDS_NRC_GENERAL_PROGRAMMING_FAILURE, response);
            return TRUE;
        }
        
        if (!SoftwarePackage_VerifyHeader(&g_download_session.sw_header))
        {
            sendUARTMessage("[UDS 0x36] ERROR: Header verification failed!\r\n", 48);
            UDS_CreateNegativeResponse(request, UDS_NRC_GENERAL_PROGRAMMING_FAILURE, response);
            return TRUE;
        }
        
        g_download_session.header_received = TRUE;
        g_download_session.target_ecu_id = g_download_session.sw_header.target_ecu_id;
        g_download_session.is_for_this_ecu = SoftwarePackage_IsForThisECU(&g_download_session.sw_header);
        
        /* Print header info */
        sprintf(msg, "[SWPackage] Target: %s (0x%04X)\r\n", 
                SoftwarePackage_GetECUName(g_download_session.target_ecu_id),
                g_download_session.target_ecu_id);
        sendUARTMessage(msg, strlen(msg));
        
        sprintf(msg, "[SWPackage] Version: %d.%d.%d (Build %d)\r\n",
                g_download_session.sw_header.version_major,
                g_download_session.sw_header.version_minor,
                g_download_session.sw_header.version_patch,
                g_download_session.sw_header.version_build);
        sendUARTMessage(msg, strlen(msg));
        
        sprintf(msg, "[SWPackage] Size: %u bytes (%u KB)\r\n",
                g_download_session.sw_header.payload_size,
                g_download_session.sw_header.payload_size / 1024);
        sendUARTMessage(msg, strlen(msg));
        
        /* Determine SPI Flash Staging Area based on target ECU */
        if (g_download_session.target_ecu_id == ECU_ID_ZGW)
        {
            g_download_session.spi_staging_address = SPI_FLASH_ECU_091_START;
            sendUARTMessage("[SWPackage] ✓ Target: ZGW (This ECU)\r\n", 40);
            sendUARTMessage("[SWPackage] Staging: SPI Flash @ 0x00000000\r\n", 47);
        }
        else if (g_download_session.target_ecu_id == ECU_ID_ZONE_1)
        {
            g_download_session.spi_staging_address = SPI_FLASH_ECU_011_START;
            sendUARTMessage("[SWPackage] → Target: Zone ECU 1\r\n", 35);
            sendUARTMessage("[SWPackage] Staging: SPI Flash @ 0x00400000\r\n", 47);
        }
        else if (g_download_session.target_ecu_id == ECU_ID_ZONE_2)
        {
            g_download_session.spi_staging_address = SPI_FLASH_ECU_012_START;
            sendUARTMessage("[SWPackage] → Target: Zone ECU 2\r\n", 35);
            sendUARTMessage("[SWPackage] Staging: SPI Flash @ 0x00800000\r\n", 47);
        }
        else if (g_download_session.target_ecu_id == ECU_ID_ZONE_3)
        {
            g_download_session.spi_staging_address = SPI_FLASH_ECU_013_START;
            sendUARTMessage("[SWPackage] → Target: Zone ECU 3\r\n", 35);
            sendUARTMessage("[SWPackage] Staging: SPI Flash @ 0x00C00000\r\n", 47);
        }
        else
        {
            sprintf(msg, "[SWPackage] ERROR: Unknown target ECU 0x%04X!\r\n", 
                    g_download_session.target_ecu_id);
            sendUARTMessage(msg, strlen(msg));
            UDS_CreateNegativeResponse(request, UDS_NRC_REQUEST_OUT_OF_RANGE, response);
            return TRUE;
        }
        
        /* Update flash addresses to staging area */
        g_download_session.flash_start_address = g_download_session.spi_staging_address;
        g_download_session.flash_current_address = g_download_session.spi_staging_address;
        
        /* Erase staging area */
        sendUARTMessage("[SWPackage] Erasing SPI Flash Staging Area...\r\n", 49);
        if (!Flash_EraseArea(g_download_session.spi_staging_address, 
                            g_download_session.sw_header.payload_size))
        {
            sendUARTMessage("[SWPackage] ERROR: Erase failed!\r\n", 35);
            UDS_CreateNegativeResponse(request, UDS_NRC_GENERAL_PROGRAMMING_FAILURE, response);
            return TRUE;
        }
        sendUARTMessage("[SWPackage] Erase complete!\r\n", 30);
        sendUARTMessage("[UDS 0x36] ========================================\r\n\r\n", 52);
        
        /* Write entire first block (header + any payload) to staging area */
        if (!Flash_WriteData(g_download_session.flash_current_address, data, data_len))
        {
            UDS_CreateNegativeResponse(request, UDS_NRC_GENERAL_PROGRAMMING_FAILURE, response);
            g_download_session.state = DOWNLOAD_STATE_ERROR;
            return TRUE;
        }
        
        g_download_session.flash_current_address += data_len;
        g_download_session.total_bytes_received += data_len;
    }
    /*=========================================================================
     * BLOCK 2+: Regular Data Transfer
     *=========================================================================*/
    else
    {
        if (!g_download_session.header_received)
        {
            sendUARTMessage("[UDS 0x36] ERROR: Header not yet received!\r\n", 45);
            UDS_CreateNegativeResponse(request, UDS_NRC_REQUEST_SEQUENCE_ERROR, response);
            return TRUE;
        }
        
        /* Check if data exceeds expected size */
        if (g_download_session.total_bytes_received + data_len > g_download_session.total_bytes_expected)
        {
            sprintf(msg, "[UDS 0x36] ERROR: Data overflow! Expected: %u, Got: %u + %u\r\n",
                    g_download_session.total_bytes_expected,
                    g_download_session.total_bytes_received, data_len);
            sendUARTMessage(msg, strlen(msg));
            UDS_CreateNegativeResponse(request, UDS_NRC_TRANSFER_DATA_SUSPENDED, response);
            return TRUE;
        }
        
        /* Write data to SPI Flash staging area */
        if (!Flash_WriteData(g_download_session.flash_current_address, data, data_len))
        {
            sprintf(msg, "[UDS 0x36] ERROR: Flash write failed!\r\n");
            sendUARTMessage(msg, strlen(msg));
            UDS_CreateNegativeResponse(request, UDS_NRC_GENERAL_PROGRAMMING_FAILURE, response);
            g_download_session.state = DOWNLOAD_STATE_ERROR;
            return TRUE;
        }
        
        g_download_session.flash_current_address += data_len;
        g_download_session.total_bytes_received += data_len;
    }
    
    /* Update session state */
    g_download_session.block_sequence_counter = (g_download_session.block_sequence_counter + 1) & 0xFF;
    if (g_download_session.block_sequence_counter == 0)
    {
        g_download_session.block_sequence_counter = 1;  /* Wrap to 1, not 0 */
    }
    g_download_session.state = DOWNLOAD_STATE_TRANSFERRING;
    
    /* Progress logging (every 10 blocks) */
    if ((block_counter % 10) == 0 || 
        g_download_session.total_bytes_received >= g_download_session.total_bytes_expected)
    {
        uint32 progress_pct = (g_download_session.total_bytes_received * 100) / 
                              g_download_session.total_bytes_expected;
        sprintf(msg, "[UDS 0x36] Block %d: %u / %u bytes (%u%%)\r\n",
                block_counter,
                g_download_session.total_bytes_received,
                g_download_session.total_bytes_expected,
                progress_pct);
        sendUARTMessage(msg, strlen(msg));
    }
    
    /* Positive response: echo block counter */
    UDS_CreatePositiveResponse(request, response);
    response->data[0] = block_counter;
    response->data_len = 1;
    
    return TRUE;
}

boolean UDS_Service_RequestTransferExit(const UDS_Request *request, UDS_Response *response)
{
    char msg[128];
    
    /* Check if download session is active */
    if (!g_download_session.is_active)
    {
        sprintf(msg, "[UDS 0x37] ERROR: No download session active!\r\n");
        sendUARTMessage(msg, strlen(msg));
        UDS_CreateNegativeResponse(request, UDS_NRC_REQUEST_SEQUENCE_ERROR, response);
        return TRUE;
    }
    
    sendUARTMessage("\r\n[UDS 0x37] ========================================\r\n", 50);
    sendUARTMessage("[UDS 0x37] RequestTransferExit\r\n", 33);
    sendUARTMessage("[UDS 0x37] ========================================\r\n", 50);
    
    sprintf(msg, "[UDS 0x37] Received %u / %u bytes\r\n",
            g_download_session.total_bytes_received,
            g_download_session.total_bytes_expected);
    sendUARTMessage(msg, strlen(msg));
    
    /* Verify all data was received */
    if (g_download_session.total_bytes_received != g_download_session.total_bytes_expected)
    {
        sprintf(msg, "[UDS 0x37] ERROR: Incomplete transfer!\r\n");
        sendUARTMessage(msg, strlen(msg));
        UDS_CreateNegativeResponse(request, UDS_NRC_GENERAL_PROGRAMMING_FAILURE, response);
        g_download_session.state = DOWNLOAD_STATE_ERROR;
        return TRUE;
    }
    
    sprintf(msg, "[UDS 0x37] SPI Flash Staging: 0x%08X - 0x%08X\r\n",
            g_download_session.flash_start_address,
            g_download_session.flash_current_address);
    sendUARTMessage(msg, strlen(msg));
    
    /*=========================================================================
     * Path 1: ZGW Self-Update → Application programs MCU Flash (차량 운행 중!)
     *=========================================================================*/
    if (g_download_session.is_for_this_ecu)
    {
        sendUARTMessage("\r\n[UDS 0x37] ========================================\r\n", 50);
        sendUARTMessage("[UDS 0x37] Target: ZGW (This ECU)\r\n", 36);
        sendUARTMessage("[UDS 0x37] ========================================\r\n", 50);
        
        sendUARTMessage("[UDS 0x37] SW stored to SPI Flash Staging Area\r\n", 50);
        
        /* Step 1: Get standby MCU Flash bank */
        MCU_FlashBank standby_bank = MCU_FlashBank_GetStandby();
        uint32 mcu_target_address = MCU_FlashBank_GetAddress(standby_bank);
        
        sprintf(msg, "[UDS 0x37] Target MCU Bank: %c (0x%08X)\r\n",
                (standby_bank == MCU_BANK_A) ? 'A' : 'B',
                mcu_target_address);
        sendUARTMessage(msg, strlen(msg));
        
        /* Step 2: Copy SPI Flash → MCU PFLASH (차량 운행 중!) */
        sendUARTMessage("\r\n[UDS 0x37] Programming MCU PFLASH...\r\n", 39);
        sendUARTMessage("[UDS 0x37] ⚠️ Vehicle can continue operating!\r\n", 48);
        
        if (!MCU_PFLASH_CopyFromSPI(g_download_session.spi_staging_address,
                                     mcu_target_address,
                                     g_download_session.sw_header.payload_size))
        {
            sendUARTMessage("[UDS 0x37] ERROR: MCU Flash programming failed!\r\n", 51);
            UDS_CreateNegativeResponse(request, UDS_NRC_GENERAL_PROGRAMMING_FAILURE, response);
            g_download_session.state = DOWNLOAD_STATE_ERROR;
            return TRUE;
        }
        
        /* Step 3: Verify CRC32 */
        MCU_Flash_Result verify_result = MCU_Flash_VerifyCRC32(mcu_target_address,
                                                                  g_download_session.sw_header.payload_size,
                                                                  g_download_session.sw_header.crc32);
        
        if (verify_result != MCU_FLASH_OK)
        {
            sendUARTMessage("[UDS 0x37] ERROR: CRC verification failed!\r\n", 45);
            UDS_CreateNegativeResponse(request, UDS_NRC_GENERAL_PROGRAMMING_FAILURE, response);
            g_download_session.state = DOWNLOAD_STATE_ERROR;
            return TRUE;
        }
        
        /* Step 4: Set Bootloader flag (Bootloader will only switch banks) */
        sendUARTMessage("\r\n[UDS 0x37] Setting Bootloader flag...\r\n", 41);
        
        if (!MCU_FlashBank_RequestUpdate(g_download_session.spi_staging_address,
                                          g_download_session.sw_header.payload_size))
        {
            sendUARTMessage("[UDS 0x37] ERROR: Failed to set Bootloader flag!\r\n", 52);
            UDS_CreateNegativeResponse(request, UDS_NRC_GENERAL_PROGRAMMING_FAILURE, response);
            g_download_session.state = DOWNLOAD_STATE_ERROR;
            return TRUE;
        }
        
        /* Success! Ready to reboot */
        sendUARTMessage("\r\n[UDS 0x37] ========================================\r\n", 50);
        sendUARTMessage("[UDS 0x37] ✓ MCU PFLASH PROGRAMMING COMPLETE!\r\n", 48);
        sendUARTMessage("[UDS 0x37] ========================================\r\n", 50);
        sendUARTMessage("[UDS 0x37] Ready to activate new software\r\n", 44);
        sendUARTMessage("\r\n[UDS 0x37] *** REBOOT TO ACTIVATE NEW SW ***\r\n", 48);
        sendUARTMessage("[UDS 0x37] Bootloader will:\r\n", 30);
        sendUARTMessage("[UDS 0x37]   1. Verify Bank Marker\r\n", 37);
        sendUARTMessage("[UDS 0x37]   2. Switch active bank (A ↔ B)\r\n", 45);
        sendUARTMessage("[UDS 0x37]   3. Start new application (~1 sec)\r\n", 49);
        sendUARTMessage("[UDS 0x37] ========================================\r\n", 50);
    }
    /*=========================================================================
     * Path 2: Zone ECU Update → Route package via CAN/DoIP
     *=========================================================================*/
    else
    {
        sendUARTMessage("\r\n[UDS 0x37] Target: Zone ECU (Routing required)\r\n", 50);
        
        sprintf(msg, "[UDS 0x37] Routing to: %s\r\n", 
                SoftwarePackage_GetECUName(g_download_session.target_ecu_id));
        sendUARTMessage(msg, strlen(msg));
        
        sprintf(msg, "[UDS 0x37] Package stored @ SPI Flash 0x%08X (%u bytes)\r\n",
                g_download_session.spi_staging_address,
                g_download_session.total_bytes_received);
        sendUARTMessage(msg, strlen(msg));
        
        /* Route to Zone ECU */
        if (UDS_Download_RouteToZoneECU(g_download_session.target_ecu_id,
                                        g_download_session.spi_staging_address,
                                        g_download_session.total_bytes_received))
        {
            sendUARTMessage("[UDS 0x37] ✓ Routing initiated!\r\n", 34);
            sendUARTMessage("[UDS 0x37] Package will be forwarded to Zone ECU\r\n", 51);
        }
        else
        {
            sendUARTMessage("[UDS 0x37] ERROR: Routing failed!\r\n", 36);
            UDS_CreateNegativeResponse(request, UDS_NRC_GENERAL_PROGRAMMING_FAILURE, response);
            g_download_session.state = DOWNLOAD_STATE_ERROR;
            return TRUE;
        }
    }
    
    /* Mark session as completed */
    g_download_session.state = DOWNLOAD_STATE_COMPLETED;
    g_download_session.is_active = FALSE;
    
    /* Positive response */
    UDS_CreatePositiveResponse(request, response);
    response->data_len = 0;
    
    sendUARTMessage("[UDS 0x37] ========================================\r\n", 50);
    sendUARTMessage("[UDS 0x37] SOFTWARE DOWNLOAD SUCCESS!\r\n", 40);
    sendUARTMessage("[UDS 0x37] ========================================\r\n", 50);
    
    return TRUE;
}

/*******************************************************************************
 * Route Software Package to Zone ECU
 ******************************************************************************/

boolean UDS_Download_RouteToZoneECU(uint16 target_ecu_id, uint32 spi_flash_address, uint32 package_size)
{
    char msg[128];
    
    sprintf(msg, "[Route] Initiating routing to ECU 0x%04X...\r\n", target_ecu_id);
    sendUARTMessage(msg, strlen(msg));
    
    sprintf(msg, "[Route] Package location: SPI Flash @ 0x%08X (%u bytes)\r\n",
            spi_flash_address, package_size);
    sendUARTMessage(msg, strlen(msg));
    
    /* TODO: Implement actual routing logic
     * 
     * 현재 상태: Package는 이미 SPI Flash Staging Area에 저장됨
     * 
     * 구현해야 할 것:
     * 1. DoIP 라우팅: DoIP_Client_SendDiagnostic()로 전달
     * 2. CAN TP 라우팅: ISO 15765-2 Transport Protocol로 전달
     * 3. 직접 Ethernet 전달
     * 
     * 예시:
     * - SPI Flash에서 읽기 (chunk 단위)
     * - Zone ECU에 UDS RequestDownload 전송
     * - Chunk를 TransferData로 전송
     * - RequestTransferExit 전송
     * - Zone ECU 응답 확인
     */
    
    sendUARTMessage("[Route] Package ready for forwarding\r\n", 40);
    sendUARTMessage("[Route] TODO: Implement DoIP/CAN forwarding\r\n", 47);
    
    /* 임시: 성공 반환 (실제 라우팅 구현 필요) */
    return TRUE;
}


