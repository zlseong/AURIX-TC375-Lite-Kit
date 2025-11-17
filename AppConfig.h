/**
 * @file AppConfig.h
 * @brief Application Configuration Parameters
 */

#ifndef APP_CONFIG_H_
#define APP_CONFIG_H_

#include "Ifx_Types.h"

/* System Configuration */
#define SYSTEM_CPU_FREQUENCY        300000000UL

/* UART Configuration */
#define UART_BAUDRATE              115200
#define UART_TX_PIN                &IfxAsclin0_TX_P14_0_OUT
#define UART_RX_PIN                &IfxAsclin0_RXA_P14_1_IN

/* Ethernet Configuration */
#define ETH_MAC_ADDR_0             0xDE
#define ETH_MAC_ADDR_1             0xAD
#define ETH_MAC_ADDR_2             0xBE
#define ETH_MAC_ADDR_3             0xEF
#define ETH_MAC_ADDR_4             0xFE
#define ETH_MAC_ADDR_5             0xED

#define ETH_IP_ADDR_0              192
#define ETH_IP_ADDR_1              168
#define ETH_IP_ADDR_2              1
#define ETH_IP_ADDR_3              10

/* Network Configuration */
#define TCP_ECHO_PORT              8765
#define UDP_DOIP_PORT              13400

#define VMG_IP_ADDR_0              192
#define VMG_IP_ADDR_1              168
#define VMG_IP_ADDR_2              1
#define VMG_IP_ADDR_3              100
#define VMG_PORT                   13400

/* VCI Configuration */
#define VCI_MAGIC                  0x56434921
#define VCI_COLLECTION_TIMEOUT_MS  10000

/* Zonal Gateway Identity */
#define ZGW_ECU_ID                 "ECU_091"
#define ZGW_SW_VERSION             "v0.0.0"       /* Initial Development Version */
#define ZGW_HW_VERSION             "HW_1.0.0"     /* Hardware Version (fixed) */
#define ZGW_SERIAL_NUM             "ZGW-091-001"  /* Serial Number (unique per device) */

/* Zone ECU Identity */
#define ZONE_ECU_ID                "ECU_011"

/* Timer Configuration */
#define STM_TIMER_INTERVAL_MS      10

/* PHY Link Configuration */
#define PHY_LINK_TIMEOUT_MS        5000

/* Return Values */
typedef enum {
    E_OK = 0,
    E_NOT_OK = 1,
    E_TIMEOUT = 2,
    E_BUSY = 3,
    E_NO_MEMORY = 4
} Std_ReturnType;

#endif /* APP_CONFIG_H_ */

