/*******************************************************************************
 * @file    uds_handler.h
 * @brief   UDS (Unified Diagnostic Services) Handler for DoIP
 * @details Implements ISO 14229-1 UDS services over DoIP (ISO 13400)
 * 
 * @version 1.0
 * @date    2025-11-04
 ******************************************************************************/

#ifndef UDS_HANDLER_H
#define UDS_HANDLER_H

#include "Ifx_Types.h"
#include "doip_types.h"

/*******************************************************************************
 * UDS Service IDs (ISO 14229-1)
 ******************************************************************************/

/* Diagnostic and Communication Management */
#define UDS_SID_DIAGNOSTIC_SESSION_CONTROL      0x10
#define UDS_SID_ECU_RESET                       0x11
#define UDS_SID_SECURITY_ACCESS                 0x27
#define UDS_SID_COMMUNICATION_CONTROL           0x28
#define UDS_SID_TESTER_PRESENT                  0x3E
#define UDS_SID_CONTROL_DTC_SETTING             0x85

/* Data Transmission */
#define UDS_SID_READ_DATA_BY_IDENTIFIER         0x22
#define UDS_SID_READ_MEMORY_BY_ADDRESS          0x23
#define UDS_SID_READ_SCALING_DATA_BY_ID         0x24
#define UDS_SID_READ_DATA_BY_PERIODIC_ID        0x2A
#define UDS_SID_DYNAMICALLY_DEFINE_DATA_ID      0x2C
#define UDS_SID_WRITE_DATA_BY_IDENTIFIER        0x2E
#define UDS_SID_WRITE_MEMORY_BY_ADDRESS         0x3D

/* Stored Data Transmission */
#define UDS_SID_CLEAR_DIAGNOSTIC_INFORMATION    0x14
#define UDS_SID_READ_DTC_INFORMATION            0x19

/* Input/Output Control */
#define UDS_SID_IO_CONTROL_BY_IDENTIFIER        0x2F

/* Routine Control */
#define UDS_SID_ROUTINE_CONTROL                 0x31

/* Routine Control Sub-functions */
#define UDS_RC_START_ROUTINE                    0x01
#define UDS_RC_STOP_ROUTINE                     0x02
#define UDS_RC_REQUEST_ROUTINE_RESULTS          0x03

/* Routine IDs for VCI Management */
#define UDS_RID_VCI_COLLECTION_START            0xF001  /* Start VCI collection from Zone ECUs */
#define UDS_RID_VCI_SEND_REPORT                 0xF002  /* Send consolidated VCI report to VMG */

/* Routine IDs for OTA Readiness Check */
#define UDS_RID_READINESS_CHECK                 0xF003  /* Start readiness check from Zone ECUs */
#define UDS_RID_READINESS_SEND_REPORT           0xF004  /* Send consolidated readiness report to VMG */

/* Upload/Download */
#define UDS_SID_REQUEST_DOWNLOAD                0x34
#define UDS_SID_REQUEST_UPLOAD                  0x35
#define UDS_SID_TRANSFER_DATA                   0x36
#define UDS_SID_REQUEST_TRANSFER_EXIT           0x37

/*******************************************************************************
 * UDS Response Codes
 ******************************************************************************/

/* Positive Response: Add 0x40 to Service ID */
#define UDS_POSITIVE_RESPONSE_OFFSET            0x40

/* Negative Response */
#define UDS_SID_NEGATIVE_RESPONSE               0x7F

/*******************************************************************************
 * UDS Negative Response Codes (NRC)
 ******************************************************************************/

#define UDS_NRC_GENERAL_REJECT                  0x10
#define UDS_NRC_SERVICE_NOT_SUPPORTED           0x11
#define UDS_NRC_SUBFUNCTION_NOT_SUPPORTED       0x12
#define UDS_NRC_INCORRECT_MESSAGE_LENGTH        0x13
#define UDS_NRC_RESPONSE_TOO_LONG               0x14

#define UDS_NRC_CONDITIONS_NOT_CORRECT          0x22
#define UDS_NRC_REQUEST_SEQUENCE_ERROR          0x24
#define UDS_NRC_REQUEST_OUT_OF_RANGE            0x31
#define UDS_NRC_SECURITY_ACCESS_DENIED          0x33
#define UDS_NRC_INVALID_KEY                     0x35
#define UDS_NRC_EXCEED_NUMBER_OF_ATTEMPTS       0x36
#define UDS_NRC_REQUIRED_TIME_DELAY_NOT_EXPIRED 0x37

#define UDS_NRC_UPLOAD_DOWNLOAD_NOT_ACCEPTED    0x70
#define UDS_NRC_TRANSFER_DATA_SUSPENDED         0x71
#define UDS_NRC_GENERAL_PROGRAMMING_FAILURE     0x72
#define UDS_NRC_WRONG_BLOCK_SEQUENCE_COUNTER    0x73

#define UDS_NRC_REQUEST_CORRECTLY_RECEIVED      0x78  /* Response Pending */

/*******************************************************************************
 * UDS Data Identifiers (DID) - Standard (ISO 14229-1)
 ******************************************************************************/

