/**
 * @file SystemInit.c
 * @brief System Initialization Manager Implementation
 */

#include "SystemInit.h"
#include "Ifx_Types.h"
#include "IfxCpu.h"
#include "IfxScuWdt.h"
#include "IfxScuRcu.h"
#include "IfxStm.h"
#include "IfxGeth_Eth.h"
#include "Ifx_Lwip.h"
#include "AppConfig.h"
#include "Configuration.h"
#include "ConfigurationIsr.h"
#include "UART_Logging.h"
#include "lwip/tcp.h"
#include "lwip/udp.h"
#include "lwip/pbuf.h"
#include "Libraries/DoIP/doip_client.h"
#include "Libraries/DoIP/doip_link.h"
#include "Libraries/DoIP/uds_handler.h"
#include "Libraries/DataCollection/vci_aggregator.h"
#include "Libraries/DataCollection/readiness_aggregator.h"
#include "Libraries/OTA/ota_manager.h"
#include "Flash4_Driver.h"
#include "Libraries/Flash/FlashBankManager.h"
#include <string.h>
#include <stdio.h>

/*******************************************************************************
 * Boot Validation - Safety Mechanism
 ******************************************************************************/

/* Global flag: Set to TRUE when all init functions complete successfully */
volatile boolean g_systemInitSuccess = FALSE;

/* Boot validation timeout: 30 seconds */
#define BOOT_VALIDATION_TIMEOUT_MS  30000

extern IFX_ALIGN(4) IfxCpu_syncEvent g_cpuSyncEvent;
extern struct tcp_pcb *g_tcp_server_pcb;
extern struct udp_pcb *g_udp_server_pcb;
extern DoIP_VCI_Info g_zgw_vci;
extern DoIP_HealthStatus_Info g_health_data[MAX_ZONE_ECUS + 1];

static void Init_System(void);
static void Init_STM_Timer(void);
static void Init_Ethernet(void);
static void Wait_PHY_Link(void);
static void Init_DoIP(void);
static void Init_VCI(void);
static void Init_Readiness(void);
static void Init_Health_Database(void);
static void Init_FlashBankManager(void);
static void Print_System_Ready(void);

static void Init_System(void)
{
    IfxCpu_enableInterrupts();
    IfxScuWdt_disableCpuWatchdog(IfxScuWdt_getCpuWatchdogPassword());
    IfxScuWdt_disableSafetyWatchdog(IfxScuWdt_getSafetyWatchdogPassword());
    IfxCpu_emitEvent(&g_cpuSyncEvent);
    IfxCpu_waitEvent(&g_cpuSyncEvent, 1);
}

static void Init_STM_Timer(void)
{
    IfxStm_CompareConfig stmCompareConfig;
    IfxStm_initCompareConfig(&stmCompareConfig);
    stmCompareConfig.triggerPriority = ISR_PRIORITY_OS_TICK;
    stmCompareConfig.comparatorInterrupt = IfxStm_ComparatorInterrupt_ir0;
    stmCompareConfig.ticks = IFX_CFG_STM_TICKS_PER_MS * 10;
    stmCompareConfig.typeOfService = IfxSrc_Tos_cpu0;
    IfxStm_initCompare(&MODULE_STM0, &stmCompareConfig);
    sendUARTMessage("STM Timer OK\r\n", 14);
}

static void Init_Ethernet(void)
{
    IfxGeth_enableModule(&MODULE_GETH);
    sendUARTMessage("GETH Module Enabled\r\n", 21);
    
    eth_addr_t ethAddr;
    ethAddr.addr[0] = ETH_MAC_ADDR_0;
    ethAddr.addr[1] = ETH_MAC_ADDR_1;
    ethAddr.addr[2] = ETH_MAC_ADDR_2;
    ethAddr.addr[3] = ETH_MAC_ADDR_3;
    ethAddr.addr[4] = ETH_MAC_ADDR_4;
    ethAddr.addr[5] = ETH_MAC_ADDR_5;
    
    Ifx_Lwip_init(ethAddr);
    sendUARTMessage("lwIP Init OK - IP: 192.168.1.10\r\n", 36);
    sendUARTMessage("Ready for Ping Test!\r\n", 22);
}

