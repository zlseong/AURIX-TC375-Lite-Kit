/*******************************************************************************
 * @file    uds_download.h
 * @brief   UDS Download Services for Flash Programming
 * @details Implements 0x34/0x36/0x37 services for software update via DoIP
 * 
 * @version 1.0
 * @date    2025-11-06
 ******************************************************************************/

#ifndef UDS_DOWNLOAD_H
#define UDS_DOWNLOAD_H

#include "Ifx_Types.h"
#include "uds_handler.h"
#include "software_package.h"
/* Flash4 driver removed - UDS Download uses MCU PFLASH only */

/*******************************************************************************
 * SPI Flash Staging Areas (Temporary Buffer)
 ******************************************************************************/

/* SPI Flash는 임시 버퍼 - 각 ECU당 Staging Area 할당 */
/* 실제 주소는 software_package.h에서 정의됨 (SPI_FLASH_ECU_xxx_START) */

/* Block sizes */
#define FLASH_ERASE_BLOCK_SIZE          0x00040000  /* 256KB per erase (0xDC cmd) */
#define FLASH_WRITE_MAX_SIZE            256         /* Max bytes per write (page size) */

/*******************************************************************************
 * Download Session State
 ******************************************************************************/

typedef enum
{
    DOWNLOAD_STATE_IDLE = 0,            /* No download in progress */
    DOWNLOAD_STATE_REQUESTED,           /* RequestDownload accepted */
    DOWNLOAD_STATE_TRANSFERRING,        /* TransferData in progress */
    DOWNLOAD_STATE_COMPLETED,           /* Transfer complete */
    DOWNLOAD_STATE_ERROR                /* Download error occurred */
} UDS_DownloadState;

/* Download Session Context */
typedef struct
{
    UDS_DownloadState state;            /* Current download state */
    uint32 flash_start_address;         /* Flash start address */
    uint32 flash_current_address;       /* Current write position */
    uint32 total_bytes_expected;        /* Total bytes to download */
    uint32 total_bytes_received;        /* Total bytes received so far */
    uint8  block_sequence_counter;      /* Transfer block counter (wraps at 0xFF) */
    uint16 max_block_length;            /* Max data bytes per TransferData */
    boolean is_active;                  /* Download session active flag */
    
    /* Software Package Information */
    SoftwarePackageHeader sw_header;    /* Parsed package header */
    boolean header_received;            /* Header parsed flag */
    uint16 target_ecu_id;               /* Target ECU ID */
    boolean is_for_this_ecu;            /* TRUE if target is ZGW */
    uint32 spi_staging_address;         /* SPI Flash staging area address */
} UDS_DownloadSession;

/*******************************************************************************
 * Public Functions
 ******************************************************************************/

/**
 * @brief Initialize UDS Download Module
 * @note Flash4 driver removed - uses MCU PFLASH only
 */
void UDS_Download_Init(void);

/**
 * @brief Handle 0x34 RequestDownload service
 * @param request UDS request
 * @param response UDS response (output)
 * @return TRUE if handled, FALSE otherwise
 */
boolean UDS_Service_RequestDownload(const UDS_Request *request, UDS_Response *response);

/**
 * @brief Handle 0x36 TransferData service
 * @param request UDS request
 * @param response UDS response (output)
 * @return TRUE if handled, FALSE otherwise
 */
boolean UDS_Service_TransferData(const UDS_Request *request, UDS_Response *response);

/**
 * @brief Handle 0x37 RequestTransferExit service
 * @param request UDS request
 * @param response UDS response (output)
 * @return TRUE if handled, FALSE otherwise
 */
boolean UDS_Service_RequestTransferExit(const UDS_Request *request, UDS_Response *response);

/**
 * @brief Get current download session state
 * @return Pointer to download session (read-only)
 */
const UDS_DownloadSession* UDS_Download_GetSession(void);

/**
 * @brief Reset download session (abort)
 */
void UDS_Download_Reset(void);

/**
 * @brief Route software package to Zone ECU
 * @param target_ecu_id Target ECU ID
 * @param spi_flash_address SPI Flash staging area address where package is stored
 * @param package_size Package size
 * @return TRUE if routing initiated, FALSE otherwise
 */
boolean UDS_Download_RouteToZoneECU(uint16 target_ecu_id, uint32 spi_flash_address, uint32 package_size);

#endif /* UDS_DOWNLOAD_H */

