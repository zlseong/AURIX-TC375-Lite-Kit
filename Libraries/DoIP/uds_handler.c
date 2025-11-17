/*******************************************************************************
 * @file    uds_handler.c
 * @brief   UDS (Unified Diagnostic Services) Handler Implementation
 * @details Implements ISO 14229-1 UDS services over DoIP (ISO 13400)
 * 
 * @version 1.0
 * @date    2025-11-04
 ******************************************************************************/

#include "uds_handler.h"
#include "doip_types.h"
#include "doip_client.h"
#include "doip_link.h"
#include "Libraries/DataCollection/vci_aggregator.h"
#include "Libraries/DataCollection/readiness_aggregator.h"
#include "Libraries/OTA/ota_manager.h"
#include "UART_Logging.h"
#include <string.h>
#include <stdio.h>

/*******************************************************************************
 * External VCI Database (from Cpu0_Main.c)
 ******************************************************************************/

/* These will be defined in Cpu0_Main.c and accessed here */
extern DoIP_VCI_Info g_vci_database[MAX_ZONE_ECUS + 1];  /* +1 for ZGW itself */
extern uint8 g_zone_ecu_count;
extern boolean g_vci_collection_complete;

/* ZGW own VCI */
extern DoIP_VCI_Info g_zgw_vci;

/* Health Status Database */
extern DoIP_HealthStatus_Info g_health_data[MAX_ZONE_ECUS + 1];

/*******************************************************************************
 * Private Variables
 ******************************************************************************/

/* UDS Service Handler Table */
static const struct {
    uint8 service_id;
    UDS_ServiceHandler handler;
} g_service_handlers[] = {
    { UDS_SID_READ_DATA_BY_IDENTIFIER, UDS_Service_ReadDataByIdentifier },
    { UDS_SID_ROUTINE_CONTROL, UDS_Service_RoutineControl },
    { UDS_SID_REQUEST_DOWNLOAD, UDS_Service_RequestDownload },
    { UDS_SID_TRANSFER_DATA, UDS_Service_TransferData },
    { UDS_SID_REQUEST_TRANSFER_EXIT, UDS_Service_RequestTransferExit },
    /* Add more service handlers here as needed */
};

#define SERVICE_HANDLER_COUNT (sizeof(g_service_handlers) / sizeof(g_service_handlers[0]))

/*******************************************************************************
 * Public Functions
 ******************************************************************************/

void UDS_Init(void)
{
    /* Initialize UDS handler */
    /* Currently no initialization needed */
}

boolean UDS_HandleRequest(const UDS_Request *request, UDS_Response *response)
{
    if (request == NULL || response == NULL)
    {
        return FALSE;
    }
    
    /* Initialize response */
    memset(response, 0, sizeof(UDS_Response));
    response->source_address = request->target_address;  /* Swap addresses */
    response->target_address = request->source_address;
    
    /* Find service handler */
    for (uint8 i = 0; i < SERVICE_HANDLER_COUNT; i++)
    {
        if (g_service_handlers[i].service_id == request->service_id)
        {
            /* Call service handler */
            return g_service_handlers[i].handler(request, response);
        }
    }
    
    /* Service not supported */
    UDS_CreateNegativeResponse(request, UDS_NRC_SERVICE_NOT_SUPPORTED, response);
    return TRUE;
}

