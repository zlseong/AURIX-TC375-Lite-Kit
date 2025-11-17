/*******************************************************************************
 * @file    ota_manager.h
 * @brief   OTA Manager for Zone Package Processing
 * @details Handles Zone Package download, validation, extraction, and installation
 * 
 * @version 1.0
 * @date    2024-11-17
 ******************************************************************************/

#ifndef OTA_MANAGER_H
#define OTA_MANAGER_H

#include "Ifx_Types.h"
#include "zone_package.h"

/*******************************************************************************
 * OTA State Machine
 ******************************************************************************/

typedef enum
{
    OTA_STATE_IDLE = 0,
    OTA_STATE_DOWNLOADING,
    OTA_STATE_VERIFYING,
    OTA_STATE_EXTRACTING,
    OTA_STATE_INSTALLING,
    OTA_STATE_COMPLETE,
    OTA_STATE_ERROR
} OTA_State_t;

/*******************************************************************************
 * OTA Progress Information
 ******************************************************************************/

typedef struct
{
    OTA_State_t state;
    uint32 total_size;
    uint32 downloaded_size;
    uint32 progress_percent;
    char current_ecu[16];
} OTA_Progress_t;

/*******************************************************************************
 * Function Prototypes
 ******************************************************************************/

/**
 * @brief Initialize OTA Manager
 * @return TRUE if success, FALSE otherwise
 */
boolean OTA_Init(void);

/**
 * @brief Start Zone Package download (0x34 Request Download)
 * @param total_size Zone Package total size
 * @return TRUE if ready, FALSE otherwise
 */
boolean OTA_StartDownload(uint32 total_size);

/**
 * @brief Write Zone Package chunk (0x36 Transfer Data)
 * @param data Pointer to data chunk
 * @param size Size of chunk
 * @return TRUE if success, FALSE otherwise
 */
boolean OTA_WriteChunk(const uint8 *data, uint32 size);

/**
 * @brief Finish download and verify (0x37 Request Transfer Exit)
 * @return TRUE if valid, FALSE otherwise
 */
boolean OTA_FinishDownload(void);

/**
 * @brief Extract ZGW firmware from Zone Package and install
 * @return TRUE if success, FALSE otherwise
 */
boolean OTA_InstallZGWFirmware(void);

/**
 * @brief Distribute ECU firmware to Zone ECUs
 * @param ecu_id Target ECU ID
 * @return TRUE if success, FALSE otherwise
 */
boolean OTA_DistributeToZoneECU(const char *ecu_id);

/**
 * @brief Distribute firmware to all Zone ECUs in the package
 * @return TRUE if all success, FALSE if any failed
 */
boolean OTA_DistributeAllECUs(void);

/**
 * @brief Get current OTA progress
 * @param progress Output progress structure
 */
void OTA_GetProgress(OTA_Progress_t *progress);

/**
 * @brief Get current OTA state
 * @return Current state
 */
OTA_State_t OTA_GetState(void);

/**
 * @brief Check if OTA is in progress
 * @return TRUE if in progress, FALSE otherwise
 */
boolean OTA_IsInProgress(void);

/**
 * @brief Cancel current OTA operation
 */
void OTA_Cancel(void);

#endif /* OTA_MANAGER_H */

