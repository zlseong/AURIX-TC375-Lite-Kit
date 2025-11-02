/**********************************************************************************************************************
 * \file Cpu0_Main.c
 * \copyright Copyright (C) Infineon Technologies AG 2019
 * 
 * Zonal Gateway - Ethernet + lwIP Integration
 * Target: TC375 Lite Kit
 * IP: 192.168.1.10 (Static)
 *********************************************************************************************************************/
#include "Ifx_Types.h"
#include "IfxCpu.h"
#include "IfxScuWdt.h"
#include "IfxStm.h"
#include "IfxGeth_Eth.h"
#include "Ifx_Lwip.h"
#include "Configuration.h"
#include "ConfigurationIsr.h"
#include "UART_Logging.h"

IFX_ALIGN(4) IfxCpu_syncEvent g_cpuSyncEvent = 0;

/* lwIP Timer ISR (1ms period) */
IFX_INTERRUPT(updateLwIPStackISR, 0, ISR_PRIORITY_OS_TICK);
void updateLwIPStackISR(void)
{
    /* Configure STM to generate next interrupt in 1ms */
    IfxStm_increaseCompare(&MODULE_STM0, IfxStm_Comparator_0, IFX_CFG_STM_TICKS_PER_MS);
    
    /* Increase lwIP system time */
    g_TickCount_1ms++;
    
    /* Update lwIP timers for all enabled protocols (ARP, TCP, DHCP, LINK) */
    Ifx_Lwip_onTimerTick();
}

void core0_main(void)
{
    /* Enable global interrupts */
    IfxCpu_enableInterrupts();
    
    /* !!WATCHDOG0 AND SAFETY WATCHDOG ARE DISABLED HERE!!
     * Enable the watchdogs and service them periodically if it is required
     */
    IfxScuWdt_disableCpuWatchdog(IfxScuWdt_getCpuWatchdogPassword());
    IfxScuWdt_disableSafetyWatchdog(IfxScuWdt_getSafetyWatchdogPassword());
    
    /* Wait for CPU sync event */
    IfxCpu_emitEvent(&g_cpuSyncEvent);
    IfxCpu_waitEvent(&g_cpuSyncEvent, 1);
    
    /* ============================================
     * UART Initialization (Debug Logging)
     * ============================================ */
    initUART();
    sendUARTMessage("Zonal Gateway Starting...\r\n", 28);
    
    /* ============================================
     * STM Timer Initialization (for lwIP timers)
     * ============================================ */
    IfxStm_CompareConfig stmCompareConfig;
    IfxStm_initCompareConfig(&stmCompareConfig);
    stmCompareConfig.triggerPriority = ISR_PRIORITY_OS_TICK;
    stmCompareConfig.comparatorInterrupt = IfxStm_ComparatorInterrupt_ir0;
    stmCompareConfig.ticks = IFX_CFG_STM_TICKS_PER_MS * 10;  /* First interrupt after 10ms */
    stmCompareConfig.typeOfService = IfxSrc_Tos_cpu0;
    IfxStm_initCompare(&MODULE_STM0, &stmCompareConfig);
    
    sendUARTMessage("STM Timer OK\r\n", 14);
    
    /* ============================================
     * GETH Module Initialization
     * ============================================ */
    IfxGeth_enableModule(&MODULE_GETH);
    sendUARTMessage("GETH Module Enabled\r\n", 21);
    
    /* ============================================
     * lwIP Stack Initialization
     * ============================================ */
    /* Define MAC Address */
    eth_addr_t ethAddr;
    ethAddr.addr[0] = 0xDE;
    ethAddr.addr[1] = 0xAD;
    ethAddr.addr[2] = 0xBE;
    ethAddr.addr[3] = 0xEF;
    ethAddr.addr[4] = 0xFE;
    ethAddr.addr[5] = 0xED;
    
    /* Initialize lwIP with MAC address and Static IP */
    Ifx_Lwip_init(ethAddr);
    sendUARTMessage("lwIP Init OK - IP: 192.168.1.10\r\n", 36);
    sendUARTMessage("Ready for Ping Test!\r\n", 22);
    
    /* ============================================
     * Main Loop
     * ============================================ */
    while (1)
    {
        /* Poll lwIP timers and trigger protocol execution if required */
        Ifx_Lwip_pollTimerFlags();
        
        /* Receive data packets through ETH */
        Ifx_Lwip_pollReceiveFlags();
    }
}
