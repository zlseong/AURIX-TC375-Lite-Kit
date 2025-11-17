/*******************************************************************************
 * @file    zone_package.h
 * @brief   Zone Package Structure Definitions for OTA
 * @details Defines the hierarchical header structure for Zone OTA packages
 * 
 * @version 1.0
 * @date    2024-11-17
 ******************************************************************************/

#ifndef ZONE_PACKAGE_H
#define ZONE_PACKAGE_H

#include "Ifx_Types.h"

/*******************************************************************************
 * Magic Numbers
 ******************************************************************************/

#define ZONE_PACKAGE_MAGIC      0x5A4F4E45  /* "ZONE" */
#define ECU_METADATA_MAGIC      0x4543554D  /* "ECUM" */

/*******************************************************************************
 * Configuration
 ******************************************************************************/

#define MAX_ECUS_IN_ZONE        16
#define MAX_DEPENDENCIES        8

/*******************************************************************************
 * Zone Package Header (1KB)
 ******************************************************************************/

typedef struct
{
    uint32 magic_number;        /* 0x5A4F4E45 ("ZONE") - Zone Package Magic num */
    uint32 version;             /* Zone Package format version (0x00010000) */
    uint32 zone_package_id;     /* Zone package ID (unique identifier) */
    char   zone_id[16];         /* Zone ID: "ZONE_01", "ZONE_02", etc. */
    uint32 total_size;          /* Zone package size (bytes) */
    uint8  package_count;       /* Number of ECU packages (Updatable ECU#01~ECU#xx) */
    uint8  reserved1[3];
    uint32 zone_crc32;          /* CRC */
    uint32 timestamp;           /* Package creation time (Unix timestamp) */
    char   zone_name[32];       /* Human-readable: "Zone_Front_Left", "Zone_Rear" */
    uint8  reserved2[180];      /* Padding to 256 bytes */
    
    /* ECU Table (each 64 bytes, max 16 ECUs = 1024 bytes total) */
    struct {
        char   ecu_id[16];      /* "ECU_091", "ECU_011", etc. */
        uint32 offset;          /* Offset in Zone Package */
        uint32 size;            /* Total ECU Package size (metadata + firmware) */
        uint32 metadata_size;   /* ECU Metadata size (usually 256 bytes) */
        uint32 firmware_size;   /* Firmware binary size */
        uint32 firmware_version;/* 0x00010203 (v1.2.3) */
        uint32 crc32;           /* CRC32 of ECU Package */
        uint8  priority;        /* Update priority (0=highest) */
        uint8  reserved[23];
    } ecu_table[MAX_ECUS_IN_ZONE];
    
} ZonePackageHeader_t;  /* Total: 1024 bytes (1KB) */

/*******************************************************************************
 * ECU Metadata (256 bytes) - Per ECU Package Header
 ******************************************************************************/

typedef struct
{
    uint32 magic_number;        /* 0x4543554D ("ECUM") */
    char   ecu_id[16];          /* "ECU_091" */
    uint32 firmware_version;    /* 0x00010203 (v1.2.3) */
    uint32 hardware_version;    /* 0x00010000 (HW v1.0.0) */
    uint32 firmware_size;       /* Firmware binary size */
    uint32 firmware_crc32;      /* Firmware CRC32 */
    uint32 build_timestamp;     /* Build time (Unix timestamp) */
    char   version_string[32];  /* "v1.2.3-20241117" */
    
    /* Dependency Information */
    uint8  dependency_count;    /* Number of dependencies */
    uint8  reserved1[3];
    
    struct {
        char   ecu_id[16];      /* Dependent ECU ID */
        uint32 min_version;     /* Minimum required version */
        uint8  reserved[12];
    } dependencies[MAX_DEPENDENCIES];  /* 8 deps × 32 bytes = 256 bytes */
    
    uint8  reserved2[144];      /* Padding to 256 bytes total */
    
} ECUMetadata_t;  /* Total: 256 bytes */

/*******************************************************************************
 * Function Prototypes
 ******************************************************************************/

/**
 * @brief Parse Zone Package Header from External Flash
 * @param addr External Flash address
 * @param header Output structure
 * @return TRUE if valid, FALSE otherwise
 */
boolean ZonePackage_ParseHeader(uint32 addr, ZonePackageHeader_t *header);

/**
 * @brief Find ECU Metadata in Zone Package
 * @param zone_header Zone Package Header
 * @param ecu_id Target ECU ID (e.g., "ECU_091")
 * @param metadata Output structure
 * @return TRUE if found, FALSE otherwise
 */
boolean ZonePackage_FindECUMetadata(const ZonePackageHeader_t *zone_header, 
                                     const char *ecu_id, 
                                     ECUMetadata_t *metadata);

/**
 * @brief Validate Zone Package CRC32
 * @param zone_header Zone Package Header
 * @return TRUE if valid, FALSE otherwise
 */
boolean ZonePackage_ValidateCRC(const ZonePackageHeader_t *zone_header);

/**
 * @brief Print Zone Package information (for debugging)
 * @param zone_header Zone Package Header
 */
void ZonePackage_PrintInfo(const ZonePackageHeader_t *zone_header);

#endif /* ZONE_PACKAGE_H */

