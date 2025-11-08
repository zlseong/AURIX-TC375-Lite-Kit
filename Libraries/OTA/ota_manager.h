/******************************************************************************
 * @file     ota_manager.h
 * @brief    OTA Package Manager (DISABLED - External Flash Not Available)
 * @version  1.0.0
 * @date     2025-11-05
 * 
 * @note     This module is currently disabled as external SPI Flash is not
 *           available. All functions are stubbed to return errors.
 *****************************************************************************/

#ifndef OTA_MANAGER_H
#define OTA_MANAGER_H

/******************************************************************************
 * Includes
 *****************************************************************************/
#include "Ifx_Types.h"

/******************************************************************************
 * Constants
 *****************************************************************************/
#define OTA_MAX_PACKAGES            32
#define OTA_METADATA_ADDRESS        0x000000
#define OTA_METADATA_SIZE           4096
#define OTA_DATA_START_ADDRESS      0x001000
#define OTA_MAGIC                   0x4F54414D
#define OTA_VERSION                 0x0100
#define OTA_MAX_ECU_ID_LEN          16

/******************************************************************************
 * OTA Package Target Type
 *****************************************************************************/
typedef enum
{
    OTA_TARGET_ZGW          = 0x01,
    OTA_TARGET_CAN_ECU      = 0x02,
    OTA_TARGET_LIN_ECU      = 0x03,
    OTA_TARGET_ETH_ECU      = 0x04
} OTA_TargetType;

/******************************************************************************
 * OTA Package Bus Type
 *****************************************************************************/
typedef enum
{
    OTA_BUS_CAN             = 0x01,
    OTA_BUS_LIN             = 0x02,
    OTA_BUS_ETHERNET        = 0x03
} OTA_BusType;

/******************************************************************************
 * OTA Package State
 *****************************************************************************/
typedef enum
{
    OTA_STATE_EMPTY         = 0x00,
    OTA_STATE_DOWNLOADING   = 0x01,
    OTA_STATE_READY         = 0x02,
    OTA_STATE_DEPLOYING     = 0x03,
    OTA_STATE_DEPLOYED      = 0x04,
    OTA_STATE_FAILED        = 0x05
} OTA_PackageState;

/******************************************************************************
 * OTA Result Codes
 *****************************************************************************/
typedef enum
{
    OTA_OK                      = 0x00,
    OTA_ERROR_NOT_INITIALIZED   = 0x01,
    OTA_ERROR_INVALID_PARAMETER = 0x02,
    OTA_ERROR_NO_SPACE          = 0x03,
    OTA_ERROR_NOT_FOUND         = 0x04,
    OTA_ERROR_FLASH_ERROR       = 0x05,
    OTA_ERROR_INVALID_STATE     = 0x06,
    OTA_ERROR_CHECKSUM_MISMATCH = 0x07,
    OTA_ERROR_SIZE_MISMATCH     = 0x08
} OTA_Result;

/******************************************************************************
 * OTA Manager Structure (Stub)
 *****************************************************************************/
typedef struct
{
    boolean is_initialized;
} OTA_Manager;

/******************************************************************************
 * Function Prototypes (All Stubbed)
 *****************************************************************************/

/**
 * @brief Initialize OTA Manager (STUBBED)
 */
static inline OTA_Result OTA_Manager_Init(OTA_Manager *manager, void *flash)
{
    (void)manager;
    (void)flash;
    return OTA_ERROR_NOT_INITIALIZED;
}

/**
 * @brief Allocate space for new OTA package (STUBBED)
 */
static inline OTA_Result OTA_Manager_AllocatePackage(OTA_Manager *manager,
                                                     const char *target_ecu_id,
                                                     uint32 package_size,
                                                     OTA_TargetType target_type,
                                                     OTA_BusType bus_type,
                                                     uint16 *out_package_id)
{
    (void)manager;
    (void)target_ecu_id;
    (void)package_size;
    (void)target_type;
    (void)bus_type;
    (void)out_package_id;
    return OTA_ERROR_NOT_INITIALIZED;
}

/**
 * @brief Write data chunk to package (STUBBED)
 */
static inline OTA_Result OTA_Manager_WriteChunk(OTA_Manager *manager,
                                                uint16 package_id,
                                                uint32 offset,
                                                const uint8 *data,
                                                uint32 length)
{
    (void)manager;
    (void)package_id;
    (void)offset;
    (void)data;
    (void)length;
    return OTA_ERROR_NOT_INITIALIZED;
}

/**
 * @brief Finalize package download (STUBBED)
 */
static inline OTA_Result OTA_Manager_FinalizePackage(OTA_Manager *manager,
                                                     uint16 package_id,
                                                     uint32 expected_checksum)
{
    (void)manager;
    (void)package_id;
    (void)expected_checksum;
    return OTA_ERROR_NOT_INITIALIZED;
}

/**
 * @brief Read package data (STUBBED)
 */
static inline OTA_Result OTA_Manager_ReadPackage(OTA_Manager *manager,
                                                 uint16 package_id,
                                                 uint32 offset,
                                                 uint8 *data,
                                                 uint32 length)
{
    (void)manager;
    (void)package_id;
    (void)offset;
    (void)data;
    (void)length;
    return OTA_ERROR_NOT_INITIALIZED;
}

/**
 * @brief Mark package as deployed (STUBBED)
 */
static inline OTA_Result OTA_Manager_MarkDeployed(OTA_Manager *manager,
                                                  uint16 package_id)
{
    (void)manager;
    (void)package_id;
    return OTA_ERROR_NOT_INITIALIZED;
}

/**
 * @brief Delete package (STUBBED)
 */
static inline OTA_Result OTA_Manager_DeletePackage(OTA_Manager *manager,
                                                   uint16 package_id)
{
    (void)manager;
    (void)package_id;
    return OTA_ERROR_NOT_INITIALIZED;
}

/**
 * @brief Get available space (STUBBED)
 */
static inline uint32 OTA_Manager_GetAvailableSpace(OTA_Manager *manager)
{
    (void)manager;
    return 0;
}

#endif /* OTA_MANAGER_H */
