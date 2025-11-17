/**
 * @file doip_link.h
 * @brief DoIP Link Layer - Role-based unified Server/Client implementation
 * @details Provides a unified interface for DoIP communication that supports
 *          both Server (Listen/Accept) and Client (Connect) roles.
 */

#ifndef DOIP_LINK_H
#define DOIP_LINK_H

#include "doip_types.h"
#include "lwip/tcp.h"
#include "Ifx_Types.h"

/*******************************************************************************
 * DoIP Link Role
 ******************************************************************************/

typedef enum
{
    DOIP_ROLE_SERVER,   /* Server role: Listen and accept connections */
    DOIP_ROLE_CLIENT    /* Client role: Connect to remote server */
} DoIPLinkRole;

/*******************************************************************************
 * DoIP Link State
 ******************************************************************************/

typedef enum
{
    DOIP_LINK_STATE_IDLE,           /* Not initialized */
    DOIP_LINK_STATE_LISTENING,      /* Server: Listening for connections */
    DOIP_LINK_STATE_CONNECTING,     /* Client: Connection in progress */
    DOIP_LINK_STATE_CONNECTED,      /* TCP connected */
    DOIP_LINK_STATE_AUTHENTICATED,  /* Routing activated */
    DOIP_LINK_STATE_ERROR           /* Error state */
} DoIPLinkState;

/*******************************************************************************
 * DoIP Link Structure
 ******************************************************************************/

typedef struct DoIPLink DoIPLink;  /* Forward declaration */

/* Callback function types */
typedef void (*DoIPLink_RecvCallback)(DoIPLink *link, uint8 *data, uint16 len);
typedef void (*DoIPLink_ConnectedCallback)(DoIPLink *link);
typedef void (*DoIPLink_DisconnectedCallback)(DoIPLink *link);

struct DoIPLink
{
    /* Configuration */
    DoIPLinkRole role;              /* Server or Client */
    uint16 local_port;              /* Local port (Server: listen port, Client: source port) */
    
    /* Remote address (Client only) */
    ip_addr_t remote_addr;          /* Remote IP address */
    uint16 remote_port;             /* Remote port */
    
    /* Connection state */
    DoIPLinkState state;            /* Current state */
    struct tcp_pcb *listen_pcb;     /* Server: Listen PCB */
    struct tcp_pcb *conn_pcb;       /* Connection PCB (active connection) */
    
    /* DoIP session */
    uint16 logical_address;         /* Local logical address */
    uint16 remote_logical_address;  /* Remote logical address */
    boolean routing_activated;      /* Routing activation status */
    
    /* RX/TX Buffers */
    uint8 rx_buffer[DOIP_MAX_MESSAGE_SIZE];  /* Receive buffer */
    uint16 rx_length;                        /* Received data length */
    
    /* Callbacks */
    DoIPLink_RecvCallback recv_callback;
    DoIPLink_ConnectedCallback connected_callback;
    DoIPLink_DisconnectedCallback disconnected_callback;
    
    /* User data */
    void *user_data;                /* User-defined data pointer */
};

/*******************************************************************************
 * Function Prototypes
 ******************************************************************************/

/**
 * @brief Initialize DoIP Link
 * @param link DoIP Link structure
 * @param role Server or Client role
 * @param local_port Local port number
 * @param logical_addr Local logical address
 * @return TRUE if successful, FALSE otherwise
 */
boolean DoIP_Link_Init(DoIPLink *link, DoIPLinkRole role, uint16 local_port, uint16 logical_addr);

/**
 * @brief Set remote address (Client only)
 * @param link DoIP Link structure
 * @param remote_ip Remote IP address string
 * @param remote_port Remote port number
 * @return TRUE if successful, FALSE otherwise
 */
boolean DoIP_Link_SetRemote(DoIPLink *link, const char *remote_ip, uint16 remote_port);

/**
 * @brief Set callbacks
 * @param link DoIP Link structure
 * @param recv_cb Receive callback
 * @param connected_cb Connected callback
 * @param disconnected_cb Disconnected callback
 */
void DoIP_Link_SetCallbacks(DoIPLink *link,
                            DoIPLink_RecvCallback recv_cb,
                            DoIPLink_ConnectedCallback connected_cb,
                            DoIPLink_DisconnectedCallback disconnected_cb);

/**
 * @brief Start DoIP Link (Server: Listen, Client: Connect)
 * @param link DoIP Link structure
 * @return TRUE if successful, FALSE otherwise
 */
boolean DoIP_Link_Start(DoIPLink *link);

/**
 * @brief Send data over DoIP Link
 * @param link DoIP Link structure
 * @param data Data buffer
 * @param len Data length
 * @return TRUE if successful, FALSE otherwise
 */
boolean DoIP_Link_Send(DoIPLink *link, const uint8 *data, uint16 len);

/**
 * @brief Close DoIP Link
 * @param link DoIP Link structure
 */
void DoIP_Link_Close(DoIPLink *link);

/**
 * @brief Get current state
 * @param link DoIP Link structure
 * @return Current state
 */
DoIPLinkState DoIP_Link_GetState(const DoIPLink *link);

/**
 * @brief Check if link is active (routing activated)
 * @param link DoIP Link structure
 * @return TRUE if active, FALSE otherwise
 */
boolean DoIP_Link_IsActive(const DoIPLink *link);

/**
 * @brief Send DoIP Routing Activation Request (Client only)
 * @param link DoIP Link structure
 * @return TRUE if successful, FALSE otherwise
 */
boolean DoIP_Link_SendRoutingActivation(DoIPLink *link);

/**
 * @brief Send DoIP Routing Activation Response (Server only)
 * @param link DoIP Link structure
 * @param response_code Response code
 * @return TRUE if successful, FALSE otherwise
 */
boolean DoIP_Link_SendRoutingActivationResponse(DoIPLink *link, uint8 response_code);

#endif /* DOIP_LINK_H */