boolean UDS_ParseDoIPDiagnostic(const uint8 *doip_payload, uint32 payload_len, UDS_Request *request)
{
    if (doip_payload == NULL || request == NULL || payload_len < 5)
    {
        return FALSE;  /* Minimum: 4 bytes routing + 1 byte service ID */
    }
    
    /* Parse DoIP Routing */
    request->source_address = ((uint16)doip_payload[0] << 8) | doip_payload[1];
    request->target_address = ((uint16)doip_payload[2] << 8) | doip_payload[3];
    
    /* Parse UDS Message */
    request->service_id = doip_payload[4];
    request->data_len = (uint16)(payload_len - 5);
    
    if (request->data_len > 0 && request->data_len < UDS_MAX_REQUEST_SIZE)
    {
        memcpy(request->data, &doip_payload[5], request->data_len);
    }
    
    /* Debug: Log received UDS request */
    char log_msg[128];
    sprintf(log_msg, "[UDS] RX: SID=0x%02X, SA=0x%04X, TA=0x%04X, Len=%u\r\n",
            request->service_id, request->source_address, request->target_address, (unsigned int)request->data_len);
    sendUARTMessage(log_msg, strlen(log_msg));
    
    /* Hex dump of UDS data */
    if (request->data_len > 0 && request->data_len <= 8)
    {
        sprintf(log_msg, "[UDS] Data: ");
        uint16 len = strlen(log_msg);
        for (uint16 i = 0; i < request->data_len; i++)
        {
            sprintf(log_msg + len, "%02X ", request->data[i]);
            len += 3;
        }
        sprintf(log_msg + len, "\r\n");
        sendUARTMessage(log_msg, strlen(log_msg));
    }
    
    return TRUE;
}

uint16 UDS_BuildDoIPDiagnostic(const UDS_Response *response, uint8 *buffer, uint16 buffer_size)
{
    if (response == NULL || buffer == NULL)
    {
        return 0;
    }
    
    /* Calculate required size */
    uint16 payload_len = 4 + 1 + response->data_len;  /* Routing + SID + data */
    uint16 total_len = DOIP_HEADER_SIZE + payload_len;
    
    if (total_len > buffer_size)
    {
        return 0;  /* Buffer too small */
    }
    
    uint16 offset = 0;
    
    /* DoIP Header (8 bytes) */
    buffer[offset++] = DOIP_PROTOCOL_VERSION;
    buffer[offset++] = DOIP_INVERSE_VERSION;
    buffer[offset++] = (DOIP_DIAGNOSTIC_MESSAGE >> 8) & 0xFF;
    buffer[offset++] = DOIP_DIAGNOSTIC_MESSAGE & 0xFF;
    
    /* Payload Length (4 bytes) */
    buffer[offset++] = (payload_len >> 24) & 0xFF;
    buffer[offset++] = (payload_len >> 16) & 0xFF;
    buffer[offset++] = (payload_len >> 8) & 0xFF;
    buffer[offset++] = payload_len & 0xFF;
    
    /* DoIP Routing (4 bytes) */
    buffer[offset++] = (response->source_address >> 8) & 0xFF;
    buffer[offset++] = response->source_address & 0xFF;
    buffer[offset++] = (response->target_address >> 8) & 0xFF;
    buffer[offset++] = response->target_address & 0xFF;
    
    /* UDS Response */
    buffer[offset++] = response->service_id;
    
    /* Response Data */
    if (response->data_len > 0)
    {
        memcpy(&buffer[offset], response->data, response->data_len);
        offset += response->data_len;
    }
    
    /* Debug: Log sent UDS response */
    char log_msg[128];
    sprintf(log_msg, "[UDS] TX: SID=0x%02X, SA=0x%04X, TA=0x%04X, Total=%u bytes\r\n",
            response->service_id, response->source_address, response->target_address, (unsigned int)total_len);
    sendUARTMessage(log_msg, strlen(log_msg));
    
    /* Hex dump of first 16 bytes */
    if (total_len > 0)
    {
        uint16 dump_len = (total_len > 16) ? 16 : total_len;
        sprintf(log_msg, "[UDS] TX Data: ");
        uint16 len = strlen(log_msg);
        for (uint16 i = 0; i < dump_len; i++)
        {
            sprintf(log_msg + len, "%02X ", buffer[i]);
            len += 3;
            if (len >= 110)  /* Prevent buffer overflow */
                break;
        }
        if (total_len > 16)
        {
            sprintf(log_msg + len, "...");
            len += 3;
        }
        sprintf(log_msg + len, "\r\n");
        sendUARTMessage(log_msg, strlen(log_msg));
    }
    
    return total_len;
}