#define UDS_DID_VIN                             0xF190  /* Vehicle Identification Number */
#define UDS_DID_ECU_HW_NUMBER                   0xF191  /* ECU Hardware Number */
#define UDS_DID_SYSTEM_SUPPLIER_ID              0xF192  /* System Supplier Identifier */
#define UDS_DID_ECU_MFG_DATE                    0xF193  /* ECU Manufacturing Date */
#define UDS_DID_ECU_SW_NUMBER                   0xF194  /* ECU Software Number */
#define UDS_DID_SYSTEM_NAME_OR_ENGINE_TYPE      0xF197  /* System Name */
#define UDS_DID_REPAIR_SHOP_CODE                0xF198  /* Repair Shop Code */
#define UDS_DID_PROGRAMMING_DATE                0xF199  /* Programming Date */
#define UDS_DID_CALIBRATION_ID                  0xF19D  /* Calibration ID */
#define UDS_DID_APP_SW_VERSION                  0xF19E  /* Application SW Version */

/*******************************************************************************
 * Custom Data Identifiers (DID) - Project Specific
 ******************************************************************************/

/* VCI (Vehicle Configuration Information) DIDs */
#define UDS_DID_VCI_ECU_ID                      0xF194  /* Individual ECU ID + VCI */
#define UDS_DID_VCI_CONSOLIDATED                0xF195  /* Consolidated VCI (All Zone ECUs) */

/* ECU Health Status DIDs */
#define UDS_DID_HEALTH_STATUS                   0xF1A0  /* ECU Health Status (Dynamic) */
#define UDS_DID_HEALTH_STATUS_CONSOLIDATED      0xF1A1  /* Consolidated Health Status */

/* Diagnostic DIDs */
#define UDS_DID_ACTIVE_DTC_COUNT                0xF1B0  /* Active DTC Count */
#define UDS_DID_BATTERY_VOLTAGE                 0xF1B1  /* Battery Voltage */
#define UDS_DID_ECU_TEMPERATURE                 0xF1B2  /* ECU Temperature */

/*******************************************************************************
 * UDS Handler Configuration
 ******************************************************************************/

#define UDS_MAX_REQUEST_SIZE                    256     /* Max UDS request size */
#define UDS_MAX_RESPONSE_SIZE                   4096    /* Max UDS response size */
#define UDS_TIMEOUT_MS                          5000    /* UDS timeout: 5 seconds */

/*******************************************************************************
 * UDS Request/Response Structures
 ******************************************************************************/

/* UDS Request */
typedef struct
{
    uint16 source_address;      /* DoIP source address (e.g., 0x0E00 for VMG) */
    uint16 target_address;      /* DoIP target address (e.g., 0x0100 for ZGW) */
    uint8  service_id;          /* UDS Service ID */
    uint16 data_len;            /* Length of data[] */
    uint8  data[UDS_MAX_REQUEST_SIZE];  /* Service-specific data */
} UDS_Request;

/* UDS Response */
typedef struct
{
    uint16 source_address;      /* DoIP source address */
    uint16 target_address;      /* DoIP target address */
    boolean is_positive;        /* TRUE = Positive, FALSE = Negative */
    uint8  service_id;          /* UDS Service ID (with +0x40 for positive) */
    uint8  nrc;                 /* Negative Response Code (if negative) */
    uint16 data_len;            /* Length of data[] */
    uint8  data[UDS_MAX_RESPONSE_SIZE];  /* Response data */
} UDS_Response;

/*******************************************************************************
 * UDS Handler Function Pointers
 ******************************************************************************/

/* UDS Service Handler Function Type */
typedef boolean (*UDS_ServiceHandler)(const UDS_Request *request, UDS_Response *response);

/*******************************************************************************
 * Public Functions
 ******************************************************************************/

/**
 * @brief Initialize UDS Handler
 */
void UDS_Init(void);

/**
 * @brief Handle incoming UDS request
 * @param request Pointer to UDS request structure
 * @param response Pointer to UDS response structure (output)
 * @return TRUE if handled successfully, FALSE otherwise
 */
boolean UDS_HandleRequest(const UDS_Request *request, UDS_Response *response);

/**
 * @brief Parse DoIP Diagnostic Message (0x8001) to UDS Request
 * @param doip_payload DoIP diagnostic message payload (after DoIP header)
 * @param payload_len Length of DoIP payload
 * @param request Output UDS request structure
 * @return TRUE if parsed successfully, FALSE otherwise
 */
boolean UDS_ParseDoIPDiagnostic(const uint8 *doip_payload, uint32 payload_len, UDS_Request *request);

/**
 * @brief Build DoIP Diagnostic Message from UDS Response
 * @param response UDS response structure
 * @param buffer Output buffer for DoIP message
 * @param buffer_size Size of output buffer
 * @return Length of DoIP message, or 0 on error
 */
uint16 UDS_BuildDoIPDiagnostic(const UDS_Response *response, uint8 *buffer, uint16 buffer_size);

/*******************************************************************************
 * UDS Service Handlers (0x22 Read Data By Identifier)
 ******************************************************************************/

