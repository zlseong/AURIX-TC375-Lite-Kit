/**
 * @file vci_aggregator.c
 * @brief VCI Aggregator Implementation - Uses UDS Client to collect VCI from Zone ECUs
 */

#include "vci_aggregator.h"
#include "Libraries/DoIP/uds_handler.h"
#include "UART_Logging.h"
#include "IfxStm.h"
#include <string.h>
#include <stdio.h>

/*******************************************************************************
 * External References
 ******************************************************************************/

extern DoIP_VCI_Info g_vci_database[MAX_ZONE_ECUS + 1];
extern uint8 g_zone_ecu_count;
extern boolean g_vci_collection_complete;

/*******************************************************************************
 * Zone ECU Configuration (Simulated)
 ******************************************************************************/

static const char *g_zone_ecu_ips[] = {
    "192.168.1.101",  /* BCM */
    "192.168.1.102",  /* ACU */
    "192.168.1.103",  /* PEPS */
};

#define ZONE_ECU_COUNT (sizeof(g_zone_ecu_ips) / sizeof(g_zone_ecu_ips[0]))

/*******************************************************************************
 * Private Variables
 ******************************************************************************/

static boolean g_collection_active = FALSE;
static uint32 g_collection_start_time = 0;
static uint8 g_collected_count = 0;
static uint8 g_target_ecu_count = 0;

/*******************************************************************************
 * Helper Functions
 ******************************************************************************/

static uint32 GetTimestamp(void)
{
    return IfxStm_getLower(&MODULE_STM0);
}

static uint32 GetElapsedMs(uint32 start_time)
{
    uint32 current = GetTimestamp();
    uint32 elapsed_ticks = current - start_time;
    uint32 ticks_per_ms = (uint32)IfxStm_getTicksFromMilliseconds(&MODULE_STM0, 1);
    return elapsed_ticks / ticks_per_ms;
}

/*******************************************************************************
 * UDS Client Response Callback
 ******************************************************************************/

static void VCI_ResponseCallback(const char *ecu_ip, uint8 *response_data, uint16 response_len)
{
    char msg[128];
    
    sprintf(msg, "[VCI Agg] RX from %s: %u bytes\r\n", ecu_ip, (unsigned int)response_len);
    sendUARTMessage(msg, strlen(msg));
    
    /* Parse UDS response: [SID+0x40][DID_H][DID_L][VCI_Data...] */
    if (response_len < 3 + sizeof(DoIP_VCI_Info))
    {
        sendUARTMessage("[VCI Agg] Response too short\r\n", 31);
        return;
    }
    
    /* Check if positive response (0x62 = 0x22 + 0x40) */
    if (response_data[0] != 0x62)
    {
        sprintf(msg, "[VCI Agg] Negative response: NRC=0x%02X\r\n", response_data[2]);
        sendUARTMessage(msg, strlen(msg));
        return;
    }
    
    /* Extract VCI data (skip SID + DID) */
    DoIP_VCI_Info *vci = (DoIP_VCI_Info*)&response_data[3];
    
    /* Store in database */
    if (g_zone_ecu_count < MAX_ZONE_ECUS)
    {
        memcpy(&g_vci_database[g_zone_ecu_count], vci, sizeof(DoIP_VCI_Info));
        g_zone_ecu_count++;
        g_collected_count++;
        
        sprintf(msg, "[VCI Agg] Stored VCI: %s v%s [%u/%u]\r\n",
                vci->ecu_id, vci->sw_version,
                (unsigned int)g_collected_count, (unsigned int)g_target_ecu_count);
        sendUARTMessage(msg, strlen(msg));
    }
    
    /* Check if collection is complete */
    if (g_collected_count >= g_target_ecu_count)
    {
        g_vci_collection_complete = TRUE;
        g_collection_active = FALSE;
        sendUARTMessage("[VCI Agg] Collection complete!\r\n", 33);
    }
}

/*******************************************************************************
 * Public Functions
 ******************************************************************************/

boolean VCI_Aggregator_Init(void)
{
    g_collection_active = FALSE;
    g_collection_start_time = 0;
    g_collected_count = 0;
    g_target_ecu_count = 0;
    
    sendUARTMessage("[VCI Agg] Initialized\r\n", 24);
    return TRUE;
}

boolean VCI_Aggregator_Start(void)
{
    char msg[64];
    uint8 i;
    uint8 success_count;
    
    if (g_collection_active)
    {
        sendUARTMessage("[VCI Agg] Already active\r\n", 27);
        return FALSE;
    }
    
    /* Reset state */
    g_collected_count = 0;
    g_target_ecu_count = (uint8)ZONE_ECU_COUNT;
    g_collection_start_time = GetTimestamp();
    g_collection_active = TRUE;
    g_vci_collection_complete = FALSE;
    
    sprintf(msg, "[VCI Agg] Starting collection from %u ECUs\r\n", (unsigned int)g_target_ecu_count);
    sendUARTMessage(msg, strlen(msg));
    
    /* Send UDS 0x22 ReadDataByID (DID=0xF194) to all Zone ECUs */
    success_count = 0;
    for (i = 0; i < ZONE_ECU_COUNT; i++)
    {
        if (UDS_Client_ReadVCI(g_zone_ecu_ips[i], UDS_DID_VCI_ECU_ID, VCI_ResponseCallback))
        {
            success_count++;
            sprintf(msg, "[VCI Agg] Sent request to %s\r\n", g_zone_ecu_ips[i]);
            sendUARTMessage(msg, strlen(msg));
        }
        else
        {
            sprintf(msg, "[VCI Agg] Failed to send to %s\r\n", g_zone_ecu_ips[i]);
            sendUARTMessage(msg, strlen(msg));
        }
    }
    
    if (success_count == 0)
    {
        g_collection_active = FALSE;
        sendUARTMessage("[VCI Agg] No requests sent\r\n", 29);
        return FALSE;
    }
    
    return TRUE;
}

void VCI_Aggregator_Poll(void)
{
    uint32 elapsed;
    
    if (!g_collection_active)
    {
        return;
    }
    
    /* Check timeout */
    elapsed = GetElapsedMs(g_collection_start_time);
    if (elapsed > VCI_COLLECTION_TIMEOUT)
    {
        char msg[64];
        
        g_collection_active = FALSE;
        g_vci_collection_complete = TRUE;  /* Mark as complete even if partial */
        
        sprintf(msg, "[VCI Agg] Timeout: %u/%u ECUs\r\n",
                (unsigned int)g_collected_count, (unsigned int)g_target_ecu_count);
        sendUARTMessage(msg, strlen(msg));
    }
}

uint8 VCI_Aggregator_GetCount(void)
{
    return g_collected_count;
}

boolean VCI_Aggregator_IsComplete(void)
{
    return g_vci_collection_complete;
}

void VCI_Aggregator_Clear(void)
{
    g_collected_count = 0;
    g_target_ecu_count = 0;
    g_vci_collection_complete = FALSE;
    g_collection_active = FALSE;
    sendUARTMessage("[VCI Agg] Cleared\r\n", 20);
}

boolean VCI_Aggregator_IsActive(void)
{
    return g_collection_active;
}

