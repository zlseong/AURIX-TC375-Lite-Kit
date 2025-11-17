/**
 * @file udp_link.h
 * @brief UDP Link Layer - Role-based UDP communication
 * @details Provides a unified interface for UDP communication supporting
 *          Server (Bind/Receive), Client (Send), and Broadcast roles.
 */

#ifndef UDP_LINK_H
#define UDP_LINK_H

#include "Ifx_Types.h"
#include "lwip/udp.h"
#include "lwip/ip_addr.h"

/*******************************************************************************
 * UDP Link Role
 ******************************************************************************/

typedef enum
{
    UDP_LINK_ROLE_SERVER,      /* Server: Bind and receive */
    UDP_LINK_ROLE_CLIENT,      /* Client: Send to specific address */
    UDP_LINK_ROLE_BROADCAST    /* Broadcast: Send to all (255.255.255.255) */
} UDPLinkRole;

/*******************************************************************************
 * UDP Link State
 ******************************************************************************/

typedef enum
{
    UDP_LINK_STATE_IDLE,       /* Not initialized */
    UDP_LINK_STATE_READY,      /* Ready for communication */
    UDP_LINK_STATE_ERROR       /* Error state */
} UDPLinkState;

/*******************************************************************************
 * UDP Link Structure
 ******************************************************************************/

typedef struct UDPLink UDPLink;  /* Forward declaration */

/* Callback function type */
typedef void (*UDPLink_RecvCallback)(UDPLink *link, uint8 *data, uint16 len, 
                                     ip_addr_t *src_addr, uint16 src_port);

struct UDPLink
{
    /* Configuration */
    UDPLinkRole role;              /* Server, Client, or Broadcast */
    uint16 local_port;             /* Local port (Server: bind port, Client: source port) */
    
    /* State */
    UDPLinkState state;            /* Current state */
    struct udp_pcb *pcb;           /* UDP PCB */
    
    /* Callback */
    UDPLink_RecvCallback recv_callback;
    
    /* User data */
    void *user_data;               /* User-defined data pointer */
};

/*******************************************************************************
 * Function Prototypes
 ******************************************************************************/

/**
 * @brief Initialize UDP Link
 * @param link UDP Link structure
 * @param role Server, Client, or Broadcast role
 * @param local_port Local port number (0 for auto-assign in Client mode)
 * @return TRUE if successful, FALSE otherwise
 */
boolean UDP_Link_Init(UDPLink *link, UDPLinkRole role, uint16 local_port);

/**
 * @brief Set receive callback (Server only)
 * @param link UDP Link structure
 * @param recv_cb Receive callback function
 */
void UDP_Link_SetCallback(UDPLink *link, UDPLink_RecvCallback recv_cb);

/**
 * @brief Start UDP Link (Server: Bind, Client: Ready)
 * @param link UDP Link structure
 * @return TRUE if successful, FALSE otherwise
 */
boolean UDP_Link_Start(UDPLink *link);

/**
 * @brief Send data over UDP
 * @param link UDP Link structure
 * @param data Data buffer
 * @param len Data length
 * @param dest_ip Destination IP address (NULL for broadcast in BROADCAST role)
 * @param dest_port Destination port
 * @return TRUE if successful, FALSE otherwise
 */
boolean UDP_Link_Send(UDPLink *link, const uint8 *data, uint16 len,
                     const char *dest_ip, uint16 dest_port);

/**
 * @brief Close UDP Link
 * @param link UDP Link structure
 */
void UDP_Link_Close(UDPLink *link);

/**
 * @brief Get current state
 * @param link UDP Link structure
 * @return Current state
 */
UDPLinkState UDP_Link_GetState(const UDPLink *link);

#endif /* UDP_LINK_H */


