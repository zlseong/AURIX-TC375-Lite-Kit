/*******************************************************************************
 * @file    uds_handler.c
 * @brief   UDS (Unified Diagnostic Services) Handler Implementation
 * @details Implements ISO 14229-1 UDS services over DoIP (ISO 13400)
 * 
 * @version 1.0
 * @date    2025-11-04
 ******************************************************************************/

#include "uds_handler.h"
#include "uds_download.h"
#include "doip_types.h"
#include "doip_client.h"
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
    sprintf(log_msg, "[UDS] RX: SID=0x%02X, SA=0x%04X, TA=0x%04X, Len=%d\r\n",
            request->service_id, request->source_address, request->target_address, request->data_len);
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
    uint32 payload_len = 4 + 1 + response->data_len;  /* Routing + SID + data */
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
    sprintf(log_msg, "[UDS] TX: SID=0x%02X, SA=0x%04X, TA=0x%04X, Total=%d bytes\r\n",
            response->service_id, response->source_address, response->target_address, total_len);
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