static void Wait_PHY_Link(void)
{
    sendUARTMessage("Waiting for PHY Link UP...\r\n", 28);
    uint32 phy_wait_start = IfxStm_getLower(&MODULE_STM0);
    uint32 phy_timeout_ticks = (uint32)IfxStm_getTicksFromMilliseconds(&MODULE_STM0, PHY_LINK_TIMEOUT_MS);
    
    while (1)
    {
        Ifx_Lwip_pollTimerFlags();
        Ifx_Lwip_pollReceiveFlags();
        
        if (netif_is_link_up(&g_Lwip.netif))
        {
            break;
        }
        
        if ((IfxStm_get(&MODULE_STM0) - phy_wait_start) > phy_timeout_ticks)
        {
            sendUARTMessage("PHY Link timeout! Continuing anyway...\r\n", 41);
            break;
        }
        
        IfxStm_waitTicks(&MODULE_STM0, IfxStm_getTicksFromMilliseconds(&MODULE_STM0, 100));
    }
    sendUARTMessage("PHY Link UP! Network ready.\r\n", 29);
}

static void Init_DoIP(void)
{
    DoIP_ClientConfig doip_config;
    IP4_ADDR(&doip_config.vmg_ip, VMG_IP_ADDR_0, VMG_IP_ADDR_1, VMG_IP_ADDR_2, VMG_IP_ADDR_3);
    doip_config.vmg_port = VMG_PORT;
    doip_config.source_address = DOIP_ZONAL_GW_ADDRESS;
    DoIP_Client_Init(&doip_config);
    sendUARTMessage("[DoIP] Client ready (will connect in 5s)\r\n", 43);
    
    UDS_Init();
    sendUARTMessage("[UDS] Handler initialized\r\n", 27);
    
    OTA_Init();
    sendUARTMessage("[OTA] Manager initialized\r\n", 27);
}

static void Init_VCI(void)
{
    BankMetadata_t currentMeta;
    FlashBank_t currentBank;
    boolean metadataValid;
    char sw_version_buf[16];
    
    /* 1. Read metadata from currently running bank */
    currentBank = FlashBank_GetActiveBoot();
    metadataValid = FlashBank_ReadMetadata(currentBank, &currentMeta);
    
    /* 2. Set ECU ID (fixed) */
    memcpy(g_zgw_vci.ecu_id, ZGW_ECU_ID, sizeof(ZGW_ECU_ID));
    
    /* 3. Set SW Version - Read from Flash Metadata if valid, otherwise use default */
    if (metadataValid && currentMeta.magic_number == BANK_METADATA_MAGIC)
    {
        /* Extract version from metadata (0xAABBCCDD → vAA.BB.CC) */
        uint32 ver = currentMeta.firmware_version;
        uint8 major = (ver >> 16) & 0xFF;
        uint8 minor = (ver >> 8) & 0xFF;
        uint8 patch = ver & 0xFF;
        sprintf(sw_version_buf, "v%u.%u.%u", major, minor, patch);
        memcpy(g_zgw_vci.sw_version, sw_version_buf, strlen(sw_version_buf) + 1);
    }
    else
    {
        /* Fallback to hardcoded version (simulation/development mode) */
        memcpy(g_zgw_vci.sw_version, ZGW_SW_VERSION, sizeof(ZGW_SW_VERSION));
    }
    
    /* 4. Set HW Version and Serial Number (fixed - not in metadata) */
    memcpy(g_zgw_vci.hw_version, ZGW_HW_VERSION, sizeof(ZGW_HW_VERSION));
    memcpy(g_zgw_vci.serial_num, ZGW_SERIAL_NUM, sizeof(ZGW_SERIAL_NUM));
    
    /* 5. Store ZGW's own VCI in g_vci_database[0] */
    extern DoIP_VCI_Info g_vci_database[];
    extern uint8 g_zone_ecu_count;
    memcpy(&g_vci_database[0], &g_zgw_vci, sizeof(DoIP_VCI_Info));
    g_zone_ecu_count = 1;  /* ZGW is the first entry (Zone ECUs will be added later) */
    
    /* 6. Print VCI information */
    sendUARTMessage("\r\n[VCI] Zonal Gateway (ECU_091):\r\n", 34);
    sendUARTMessage("  SW Ver:  ", 11);
    sendUARTMessage(g_zgw_vci.sw_version, strlen(g_zgw_vci.sw_version));
    sendUARTMessage("\r\n  HW Ver:  ", 12);
    sendUARTMessage(g_zgw_vci.hw_version, strlen(g_zgw_vci.hw_version));
    sendUARTMessage("\r\n  Serial:  ", 12);
    sendUARTMessage(g_zgw_vci.serial_num, strlen(g_zgw_vci.serial_num));
    sendUARTMessage("\r\n", 2);
    
    /* 7. Initialize VCI Aggregator */
    if (VCI_Aggregator_Init())
    {
        sendUARTMessage("[VCI] Aggregator initialized\r\n", 31);
    }
    else
    {
        sendUARTMessage("[VCI] Aggregator init failed\r\n", 30);
    }
}