/*******************************************************************************
 * UDS Service Handlers
 ******************************************************************************/

boolean UDS_Service_ReadDataByIdentifier(const UDS_Request *request, UDS_Response *response)
{
    /* 0x22 Read Data By Identifier requires at least 2 bytes (DID) */
    if (request->data_len < 2)
    {
        UDS_CreateNegativeResponse(request, UDS_NRC_INCORRECT_MESSAGE_LENGTH, response);
        return TRUE;
    }
    
    /* Parse DID */
    uint16 did = ((uint16)request->data[0] << 8) | request->data[1];
    
    /* Prepare positive response */
    UDS_CreatePositiveResponse(request, response);
    
    /* Echo DID in response */
    response->data[0] = request->data[0];
    response->data[1] = request->data[1];
    response->data_len = 2;
    
    /* Handle DID */
    uint16 did_data_len = 0;
    if (UDS_ReadDID_VCI(did, &response->data[2], &did_data_len))
    {
        response->data_len += did_data_len;
        return TRUE;
    }
    
    /* DID not supported */
    UDS_CreateNegativeResponse(request, UDS_NRC_REQUEST_OUT_OF_RANGE, response);
    return TRUE;
}

boolean UDS_ReadDID_VCI(uint16 did, uint8 *data, uint16 *data_len)
{
    if (data == NULL || data_len == NULL)
    {
        return FALSE;
    }
    
    switch (did)
    {
        case UDS_DID_VCI_ECU_ID:  /* 0xF194 - Individual ECU VCI */
        {
            /* Return ZGW's own VCI */
            memcpy(data, &g_zgw_vci, sizeof(DoIP_VCI_Info));
            *data_len = sizeof(DoIP_VCI_Info);
            return TRUE;
        }
        
        case UDS_DID_VCI_CONSOLIDATED:  /* 0xF195 - Consolidated VCI */
        {
            /* This triggers VCI collection from Zone ECUs */
            DoIP_VCI_Info vci_array[MAX_ZONE_ECUS + 1];
            uint8 vci_count = 0;
            
            if (UDS_ReadConsolidatedVCI(vci_array, &vci_count))
            {
                /* Build response: [Count][VCI_1][VCI_2]... */
                data[0] = vci_count;
                memcpy(&data[1], vci_array, vci_count * sizeof(DoIP_VCI_Info));
                *data_len = 1 + (vci_count * sizeof(DoIP_VCI_Info));
                return TRUE;
            }
            return FALSE;
        }
        
        case UDS_DID_HEALTH_STATUS:  /* 0xF1A0 - Health Status */
        {
            DoIP_HealthStatus_Info health_array[MAX_ZONE_ECUS + 1];
            uint8 health_count = 0;
            
            if (UDS_ReadHealthStatus(health_array, &health_count))
            {
                /* Build response: [Count][Health_1][Health_2]... */
                data[0] = health_count;
                memcpy(&data[1], health_array, health_count * sizeof(DoIP_HealthStatus_Info));
                *data_len = 1 + (health_count * sizeof(DoIP_HealthStatus_Info));
                return TRUE;
            }
            return FALSE;
        }
        
        default:
            return FALSE;  /* DID not supported */
    }
}

boolean UDS_ReadIndividualVCI(uint16 ecu_address, DoIP_VCI_Info *vci_info)
{
    /* TODO: Implement ECU-specific VCI request via DoIP */
    /* For now, return ZGW's own VCI */
    if (ecu_address == ZGW_ADDRESS)
    {
        memcpy(vci_info, &g_zgw_vci, sizeof(DoIP_VCI_Info));
        return TRUE;
    }
    
    return FALSE;
}