/**
 * @brief Handle 0x22 Read Data By Identifier
 * @param request UDS request
 * @param response UDS response (output)
 * @return TRUE if handled, FALSE otherwise
 */
boolean UDS_Service_ReadDataByIdentifier(const UDS_Request *request, UDS_Response *response);

/**
 * @brief Handle 0x31 Routine Control
 * @param request UDS request
 * @param response UDS response (output)
 * @return TRUE if handled, FALSE otherwise
 */
boolean UDS_Service_RoutineControl(const UDS_Request *request, UDS_Response *response);

/**
 * @brief Handle 0x34 Request Download (OTA)
 * @param request UDS request
 * @param response UDS response (output)
 * @return TRUE if handled, FALSE otherwise
 */
boolean UDS_Service_RequestDownload(const UDS_Request *request, UDS_Response *response);

/**
 * @brief Handle 0x36 Transfer Data (OTA)
 * @param request UDS request
 * @param response UDS response (output)
 * @return TRUE if handled, FALSE otherwise
 */
boolean UDS_Service_TransferData(const UDS_Request *request, UDS_Response *response);

/**
 * @brief Handle 0x37 Request Transfer Exit (OTA)
 * @param request UDS request
 * @param response UDS response (output)
 * @return TRUE if handled, FALSE otherwise
 */
boolean UDS_Service_RequestTransferExit(const UDS_Request *request, UDS_Response *response);

/**
 * @brief Read VCI Data for a specific DID
 * @param did Data Identifier
 * @param data Output buffer for DID data
 * @param data_len Output length of DID data
 * @return TRUE if DID is supported and data is available, FALSE otherwise
 */
boolean UDS_ReadDID_VCI(uint16 did, uint8 *data, uint16 *data_len);

/**
 * @brief Read Individual ECU VCI (DID 0xF194)
 * @param ecu_address ECU address to query
 * @param vci_info Output VCI information
 * @return TRUE if successful, FALSE otherwise
 */
boolean UDS_ReadIndividualVCI(uint16 ecu_address, DoIP_VCI_Info *vci_info);

/**
 * @brief Read Consolidated VCI (DID 0xF195)
 * @param vci_array Output array of VCI information
 * @param vci_count Output count of VCI entries
 * @return TRUE if successful, FALSE otherwise
 */
boolean UDS_ReadConsolidatedVCI(DoIP_VCI_Info *vci_array, uint8 *vci_count);

/**
 * @brief Read Health Status (DID 0xF1A0)
 * @param health_array Output array of health status
 * @param health_count Output count of health entries
 * @return TRUE if successful, FALSE otherwise
 */
boolean UDS_ReadHealthStatus(DoIP_HealthStatus_Info *health_array, uint8 *health_count);

/*******************************************************************************
 * Helper Functions
 ******************************************************************************/

/**
 * @brief Create Negative Response
 * @param request Original request
 * @param nrc Negative Response Code
 * @param response Output response structure
 */
void UDS_CreateNegativeResponse(const UDS_Request *request, uint8 nrc, UDS_Response *response);

/**
 * @brief Create Positive Response
 * @param request Original request
 * @param response Output response structure
 */
void UDS_CreatePositiveResponse(const UDS_Request *request, UDS_Response *response);

/*******************************************************************************
 * UDS CLIENT - ZGW sends requests to Zone ECUs
 ******************************************************************************/

/**
 * @brief UDS Client callback for receiving responses
 * @param ecu_ip ECU IP address
 * @param response_data UDS response data
 * @param response_len Length of response data
 */
typedef void (*UDS_Client_ResponseCallback)(const char *ecu_ip, uint8 *response_data, uint16 response_len);

/**
 * @brief Send UDS request to Zone ECU via DoIP
 * @param ecu_ip Zone ECU IP address
 * @param uds_request UDS request data (service ID + parameters)
 * @param request_len Length of UDS request
 * @param callback Callback for response (can be NULL)
 * @return TRUE if request sent successfully, FALSE otherwise
 */
boolean UDS_Client_SendRequest(const char *ecu_ip, uint8 *uds_request, uint16 request_len, UDS_Client_ResponseCallback callback);

/**
 * @brief Send 0x22 ReadDataByID request to Zone ECU (VCI)
 * @param ecu_ip Zone ECU IP address
 * @param did Data Identifier (e.g., 0xF194 for VCI)
 * @param callback Callback for response
 * @return TRUE if request sent successfully, FALSE otherwise
 */
boolean UDS_Client_ReadVCI(const char *ecu_ip, uint16 did, UDS_Client_ResponseCallback callback);

/**
 * @brief Send 0x31 RoutineControl request to Zone ECU (Readiness)
 * @param ecu_ip Zone ECU IP address
 * @param routine_id Routine ID (e.g., 0xF003 for readiness)
 * @param callback Callback for response
 * @return TRUE if request sent successfully, FALSE otherwise
 */
boolean UDS_Client_CheckReadiness(const char *ecu_ip, uint16 routine_id, UDS_Client_ResponseCallback callback);

#endif /* UDS_HANDLER_H */