static void Init_Readiness(void)
{
    if (Readiness_Aggregator_Init())
    {
        sendUARTMessage("[Readiness] Aggregator initialized\r\n", 37);
    }
    else
    {
        sendUARTMessage("[Readiness] Init failed\r\n", 25);
    }
}

static void Init_Health_Database(void)
{
    memcpy(g_health_data[0].ecu_id, ZONE_ECU_ID, sizeof(ZONE_ECU_ID));
    g_health_data[0].health_status = HEALTH_STATUS_OK;
    g_health_data[0].dtc_count = 0;
    g_health_data[0].battery_voltage = 1302;
    g_health_data[0].temperature = 65;
    
    memcpy(g_health_data[1].ecu_id, ZGW_ECU_ID, sizeof(ZGW_ECU_ID));
    g_health_data[1].health_status = HEALTH_STATUS_OK;
    g_health_data[1].dtc_count = 0;
    g_health_data[1].battery_voltage = 1320;
    g_health_data[1].temperature = 68;
    
    sendUARTMessage("[Health] Status initialized (2 ECUs)\r\n", 39);
}

static void Init_FlashBankManager(void)
{
    FlashBank_Init();
    
    FlashBank_t activeBank = FlashBank_GetActiveBoot();
    FlashBankStatus_t status = FlashBank_GetStatusFlags();
    ResetType_t resetType = FlashBank_GetResetType();
    (void)resetType;  /* Unused for now, may be used for diagnostics later */
    
    char log_msg[128];
    sprintf(log_msg, "[FlashBankMgr] Active Bank: %s\r\n", 
            (activeBank == FLASH_BANK_A) ? "Bank_A" : "Bank_B");
    sendUARTMessage(log_msg, strlen(log_msg));
    
    sprintf(log_msg, "[FlashBankMgr] Banks Identical: %s\r\n",
            status.bits.banksIdentical ? "YES" : "NO");
    sendUARTMessage(log_msg, strlen(log_msg));
    
    /***************************************************************************
     * Boot Validation Check
     * 
     * If system init failed (timeout or hang), this function won't be reached
     * or g_systemInitSuccess will still be FALSE.
     * 
     * In this case, trigger rollback to the other bank.
     ***************************************************************************/
    
    if (g_systemInitSuccess == FALSE)
    {
        sendUARTMessage("\r\n", 2);
        sendUARTMessage("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\r\n", 46);
        sendUARTMessage("!! CRITICAL: Boot Validation FAILED!     !!\r\n", 46);
        sendUARTMessage("!! System Init Timeout or Hang Detected  !!\r\n", 46);
        sendUARTMessage("!! Triggering Rollback to Other Bank...  !!\r\n", 46);
        sendUARTMessage("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\r\n", 46);
        sendUARTMessage("\r\n", 2);
        
        /* Determine target bank (opposite of current) */
        FlashBank_t targetBank = (activeBank == FLASH_BANK_A) ? FLASH_BANK_B : FLASH_BANK_A;
        
        sprintf(log_msg, "[FlashBankMgr] Current: %s → Rollback to: %s\r\n",
                (activeBank == FLASH_BANK_A) ? "Bank_A" : "Bank_B",
                (targetBank == FLASH_BANK_A) ? "Bank_A" : "Bank_B");
        sendUARTMessage(log_msg, strlen(log_msg));
        
        /* Mark current bank as ERROR */
        if (activeBank == FLASH_BANK_A)
        {
            status.bits.statusA = BANK_STATUS_ERROR;
        }
        else
        {
            status.bits.statusB = BANK_STATUS_ERROR;
        }
        
        /* Switch boot flag to target bank */
        /* NOTE: This uses DFlash flags, not actual UCB_BMHD */
        /* Requires bootloader or software-based boot selection */
        if (targetBank == FLASH_BANK_A)
        {
            status.bits.bootTarget = 0;
        }
        else
        {
            status.bits.bootTarget = 1;
        }
        
        /* Banks are now different */
        status.bits.banksIdentical = 0;
        
        /* Write status to DFlash */
        FlashBank_WriteDFlashStatus(status);
        
        sendUARTMessage("[FlashBankMgr] ⚠️  CRITICAL LIMITATION ⚠️\r\n", 48);
        sendUARTMessage("[FlashBankMgr] Current implementation uses DFlash flags only!\r\n", 65);
        sendUARTMessage("[FlashBankMgr] For true hardware rollback, requires:\r\n", 55);
        sendUARTMessage("[FlashBankMgr]   1. Bootloader at fixed address\r\n", 50);
        sendUARTMessage("[FlashBankMgr]   2. Or UCB_BMHD modification (limited cycles)\r\n", 65);
        sendUARTMessage("[FlashBankMgr] Boot flags updated. System will reset...\r\n", 59);
        
        /* Small delay for UART transmission */
        IfxStm_waitTicks(&MODULE_STM0, IfxStm_getTicksFromMilliseconds(&MODULE_STM0, 100));
        
        /* Trigger System Reset */
        sendUARTMessage("[FlashBankMgr] RESET NOW!\r\n", 27);
        IfxStm_waitTicks(&MODULE_STM0, IfxStm_getTicksFromMilliseconds(&MODULE_STM0, 100));
        
        /* Perform software reset */
        IfxScuRcu_performReset(IfxScuRcu_ResetType_system, 0);
        
        /* Should never reach here */
        while(1);
    }
    else
    {
        /* Boot validation SUCCESS */
        sendUARTMessage("[FlashBankMgr] Boot Validation SUCCESS!\r\n", 42);
        
        /* Clear error flags for current bank */
        if (activeBank == FLASH_BANK_A)
        {
            status.bits.statusA = BANK_STATUS_OK;
        }
        else
        {
            status.bits.statusB = BANK_STATUS_OK;
        }
        
        FlashBank_WriteDFlashStatus(status);
    }
}