boolean UDS_ReadConsolidatedVCI(DoIP_VCI_Info *vci_array, uint8 *vci_count)
{
    if (vci_array == NULL || vci_count == NULL)
    {
        return FALSE;
    }
    
    /* Check if VCI collection is complete */
    if (!g_vci_collection_complete)
    {
        /* VCI collection not ready - return only ZGW VCI */
        memcpy(&vci_array[0], &g_zgw_vci, sizeof(DoIP_VCI_Info));
        *vci_count = 1;
        return TRUE;
    }
    
    /* Return all collected VCI (Zone ECUs + ZGW) */
    uint8 total_count = g_zone_ecu_count + 1;  /* +1 for ZGW itself */
    
    if (total_count > MAX_ZONE_ECUS + 1)
    {
        total_count = MAX_ZONE_ECUS + 1;
    }
    
    memcpy(vci_array, g_vci_database, total_count * sizeof(DoIP_VCI_Info));
    *vci_count = total_count;
    
    return TRUE;
}

boolean UDS_ReadHealthStatus(DoIP_HealthStatus_Info *health_array, uint8 *health_count)
{
    if (health_array == NULL || health_count == NULL)
    {
        return FALSE;
    }
    
    /* Return health status for all ECUs (simulated for now) */
    uint8 total_count = g_zone_ecu_count + 1;  /* +1 for ZGW itself */
    
    if (total_count > MAX_ZONE_ECUS + 1)
    {
        total_count = MAX_ZONE_ECUS + 1;
    }
    
    memcpy(health_array, g_health_data, total_count * sizeof(DoIP_HealthStatus_Info));
    *health_count = total_count;
    
    return TRUE;
}

/*******************************************************************************
 * Helper Functions
 ******************************************************************************/

void UDS_CreateNegativeResponse(const UDS_Request *request, uint8 nrc, UDS_Response *response)
{
    response->is_positive = FALSE;
    response->service_id = UDS_SID_NEGATIVE_RESPONSE;
    response->nrc = nrc;
    response->data_len = 2;
    response->data[0] = request->service_id;  /* Rejected Service ID */
    response->data[1] = nrc;                  /* Negative Response Code */
}

void UDS_CreatePositiveResponse(const UDS_Request *request, UDS_Response *response)
{
    response->is_positive = TRUE;
    response->service_id = request->service_id + UDS_POSITIVE_RESPONSE_OFFSET;
    response->nrc = 0;
    response->data_len = 0;
}

/*******************************************************************************
 * UDS CLIENT - ZGW sends requests to Zone ECUs
 ******************************************************************************/

/* Client state management */
typedef struct
{
    DoIPLink link;
    UDS_Client_ResponseCallback callback;
    char ecu_ip[16];
    boolean active;
} UDS_ClientContext;

#define MAX_UDS_CLIENT_CONTEXTS 8
static UDS_ClientContext g_client_contexts[MAX_UDS_CLIENT_CONTEXTS];

/* DoIP Callback for receiving responses */
static void UDS_Client_DoIPCallback(DoIPLink *link, uint8 *data, uint16 len)
{
    char msg[64];
    UDS_ClientContext *ctx;
    uint16 i;
    
    /* Find context */
    ctx = NULL;
    for (i = 0; i < MAX_UDS_CLIENT_CONTEXTS; i++)
    {
        if (&g_client_contexts[i].link == link && g_client_contexts[i].active)
        {
            ctx = &g_client_contexts[i];
            break;
        }
    }
    
    if (ctx == NULL)
    {
        sendUARTMessage("[UDS Client] Context not found\r\n", 33);
        return;
    }
    
    sprintf(msg, "[UDS Client] RX from %s: %u bytes\r\n", ctx->ecu_ip, (unsigned int)len);
    sendUARTMessage(msg, strlen(msg));
    
    /* Parse DoIP header to extract UDS payload */
    if (len > 12)  /* DoIP header(8) + routing(4) */
    {
        uint8 *uds_payload = data + 12;  /* Skip DoIP header + routing */
        uint16 uds_len = len - 12;
        
        /* Call user callback */
        if (ctx->callback != NULL)
        {
            ctx->callback(ctx->ecu_ip, uds_payload, uds_len);
        }
    }
    
    /* Close link and free context */
    DoIP_Link_Close(&ctx->link);
    ctx->active = FALSE;
}

