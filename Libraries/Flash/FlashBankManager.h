/**********************************************************************************************************************
 * \file FlashBankManager.h
 * \copyright Copyright (C) Infineon Technologies AG 2019
 * 
 * Flash Bank Manager for TC375 Dual-Bank OTA Update with Dual Bootloaders
 * 
 * TC375 PFlash Configuration:
 *   - PFlash0 (Bank A): 0xA000_0000 - 0xA02F_FFFF (3MB)
 *     ├─ Bootloader A:  0xA000_0000 - 0xA000_FFFF (64KB)
 *     └─ Application A: 0xA001_0000 - 0xA02F_FFFF (2.94MB)
 * 
 *   - PFlash1 (Bank B): 0xA030_0000 - 0xA05F_FFFF (3MB)
 *     ├─ Bootloader B:  0xA030_0000 - 0xA030_FFFF (64KB)
 *     └─ Application B: 0xA031_0000 - 0xA05F_FFFF (2.94MB)
 * 
 * Boot Flow:
 *   1. BMHD0 → 0x8000_0000 (Bootloader A, fixed)
 *   2. Bootloader A reads DFlash bootTarget flag
 *   3. If bootTarget == 0: Jump to Application A (0x8001_0000)
 *   4. If bootTarget == 1: Jump to Bootloader B (0x8030_0000)
 *   5. Bootloader B jumps to Application B (0x8031_0000)
 * 
 * OTA Update Process:
 *   1. Application downloads firmware to inactive bank
 *   2. Application verifies downloaded firmware (CRC32)
 *   3. Application updates DFlash boot flag and resets
 *   4. Bootloader performs validation and rollback if needed
 *********************************************************************************************************************/

#ifndef FLASH_BANK_MANAGER_H_
#define FLASH_BANK_MANAGER_H_

#include "Ifx_Types.h"

/*******************************************************************************
 * Flash Bank Memory Map (TC375) - Dual Bootloader Architecture
 ******************************************************************************/

/* Bootloader Size (64KB per bank) */
#define BOOTLOADER_SIZE             0x00010000  /* 64KB */

/* Bank A (3MB Total) */
#define PFLASH_BANK_A_START         0xA0000000
#define PFLASH_BANK_A_SIZE          0x00300000  /* 3MB */
#define PFLASH_BANK_A_END           (PFLASH_BANK_A_START + PFLASH_BANK_A_SIZE - 1)

/* Bank A - Bootloader A (64KB) */
#define BOOTLOADER_A_START          0xA0000000
#define BOOTLOADER_A_SIZE           BOOTLOADER_SIZE
#define BOOTLOADER_A_END            (BOOTLOADER_A_START + BOOTLOADER_A_SIZE - 1)

/* Bank A - Application A (2.94MB) */
#define APPLICATION_A_START         0xA0010000
#define APPLICATION_A_SIZE          (PFLASH_BANK_A_SIZE - BOOTLOADER_SIZE)
#define APPLICATION_A_END           PFLASH_BANK_A_END

/* Bank B (3MB Total) */
#define PFLASH_BANK_B_START         0xA0300000
#define PFLASH_BANK_B_SIZE          0x00300000  /* 3MB */
#define PFLASH_BANK_B_END           (PFLASH_BANK_B_START + PFLASH_BANK_B_SIZE - 1)

/* Bank B - Bootloader B (64KB) */
#define BOOTLOADER_B_START          0xA0300000
#define BOOTLOADER_B_SIZE           BOOTLOADER_SIZE
#define BOOTLOADER_B_END            (BOOTLOADER_B_START + BOOTLOADER_B_SIZE - 1)

/* Bank B - Application B (2.94MB) */
#define APPLICATION_B_START         0xA0310000
#define APPLICATION_B_SIZE          (PFLASH_BANK_B_SIZE - BOOTLOADER_SIZE)
#define APPLICATION_B_END           PFLASH_BANK_B_END

/* Cached Execution Addresses */
#define PFLASH_CACHED_BASE          0x80000000

/* Bootloader Cached Addresses */
#define BOOTLOADER_A_CACHED         0x80000000  /* Entry point (BMHD0 STAD) */
#define BOOTLOADER_B_CACHED         0x80300000

/* Application Cached Addresses */
#define APPLICATION_A_CACHED        0x80010000
#define APPLICATION_B_CACHED        0x80310000

