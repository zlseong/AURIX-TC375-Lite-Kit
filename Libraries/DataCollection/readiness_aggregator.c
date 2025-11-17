/**
 * @file readiness_aggregator.c
 * @brief Readiness Aggregator Implementation - Uses UDS Client to collect readiness from Zone ECUs
 */

#include "readiness_aggregator.h"
#include "Libraries/DoIP/uds_handler.h"
#include "UART_Logging.h"
#include "IfxStm.h"
#include <string.h>
#include <stdio.h>

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

static Readiness_Info g_readiness_database[MAX_ZONE_ECUS];
static uint8 g_readiness_count = 0;
static boolean g_collection_active = FALSE;
static uint32 g_collection_start_time = 0;
static uint8 g_target_ecu_count = 0;

/*******************************************************************************
 * Helper Functions
 ******************************************************************************/

static uint32 GetTimestamp(void)
{
    return IfxStm_getLower(&MODULE_STM0);
}

/*******************************************************************************
 * UDS Client Response Callback
 ******************************************************************************/

static void Readiness_ResponseCallback(const char *ecu_ip, uint8 *response_data, uint16 response_len)
{
    char msg[128];
    
    sprintf(msg, "[Readiness Agg] RX from %s: %u bytes\r\n", ecu_ip, (unsigned int)response_len);
    sendUARTMessage(msg, strlen(msg));
    
    /* Parse UDS response: [SID+0x40][Sub][RID_H][RID_L][Status][Readiness_Data...] */
    if (response_len < 5)
    {
        sendUARTMessage("[Readiness Agg] Response too short\r\n", 37);
        return;
    }
    
    /* Check if positive response (0x71 = 0x31 + 0x40) */
    if (response_data[0] != 0x71)
    {
        sprintf(msg, "[Readiness Agg] Negative response: NRC=0x%02X\r\n", response_data[2]);
        sendUARTMessage(msg, strlen(msg));
        return;
    }
    
    /* Check status byte */
    if (response_data[4] != 0x00)
    {
        sprintf(msg, "[Readiness Agg] Routine failed: status=0x%02X\r\n", response_data[4]);
        sendUARTMessage(msg, strlen(msg));
        return;
    }
    
    /* Extract readiness data (skip SID + Sub + RID + Status) */
    if (response_len >= 5 + sizeof(Readiness_Info))
    {
        Readiness_Info *info = &g_readiness_database[g_readiness_count];
        uint8 *payload = &response_data[5];
        
        memcpy(info->ecu_id, payload, 16);
        info->battery_soc = payload[16];
        info->temperature = payload[17];
        info->engine_state = payload[18];
        info->parking_brake = payload[19];
        info->free_space_kb = (payload[20] << 24) | (payload[21] << 16) |
                              (payload[22] << 8) | payload[23];
        
        g_readiness_count++;
        
        sprintf(msg, "[Readiness Agg] Stored: %s [%u/%u]\r\n",
                info->ecu_id, (unsigned int)g_readiness_count, (unsigned int)g_target_ecu_count);
        sendUARTMessage(msg, strlen(msg));
    }
    
    /* Check if collection is complete */
    if (g_readiness_count >= g_target_ecu_count)
    {
        g_collection_active = FALSE;
        sendUARTMessage("[Readiness Agg] Collection complete!\r\n", 39);
    }
}

/*******************************************************************************
 * Public Functions
 ******************************************************************************/

boolean Readiness_Aggregator_Init(void)
{
    g_collection_active = FALSE;
    g_collection_start_time = 0;
    g_readiness_count = 0;
    g_target_ecu_count = 0;
    
    sendUARTMessage("[Readiness Agg] Initialized\r\n", 30);
    return TRUE;
}

boolean Readiness_Aggregator_Start(void)
{
    char msg[64];
    uint8 i;
    uint8 success_count;
    
    if (g_collection_active)
    {
        sendUARTMessage("[Readiness Agg] Already active\r\n", 33);
        return FALSE;
    }
    
    /* Reset state */
    g_readiness_count = 0;
    g_target_ecu_count = (uint8)ZONE_ECU_COUNT;
    g_collection_start_time = GetTimestamp();
    g_collection_active = TRUE;
    
    sprintf(msg, "[Readiness Agg] Starting from %u ECUs\r\n", (unsigned int)g_target_ecu_count);
    sendUARTMessage(msg, strlen(msg));
    
    /* Send UDS 0x31 RoutineControl (RID=0xF003) to all Zone ECUs */
    success_count = 0;
    for (i = 0; i < ZONE_ECU_COUNT; i++)
    {
        if (UDS_Client_CheckReadiness(g_zone_ecu_ips[i], UDS_RID_READINESS_CHECK, Readiness_ResponseCallback))
        {
            success_count++;
            sprintf(msg, "[Readiness Agg] Sent to %s\r\n", g_zone_ecu_ips[i]);
            sendUARTMessage(msg, strlen(msg));
        }
        else
        {
            sprintf(msg, "[Readiness Agg] Failed to %s\r\n", g_zone_ecu_ips[i]);
            sendUARTMessage(msg, strlen(msg));
        }
    }
    
    if (success_count == 0)
    {
        g_collection_active = FALSE;
        sendUARTMessage("[Readiness Agg] No requests sent\r\n", 35);
        return FALSE;
    }
    
    return TRUE;
}

uint8 Readiness_Aggregator_GetResults(Readiness_Info *info_array, uint8 max_count)
{
    uint8 copy_count;
    
    if (info_array == NULL)
    {
        return 0;
    }
    
    copy_count = (g_readiness_count < max_count) ? g_readiness_count : max_count;
    memcpy(info_array, g_readiness_database, copy_count * sizeof(Readiness_Info));
    
    return copy_count;
}

void Readiness_Aggregator_Clear(void)
{
    g_readiness_count = 0;
    g_target_ecu_count = 0;
    g_collection_active = FALSE;
    sendUARTMessage("[Readiness Agg] Cleared\r\n", 26);
}

boolean Readiness_Aggregator_IsActive(void)
{
    return g_collection_active;
}