boolean UDS_Client_SendRequest(const char *ecu_ip, uint8 *uds_request, uint16 request_len, UDS_Client_ResponseCallback callback)
{
    UDS_ClientContext *ctx;
    uint16 i;
    char msg[64];
    
    if (ecu_ip == NULL || uds_request == NULL || request_len == 0)
    {
        return FALSE;
    }
    
    /* Find free context */
    ctx = NULL;
    for (i = 0; i < MAX_UDS_CLIENT_CONTEXTS; i++)
    {
        if (!g_client_contexts[i].active)
        {
            ctx = &g_client_contexts[i];
            break;
        }
    }
    
    if (ctx == NULL)
    {
        sendUARTMessage("[UDS Client] No free context\r\n", 31);
        return FALSE;
    }
    
    /* Initialize context */
    ctx->active = TRUE;
    ctx->callback = callback;
    strncpy(ctx->ecu_ip, ecu_ip, sizeof(ctx->ecu_ip) - 1);
    ctx->ecu_ip[sizeof(ctx->ecu_ip) - 1] = '\0';
    
    /* Initialize DoIP link as client */
    if (!DoIP_Link_Init(&ctx->link, DOIP_ROLE_CLIENT, 13400, ZGW_ADDRESS))
    {
        ctx->active = FALSE;
        sendUARTMessage("[UDS Client] Link init failed\r\n", 32);
        return FALSE;
    }
    
    /* Set remote ECU */
    DoIP_Link_SetRemote(&ctx->link, ecu_ip, 13400);
    
    /* Set callback */
    DoIP_Link_SetCallbacks(&ctx->link, UDS_Client_DoIPCallback, NULL, NULL);
    
    /* Start connection */
    if (!DoIP_Link_Start(&ctx->link))
    {
        ctx->active = FALSE;
        sendUARTMessage("[UDS Client] Link start failed\r\n", 33);
        return FALSE;
    }
    
    /* Build DoIP diagnostic message */
    uint8 doip_buffer[256];
    uint16 doip_offset;
    uint16 payload_len;
    
    doip_offset = 0;
    
    /* DoIP routing (4 bytes) */
    doip_buffer[doip_offset++] = (ZGW_ADDRESS >> 8) & 0xFF;  /* Source: ZGW */
    doip_buffer[doip_offset++] = ZGW_ADDRESS & 0xFF;
    doip_buffer[doip_offset++] = 0x00;  /* Target: will be set dynamically */
    doip_buffer[doip_offset++] = 0x01;
    
    /* UDS payload */
    memcpy(&doip_buffer[doip_offset], uds_request, request_len);
    doip_offset += request_len;
    
    payload_len = doip_offset;
    
    /* Send via DoIP */
    if (!DoIP_Link_Send(&ctx->link, doip_buffer, payload_len))
    {
        DoIP_Link_Close(&ctx->link);
        ctx->active = FALSE;
        sendUARTMessage("[UDS Client] Send failed\r\n", 27);
        return FALSE;
    }
    
    sprintf(msg, "[UDS Client] Sent to %s: SID=0x%02X\r\n", ecu_ip, uds_request[0]);
    sendUARTMessage(msg, strlen(msg));
    
    return TRUE;
}

boolean UDS_Client_ReadVCI(const char *ecu_ip, uint16 did, UDS_Client_ResponseCallback callback)
{
    uint8 uds_req[3];
    
    /* Build 0x22 ReadDataByID request */
    uds_req[0] = UDS_SID_READ_DATA_BY_IDENTIFIER;  /* 0x22 */
    uds_req[1] = (did >> 8) & 0xFF;                /* DID high byte */
    uds_req[2] = did & 0xFF;                       /* DID low byte */
    
    return UDS_Client_SendRequest(ecu_ip, uds_req, 3, callback);
}