/* Legacy Compatibility (deprecated - use APPLICATION_x_CACHED instead) */
#define PFLASH_BANK_A_CACHED        APPLICATION_A_CACHED
#define PFLASH_BANK_B_CACHED        APPLICATION_B_CACHED

/* User Configuration Blocks (UCB) */
#define UCB_BASE_ADDRESS            0xAF400000
#define UCB_BMHD0_ADDRESS           0xAF400000  /* Boot Mode Header 0 */
#define UCB_BMHD1_ADDRESS           0xAF400200  /* Boot Mode Header 1 */
#define UCB_SIZE                    0x00006000  /* 24KB */

/* Data Flash for Bank Status Storage */
#define DFLASH_BASE_ADDRESS         0xAF000000  /* Primary DFlash base (256KB) */
#define DFLASH_BANK_STATUS_OFFSET   0x00000000  /* Bank status at start of DFlash */
#define DFLASH_SIZE                 0x00040000  /* 256KB (TC37x Primary DFlash) */

/* Alternative DFlash Access (EEPROM Emulation) */
#define DFLASH_EEPROM_BASE          0xAFC00000  /* EEPROM emulation area (128KB) */
#define DFLASH_EEPROM_SIZE          0x00020000  /* 128KB */

/* OLDA (Online Data Acquisition) for OTA Diagnostics (Optional) */
#define OLDA_BASE_ADDRESS_CACHED    0x8FE00000  /* Cached access */
#define OLDA_BASE_ADDRESS_PHYSICAL  0xAFE00000  /* Non-cached access */
#define OLDA_SIZE                   0x00080000  /* 512KB */
#define OLDA_OTA_LOG_OFFSET         0x00000000  /* OTA diagnostic log storage */

/* SCU (System Control Unit) - Reset Type Detection */
#define SCU_BASE_ADDRESS            0xF0036000
#define SCU_STMEM3_OFFSET           0x01C0  /* Application Reset detection */
#define SCU_STMEM4_OFFSET           0x01C4  /* Cold PowerOn Reset */
#define SCU_STMEM5_OFFSET           0x01C8  /* PowerOn Reset */
#define SCU_STMEM6_OFFSET           0x01CC  /* System Reset */

/* SCU_STMEMx absolute addresses */
#define SCU_STMEM3_ADDRESS          (SCU_BASE_ADDRESS + SCU_STMEM3_OFFSET)  /* 0xF00361C0 */
#define SCU_STMEM4_ADDRESS          (SCU_BASE_ADDRESS + SCU_STMEM4_OFFSET)  /* 0xF00361C4 */
#define SCU_STMEM5_ADDRESS          (SCU_BASE_ADDRESS + SCU_STMEM5_OFFSET)  /* 0xF00361C8 */
#define SCU_STMEM6_ADDRESS          (SCU_BASE_ADDRESS + SCU_STMEM6_OFFSET)  /* 0xF00361CC */

/*******************************************************************************
 * Type Definitions
 ******************************************************************************/

/**
 * \brief Bank Metadata (stored at the beginning of each bank)
 * 
 * Location:
 *   Bank A: 0xA0000000 (first 256 bytes reserved)
 *   Bank B: 0xA0300000 (first 256 bytes reserved)
 * 
 * Purpose:
 *   - Quick synchronization check (no need for full CRC)
 *   - Version tracking
 *   - Integrity verification
 */
typedef struct
{
    uint32 magic_number;           /* 0x42414E4B ("BANK") - validation marker */
    uint32 firmware_version;       /* 0xAABBCCDD (vAA.BB.CC.DD) */
    uint32 build_timestamp;        /* Unix timestamp of build */
    uint32 total_size;             /* Firmware size in bytes (excluding metadata) */
    uint32 crc32;                  /* CRC32 of entire bank (excluding this metadata) */
    char   version_string[32];     /* Human-readable version "v1.2.3-20241114" */
    uint8  bank_id;                /* 0=Bank A, 1=Bank B */
    uint8  is_valid;               /* 1=Valid, 0=Invalid/Erased */
    uint8  reserved[186];          /* Padding to 256 bytes */
} BankMetadata_t;

#define BANK_METADATA_MAGIC     0x42414E4B  /* "BANK" in ASCII */
#define BANK_METADATA_SIZE      256         /* Must be aligned to Flash page */