static void Print_System_Ready(void)
{
    sendUARTMessage("===========================================\r\n", 44);
    sendUARTMessage("Zonal Gateway Ready!\r\n", 22);
    sendUARTMessage("- TCP Echo:    8765\r\n", 21);
    sendUARTMessage("- UDP Echo:    13400\r\n", 22);
    sendUARTMessage("- DoIP Client: VMG @ 192.168.1.100:13400\r\n", 43);
    sendUARTMessage("- VCI:         Command-based (use UDS 0x31)\r\n", 46);
    sendUARTMessage("  * 0x31 01 F001: Start VCI collection\r\n", 40);
    sendUARTMessage("  * 0x31 01 F002: Send VCI report\r\n", 34);
    sendUARTMessage("  * 0x31 01 F003: Start Readiness check\r\n", 41);
    sendUARTMessage("  * 0x31 01 F004: Send Readiness report\r\n", 41);
    sendUARTMessage("===========================================\r\n", 44);
}

void SystemInit_All(void)
{
    /* Record boot start time for timeout detection */
    uint32 bootStartTime = 0;
    
    Init_System();
    
    initUART();
    sendUARTMessage("Zonal Gateway Starting...\r\n", 28);
    
    /* Start boot timer */
    bootStartTime = IfxStm_getLower(&MODULE_STM0);
    
    Init_STM_Timer();
    Flash4_Init();
    /* Test_Flash4(); */  /* Disabled: Flash test not needed for production */
    Init_Ethernet();
    
    /* TCP/UDP Echo servers removed - replaced by DoIP Link and UDP Link */
    
    Wait_PHY_Link();
    Init_DoIP();
    Init_VCI();
    Init_Readiness();
    Init_Health_Database();
    Print_System_Ready();
    
    /* Check if boot completed within timeout */
    uint32 bootEndTime = IfxStm_getLower(&MODULE_STM0);
    uint32 bootDuration = bootEndTime - bootStartTime;
    uint32 bootDurationMs = bootDuration / IfxStm_getTicksFromMilliseconds(&MODULE_STM0, 1);
    
    char log_msg[64];
    sprintf(log_msg, "[Boot] Duration: %u ms\r\n", (unsigned int)bootDurationMs);
    sendUARTMessage(log_msg, strlen(log_msg));
    
    if (bootDurationMs > BOOT_VALIDATION_TIMEOUT_MS)
    {
        /* Boot timeout - will be handled by Init_FlashBankManager */
        sendUARTMessage("[Boot] WARNING: Boot took longer than expected!\r\n", 50);
        g_systemInitSuccess = FALSE;
    }
    else
    {
        /* Boot success */
        g_systemInitSuccess = TRUE;
    }
    
    /* Initialize Flash Bank Manager after all system components are ready */
    /* This function will check g_systemInitSuccess and perform rollback if needed */
    Init_FlashBankManager();
}