boolean UDS_Client_CheckReadiness(const char *ecu_ip, uint16 routine_id, UDS_Client_ResponseCallback callback)
{
    uint8 uds_req[4];
    
    /* Build 0x31 RoutineControl request */
    uds_req[0] = UDS_SID_ROUTINE_CONTROL;          /* 0x31 */
    uds_req[1] = UDS_RC_START_ROUTINE;             /* 0x01 - Start */
    uds_req[2] = (routine_id >> 8) & 0xFF;         /* RID high byte */
    uds_req[3] = routine_id & 0xFF;                /* RID low byte */
    
    return UDS_Client_SendRequest(ecu_ip, uds_req, 4, callback);
}

/*******************************************************************************
 * UDS Service: 0x31 Routine Control
 ******************************************************************************/

boolean UDS_Service_RoutineControl(const UDS_Request *request, UDS_Response *response)
{
    /* 0x31 Routine Control requires at least 3 bytes: [sub-function][RID_high][RID_low] */
    if (request->data_len < 3)
    {
        UDS_CreateNegativeResponse(request, UDS_NRC_INCORRECT_MESSAGE_LENGTH, response);
        return TRUE;
    }
    
    uint8 sub_function = request->data[0];
    uint16 routine_id = ((uint16)request->data[1] << 8) | request->data[2];
    
    /* Handle only Start Routine (0x01) for now */
    if (sub_function != UDS_RC_START_ROUTINE)
    {
        UDS_CreateNegativeResponse(request, UDS_NRC_SUBFUNCTION_NOT_SUPPORTED, response);
        return TRUE;
    }
    
    /* Prepare positive response */
    UDS_CreatePositiveResponse(request, response);
    
    /* Echo sub-function and routine ID */
    response->data[0] = sub_function;
    response->data[1] = request->data[1];  /* RID high */
    response->data[2] = request->data[2];  /* RID low */
    response->data_len = 3;
    
    /* Handle routine based on RID */
    switch (routine_id)
    {
        case UDS_RID_VCI_COLLECTION_START:  /* 0xF001 - Start VCI Collection */
        {
            /* Start VCI collection via UDS Client (DoIP) */
            if (VCI_Aggregator_Start())
            {
                /* Response: [sub][RID_H][RID_L][status=0x00=success] */
                response->data[3] = 0x00;  /* Success */
                response->data_len = 4;
            }
            else
            {
                /* Response: [sub][RID_H][RID_L][status=0x01=failure] */
                response->data[3] = 0x01;  /* Failure */
                response->data_len = 4;
            }
            
            return TRUE;
        }
        
        case UDS_RID_VCI_SEND_REPORT:  /* 0xF002 - Send VCI Report */
        {
            /* Check if DoIP is active */
            if (!DoIP_Client_IsActive())
            {
                /* Connection not ready */
                response->data[3] = 0x01;  /* Failure: Not connected */
                response->data_len = 4;
                sendUARTMessage("[UDS] VCI send failed: DoIP not active\r\n", 41);
                return TRUE;
            }
            
            /* Send consolidated VCI report */
            uint8 total_vci_count = g_zone_ecu_count + 1;  /* Zone ECUs + ZGW */
            
            if (DoIP_Client_SendVCIReport(total_vci_count, g_vci_database))
            {
                /* Response: [sub][RID_H][RID_L][status=0x00=success][count] */
                response->data[3] = 0x00;  /* Success */
                response->data[4] = total_vci_count;
                response->data_len = 5;
                
                char log_msg[64];
                sprintf(log_msg, "[UDS] VCI report sent (%u ECUs)\r\n", (unsigned int)total_vci_count);
                sendUARTMessage(log_msg, strlen(log_msg));
                
                return TRUE;
            }
            else
            {
                /* Send failed */
                response->data[3] = 0x02;  /* Failure: Send error */
                response->data_len = 4;
                sendUARTMessage("[UDS] VCI send failed: TCP error\r\n", 35);
                return TRUE;
            }
        }
        
        case UDS_RID_READINESS_CHECK:  /* 0xF003 - Start Readiness Check */
        {
            /* Start readiness check via UDS Client (DoIP) */
            if (Readiness_Aggregator_Start())
            {
                /* Response: [sub][RID_H][RID_L][status=0x00=success] */
                response->data[3] = 0x00;  /* Success */
                response->data_len = 4;
            }
            else
            {
                /* Response: [sub][RID_H][RID_L][status=0x01=failure] */
                response->data[3] = 0x01;  /* Failure */
                response->data_len = 4;
            }
            
            return TRUE;
        }
        
        case UDS_RID_READINESS_SEND_REPORT:  /* 0xF004 - Send Readiness Report */
        {
            /* Check if DoIP is active */
            if (!DoIP_Client_IsActive())
            {
                /* Connection not ready */
                response->data[3] = 0x01;  /* Failure: Not connected */
                response->data_len = 4;
                sendUARTMessage("[UDS] Readiness send failed: DoIP not active\r\n", 47);
                return TRUE;
            }
            
            /* Get collected readiness information */
            Readiness_Info readiness_array[MAX_ZONE_ECUS + 1];
            uint8 readiness_count;
            
            readiness_count = Readiness_Aggregator_GetResults(readiness_array, MAX_ZONE_ECUS + 1);
            
            if (readiness_count > 0)
            {
                /* Send via custom DoIP payload (similar to VCI) */
                /* TODO: Implement DoIP_Client_SendReadinessReport() */
                
                /* For now, embed in UDS response */
                /* Response: [sub][RID_H][RID_L][status=0x00][count][ready_data...] */
                response->data[3] = 0x00;  /* Success */
                response->data[4] = readiness_count;
                
                /* Copy first readiness info as example (ZGW) */
                response->data[5] = readiness_array[0].battery_soc;  /* Battery SOC */
                response->data[6] = readiness_array[0].temperature;  /* Temperature */
                response->data[7] = readiness_array[0].engine_state; /* Engine state */
                response->data[8] = readiness_array[0].parking_brake; /* Parking brake */
                response->data_len = 9;
                
                char log_msg[64];
                sprintf(log_msg, "[UDS] Readiness report sent (%u ECUs)\r\n", (unsigned int)readiness_count);
                sendUARTMessage(log_msg, strlen(log_msg));
                
                return TRUE;
            }
            else
            {
                /* No readiness data collected */
                response->data[3] = 0x02;  /* Failure: No data */
                response->data_len = 4;
                sendUARTMessage("[UDS] Readiness send failed: No data\r\n", 38);
                return TRUE;
            }
        }
        
        default:
        {
            /* Routine ID not supported */
            UDS_CreateNegativeResponse(request, UDS_NRC_REQUEST_OUT_OF_RANGE, response);
            return TRUE;
        }
    }
}