/**
 * \brief Flash Bank Status Flags (8-bit system)
 * 
 * Stored in Data Flash for fast access and frequent updates.
 * This structure survives power cycles and resets.
 * 
 * Storage Location: DFLASH_BASE_ADDRESS (0xAF000000) + DFLASH_BANK_STATUS_OFFSET
 * 
 * Bit Layout:
 *   Bit 0: Boot Target (0=Bank A at 0xA0001000, 1=Bank B at 0xA0300000)
 *   Bit 1-2: Bank A Status (00=OK, 01=UPDATING, 10=ERROR, 11=Reserved)
 *   Bit 3-4: Bank B Status (00=OK, 01=UPDATING, 10=ERROR, 11=Reserved)
 *   Bit 5: Banks Identical (0=Different, 1=Identical after CRC check)
 *   Bit 6: Critical Error (0=OK, 1=Both banks failed)
 *   Bit 7: Sync In Progress (0=No, 1=Synchronization running)
 */
typedef union
{
    uint8 value;  /* Combined 8-bit value */
    
    struct
    {
        uint8 bootTarget      : 1;  /* Bit 0: Boot Target (0=Bank A, 1=Bank B) */
        uint8 statusA         : 2;  /* Bit 1-2: Bank A Status (OK/UPDATING/ERROR) */
        uint8 statusB         : 2;  /* Bit 3-4: Bank B Status (OK/UPDATING/ERROR) */
        uint8 banksIdentical  : 1;  /* Bit 5: Banks Identical (0=Different, 1=Same) */
        uint8 criticalError   : 1;  /* Bit 6: Critical Error (both banks failed) */
        uint8 syncInProgress  : 1;  /* Bit 7: Synchronization in progress */
    } bits;
} FlashBankStatus_t;

/* Bank Status Values (2-bit encoding) */
#define BANK_STATUS_OK          0x00  /* 00: Bank is valid and operational */
#define BANK_STATUS_UPDATING    0x01  /* 01: OTA update in progress */
#define BANK_STATUS_ERROR       0x02  /* 10: Bank is corrupted or invalid */
#define BANK_STATUS_RESERVED    0x03  /* 11: Reserved for future use */

/**
 * \brief Reset Type enumeration (from SCU_STMEM3 register)
 * 
 * Used to determine the cause of the last reset, which helps decide
 * whether a rollback is needed during boot initialization.
 */
typedef enum
{
    RESET_TYPE_UNKNOWN = 0,
    RESET_TYPE_COLD_POWER_ON,
    RESET_TYPE_WARM_POWER_ON,
    RESET_TYPE_SYSTEM_RESET,
    RESET_TYPE_APPLICATION_RESET
} ResetType_t;

/**
 * \brief Flash Bank enumeration
 */
typedef enum
{
    FLASH_BANK_A = 0,
    FLASH_BANK_B
} FlashBank_t;

/**
 * \brief Bank Status enumeration
 */
typedef enum
{
    BANK_STATE_UNKNOWN = 0,
    BANK_STATE_EMPTY,
    BANK_STATE_ACTIVE,
    BANK_STATE_GOLDEN,
    BANK_STATE_DOWNLOADING,
    BANK_STATE_READY,
    BANK_STATE_VALIDATED,
    BANK_STATE_ERROR
} BankStatus_t;

/**
 * \brief Flash Bank Information structure
 */
typedef struct
{
    FlashBank_t bank;             /* Bank identifier */
    uint32 startAddress;          /* Physical start address */
    uint32 size;                  /* Bank size in bytes */
    BankStatus_t status;          /* Current bank status */
    uint32 firmwareVersion;       /* Firmware version stored in bank */
    uint32 firmwareCrc32;         /* CRC32 of firmware in bank */
} FlashBankInfo_t;

/*******************************************************************************
 * Function Prototypes
 ******************************************************************************/

/**
 * \brief Initialize Flash Bank Manager
 * 
 * This function must be called once during system initialization.
 * It reads the Boot Mode Header (BMHD) to determine which bank is currently active.
 * 
 * Design Rule (Normal Operation):
 *   - Bank A: Primary ACTIVE bank (default boot target)
 *   - Bank B: GOLDEN backup (safe fallback)
 * 
 * During Rollback:
 *   - Boot flag switches to Bank B temporarily
 *   - After validation, Bank A is restored from Bank B
 *   - Boot flag returns to Bank A
 * 
 * Boot Initialization Sequence:
 *   1. Check reset type (SCU_STMEM3)
 *   2. Read DFlash status flags
 *   3. Determine active boot bank from BMHD
 *   4. If rollback detected, start restoration process
 */
void FlashBank_Init(void);