/*******************************************************************************
 * OTA Services (0x34, 0x36, 0x37)
 ******************************************************************************/

/**
 * @brief Handle 0x34 Request Download (Zone Package OTA)
 * @param request UDS request
 * @param response UDS response (output)
 * @return TRUE if handled, FALSE otherwise
 */
boolean UDS_Service_RequestDownload(const UDS_Request *request, UDS_Response *response)
{
    uint32 total_size;
    char log_msg[80];
    
    /* Parse request: [0x34][size:4 bytes] */
    if (request->data_len < 4)
    {
        UDS_CreateNegativeResponse(request, UDS_NRC_INCORRECT_MESSAGE_LENGTH, response);
        return TRUE;
    }
    
    /* Extract total size (Big Endian) */
    total_size = ((uint32)request->data[0] << 24) |
                 ((uint32)request->data[1] << 16) |
                 ((uint32)request->data[2] << 8) |
                 ((uint32)request->data[3]);
    
    sprintf(log_msg, "[UDS] 0x34 Request Download: %u bytes (%u MB)\r\n",
            total_size, total_size / (1024 * 1024));
    sendUARTMessage(log_msg, strlen(log_msg));
    
    /* Start OTA download */
    if (OTA_StartDownload(total_size))
    {
        /* Positive response: [0x74] */
        UDS_CreatePositiveResponse(request, response);
        sendUARTMessage("[UDS] 0x74: Download started\r\n", 30);
        return TRUE;
    }
    else
    {
        /* Negative response */
        UDS_CreateNegativeResponse(request, UDS_NRC_UPLOAD_DOWNLOAD_NOT_ACCEPTED, response);
        sendUARTMessage("[UDS] 0x34: Download rejected\r\n", 32);
        return TRUE;
    }
}