/**
 * \brief Get last reset type from SCU registers
 * 
 * Reads SCU_STMEM3 register to determine the cause of the last reset.
 * This is useful for detecting rollback scenarios.
 * 
 * \return Reset type enumeration
 */
ResetType_t FlashBank_GetResetType(void);

/**
 * \brief Get currently active boot bank (from BMHD)
 * 
 * \return FLASH_BANK_A or FLASH_BANK_B based on Boot Mode Header
 */
FlashBank_t FlashBank_GetActiveBoot(void);

/**
 * \brief Get golden/backup bank (opposite of active)
 * 
 * \return FLASH_BANK_B if active is A, or FLASH_BANK_A if active is B
 */
FlashBank_t FlashBank_GetGoldenBank(void);

/**
 * \brief Get information about a specific flash bank
 * 
 * \param bank Target bank
 * \param info Pointer to structure to receive bank information
 * \return TRUE if successful, FALSE otherwise
 */
boolean FlashBank_GetInfo(FlashBank_t bank, FlashBankInfo_t* info);

/**
 * \brief Get status of a specific flash bank
 * 
 * \param bank Target bank
 * \return Bank status
 */
BankStatus_t FlashBank_GetStatus(FlashBank_t bank);

/**
 * \brief Set status of a specific flash bank
 * 
 * \param bank Target bank
 * \param status New status
 */
void FlashBank_SetStatus(FlashBank_t bank, BankStatus_t status);

/**
 * \brief Get current bank status flags (8-bit system)
 * 
 * Reads the status flags from Data Flash.
 * 
 * \return Current bank status flags
 */
FlashBankStatus_t FlashBank_GetStatusFlags(void);

/**
 * \brief Read bank status from DFlash
 * 
 * \param status Pointer to status structure to fill
 * \return TRUE if read successful, FALSE otherwise
 */
boolean FlashBank_ReadDFlashStatus(FlashBankStatus_t *status);

/**
 * \brief Write bank status to DFlash
 * 
 * \param status Status structure to write
 * \return TRUE if write successful, FALSE otherwise
 */
boolean FlashBank_WriteDFlashStatus(FlashBankStatus_t status);

/**
 * \brief Read metadata from a specific bank
 * 
 * \param bank Bank to read from (FLASH_BANK_A or FLASH_BANK_B)
 * \param metadata Pointer to metadata structure to fill
 * \return TRUE if metadata is valid, FALSE otherwise
 */
boolean FlashBank_ReadMetadata(FlashBank_t bank, BankMetadata_t *metadata);

/**
 * \brief Write metadata to a specific bank
 * 
 * \param bank Bank to write to (FLASH_BANK_A or FLASH_BANK_B)
 * \param metadata Pointer to metadata structure to write
 * \return TRUE if write successful, FALSE otherwise
 */
boolean FlashBank_WriteMetadata(FlashBank_t bank, const BankMetadata_t *metadata);

/**
 * \brief Quick check if banks are synchronized (using metadata only)
 * 
 * Compares metadata (version, CRC, timestamp) between Bank A and Bank B
 * without calculating full CRC. Much faster than full synchronization check.
 * 
 * \return TRUE if banks are synchronized, FALSE otherwise
 */
boolean FlashBank_QuickSyncCheck(void);

/**
 * \brief Set bank status flags (deprecated - use FlashBank_WriteDFlashStatus)
 * 
 * Writes the status flags to Data Flash.
 * Updates are immediate and persist across resets.
 * 
 * \param flags New status flags to write
 * \return TRUE if write successful, FALSE otherwise
 */
boolean FlashBank_SetStatusFlags(FlashBankStatus_t flags);

/**
 * \brief Update Boot Mode Header (BMHD) to switch boot bank
 * 
 * ⚠️ WARNING: This function modifies UCB (User Configuration Block)
 * - UCB writes are limited (typically ~100 times)
 * - Incorrect writes can brick the device
 * - Requires password protection
 * - Production use requires careful validation
 * 
 * Modifies UCB BMHD to change the next boot address.
 * 
 * \param targetBank Bank to boot from after reset
 * \return TRUE if BMHD update successful, FALSE otherwise
 */
boolean FlashBank_UpdateBootTarget(FlashBank_t targetBank);