/**
 * @brief Handle 0x36 Transfer Data (Chunked)
 * @param request UDS request
 * @param response UDS response (output)
 * @return TRUE if handled, FALSE otherwise
 */
boolean UDS_Service_TransferData(const UDS_Request *request, UDS_Response *response)
{
    uint8 block_sequence;
    const uint8 *data;
    uint16 data_len;
    
    /* Parse request: [0x36][sequence:1 byte][data:variable] */
    if (request->data_len < 1)
    {
        UDS_CreateNegativeResponse(request, UDS_NRC_INCORRECT_MESSAGE_LENGTH, response);
        return TRUE;
    }
    
    block_sequence = request->data[0];
    data = &request->data[1];
    data_len = request->data_len - 1;
    
    /* Write chunk to OTA Manager */
    if (OTA_WriteChunk(data, data_len))
    {
        /* Positive response: [0x76][sequence] */
        UDS_CreatePositiveResponse(request, response);
        response->data[0] = block_sequence;
        response->data_len = 1;
        return TRUE;
    }
    else
    {
        /* Negative response */
        UDS_CreateNegativeResponse(request, UDS_NRC_GENERAL_PROGRAMMING_FAILURE, response);
        sendUARTMessage("[UDS] 0x36: Transfer failed\r\n", 30);
        return TRUE;
    }
}

/**
 * @brief Handle 0x37 Request Transfer Exit (Finish and verify)
 * @param request UDS request
 * @param response UDS response (output)
 * @return TRUE if handled, FALSE otherwise
 */
boolean UDS_Service_RequestTransferExit(const UDS_Request *request, UDS_Response *response)
{
    sendUARTMessage("[UDS] 0x37 Request Transfer Exit\r\n", 35);
    
    /* Finish download and verify */
    if (OTA_FinishDownload())
    {
        /* Positive response: [0x77] */
        UDS_CreatePositiveResponse(request, response);
        sendUARTMessage("[UDS] 0x77: Zone Package verified\r\n", 36);
        
        /* Auto-install ZGW firmware */
        sendUARTMessage("[UDS] Auto-installing ZGW firmware...\r\n", 40);
        if (OTA_InstallZGWFirmware())
        {
            sendUARTMessage("[UDS] ✅ ZGW firmware installed successfully\r\n", 47);
            sendUARTMessage("[UDS] System will reboot to apply update...\r\n", 46);
        }
        else
        {
            sendUARTMessage("[UDS] ❌ ZGW firmware installation failed\r\n", 44);
        }
        
        return TRUE;
    }
    else
    {
        /* Negative response */
        UDS_CreateNegativeResponse(request, UDS_NRC_GENERAL_PROGRAMMING_FAILURE, response);
        sendUARTMessage("[UDS] 0x37: Transfer exit failed\r\n", 35);
        return TRUE;
    }
}