/**
 * \brief Validate current active bank after successful boot and runtime check
 * 
 * This function should be called after the system has been running stably
 * for a certain period (e.g., 30 seconds ~ 1 minute) to confirm the firmware is working.
 * 
 * Process:
 *   1. Mark active bank as VALIDATED
 *   2. If inactive bank is GOLDEN, copy validated firmware to make new golden image
 *   3. Update boot configuration
 * 
 * Typical usage:
 *   - Call from main loop after stability check timer expires
 *   - Service center: After functional tests pass
 *   - OTA: After vehicle operates normally for defined duration
 * 
 * \return TRUE if validation successful, FALSE otherwise
 */
boolean FlashBank_ValidateActiveFirmware(void);

/**
 * \brief Check if current boot is the first boot of new firmware
 * 
 * Used to determine if we need to start the validation timer.
 * 
 * \return TRUE if this is first boot of unvalidated firmware, FALSE otherwise
 */
boolean FlashBank_IsFirstBoot(void);

/**
 * \brief SW Synchronization: Copy Active Bank → Inactive Bank
 * 
 * Universal synchronization function - copies currently running validated firmware
 * to the inactive bank as a Golden backup image.
 * 
 * Use Cases:
 *   1. After normal boot: Bank A (Active) → Bank B (Inactive)
 *   2. After OTA update: Bank B (Active) → Bank A (Inactive)
 * 
 * This function is reusable for both scenarios!
 * 
 * Process:
 *   1. Determine current active bank (from BMHD or status flags)
 *   2. Verify active bank is validated and stable
 *   3. Erase inactive bank
 *   4. Copy Active Bank → Inactive Bank (3MB, sector by sector)
 *   5. Calculate and verify CRC32 for each chunk
 *   6. Update status flags (banksIdentical = 1)
 * 
 * Runs as a background task after boot validation completes.
 * Includes Watchdog refresh to prevent timeout during long copy operation.
 * 
 * \return TRUE if synchronization successful, FALSE otherwise
 */
boolean FlashBank_SWSynchronization(void);

/**
 * \brief Trigger rollback to Golden Image (switch boot flag)
 * 
 * This function is called when:
 *   - Boot validation fails (watchdog, startup errors)
 *   - Runtime critical error detected
 *   - Manual rollback requested
 * 
 * Process (Immediate - Fast Rollback):
 *   1. Mark current active bank as ERROR
 *   2. Switch Boot Flag to Golden bank (modify BMHD)
 *   3. Perform system reset
 * 
 * After reset:
 *   - System boots from Golden bank (working firmware)
 *   - Vehicle operates normally
 *   - Background task will restore primary bank later
 * 
 * \return This function does not return (system resets)
 */
void FlashBank_RollbackToGolden(void) __attribute__((noreturn));

/**
 * \brief Restore primary bank from golden (background task after rollback)
 * 
 * This function is called automatically when:
 *   - System booted from Golden bank (not primary)
 *   - Golden bank has been validated as stable
 * 
 * Process (Background - Takes time):
 *   1. Erase primary bank (Bank A)
 *   2. Copy Golden bank → Primary bank
 *   3. Verify copy
 *   4. Switch Boot Flag back to primary bank
 *   5. Perform system reset
 * 
 * After reset:
 *   - System boots from primary bank (Bank A)
 *   - Both banks now contain validated firmware
 *   - Normal operation restored
 * 
 * \return TRUE if restoration successful, FALSE otherwise
 */
boolean FlashBank_RestorePrimaryFromGolden(void);

/**
 * \brief Write OTA diagnostic log to OLDA memory (optional)
 * 
 * Stores OTA event logs in the OLDA region for debugging and diagnostics.
 * This is useful for post-mortem analysis of OTA failures.
 * 
 * \param logMessage Pointer to log message string
 * \param length Length of log message
 * \return TRUE if log write successful, FALSE otherwise
 */
boolean FlashBank_WriteOTALog(const char* logMessage, uint32 length);

/**
 * \brief Read OTA diagnostic log from OLDA memory (optional)
 * 
 * Retrieves OTA event logs from the OLDA region.
 * 
 * \param buffer Pointer to buffer to receive log data
 * \param maxLength Maximum length to read
 * \return Number of bytes read, 0 if no log available
 */
uint32 FlashBank_ReadOTALog(char* buffer, uint32 maxLength);

#ifdef DEBUG
/**
 * \brief Debug helper: Copy firmware from Bank A to Bank B for testing
 * 
 * This function is only available in debug builds.
 * Used when debugger downloads only to Bank A, but you want to test dual-bank.
 * 
 * Call this once during system initialization in debug mode.
 */
void FlashBank_DebugCopyAtoB(void);
#endif

#endif /* FLASH_BANK_MANAGER_H_ */

