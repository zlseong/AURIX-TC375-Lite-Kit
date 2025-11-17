/**
 * @file udp_link.c
 * @brief UDP Link Layer Implementation - Role-based UDP communication
 */

#include "udp_link.h"
#include "Ifx_Lwip.h"
#include "UART_Logging.h"
#include <string.h>
#include <stdio.h>

/*******************************************************************************
 * Forward Declarations
 ******************************************************************************/

static void udp_link_recv_callback(void *arg, struct udp_pcb *pcb, struct pbuf *p,
                                   const ip_addr_t *addr, u16_t port);

/*******************************************************************************
 * Initialization
 ******************************************************************************/

boolean UDP_Link_Init(UDPLink *link, UDPLinkRole role, uint16 local_port)
{
    if (link == NULL)
    {
        return FALSE;
    }
    
    /* Clear structure */
    memset(link, 0, sizeof(UDPLink));
    
    /* Set configuration */
    link->role = role;
    link->local_port = local_port;
    link->state = UDP_LINK_STATE_IDLE;
    
    char msg[64];
    const char* role_str = (role == UDP_LINK_ROLE_SERVER) ? "SERVER" :
                          (role == UDP_LINK_ROLE_BROADCAST) ? "BROADCAST" : "CLIENT";
    sprintf(msg, "[UDP Link] Init as %s port %d\r\n", role_str, local_port);
    sendUARTMessage(msg, strlen(msg));
    
    return TRUE;
}

void UDP_Link_SetCallback(UDPLink *link, UDPLink_RecvCallback recv_cb)
{
    if (link == NULL)
    {
        return;
    }
    
    link->recv_callback = recv_cb;
}

/*******************************************************************************
 * Start Link
 ******************************************************************************/

boolean UDP_Link_Start(UDPLink *link)
{
    char msg[64];
    
    if (link == NULL)
    {
        return FALSE;
    }
    
    /* Create UDP PCB */
    link->pcb = udp_new();
    if (link->pcb == NULL)
    {
        sprintf(msg, "[UDP Link] Failed to create PCB\r\n");
        sendUARTMessage(msg, strlen(msg));
        return FALSE;
    }
    
    if (link->role == UDP_LINK_ROLE_SERVER)
    {
        /* Server mode: Bind to local port */
        err_t err = udp_bind(link->pcb, IP_ADDR_ANY, link->local_port);
        if (err != ERR_OK)
        {
            sprintf(msg, "[UDP Link] Bind failed: %d\r\n", err);
            sendUARTMessage(msg, strlen(msg));
            udp_remove(link->pcb);
            link->pcb = NULL;
            return FALSE;
        }
        
        /* Set receive callback */
        udp_recv(link->pcb, udp_link_recv_callback, link);
        
        link->state = UDP_LINK_STATE_READY;
        
        sprintf(msg, "[UDP Link] Server ready on :%d\r\n", link->local_port);
        sendUARTMessage(msg, strlen(msg));
    }
    else if (link->role == UDP_LINK_ROLE_BROADCAST)
    {
        /* Broadcast mode: Optional bind to local port */
        if (link->local_port != 0)
        {
            err_t err = udp_bind(link->pcb, IP_ADDR_ANY, link->local_port);
            if (err != ERR_OK)
            {
                sprintf(msg, "[UDP Link] Bind failed: %d\r\n", err);
                sendUARTMessage(msg, strlen(msg));
                udp_remove(link->pcb);
                link->pcb = NULL;
                return FALSE;
            }
        }
        
        link->state = UDP_LINK_STATE_READY;
        
        sprintf(msg, "[UDP Link] Broadcast ready\r\n");
        sendUARTMessage(msg, strlen(msg));
    }
    else  /* UDP_LINK_ROLE_CLIENT */
    {
        /* Client mode: Optional bind to source port */
        if (link->local_port != 0)
        {
            err_t err = udp_bind(link->pcb, IP_ADDR_ANY, link->local_port);
            if (err != ERR_OK)
            {
                sprintf(msg, "[UDP Link] Bind failed: %d\r\n", err);
                sendUARTMessage(msg, strlen(msg));
                udp_remove(link->pcb);
                link->pcb = NULL;
                return FALSE;
            }
        }
        
        link->state = UDP_LINK_STATE_READY;
        
        sprintf(msg, "[UDP Link] Client ready\r\n");
        sendUARTMessage(msg, strlen(msg));
    }
    
    return TRUE;
}

/*******************************************************************************
 * Receive Callback
 ******************************************************************************/

static void udp_link_recv_callback(void *arg, struct udp_pcb *pcb, struct pbuf *p,
                                   const ip_addr_t *addr, u16_t port)
{
    UDPLink *link = (UDPLink*)arg;
    
    if (link == NULL || p == NULL)
    {
        if (p != NULL)
        {
            pbuf_free(p);
        }
        return;
    }
    
    /* Extract data */
    uint8 buffer[256];
    uint16 len = (p->tot_len > 256) ? 256 : p->tot_len;
    
    pbuf_copy_partial(p, buffer, len, 0);
    
    /* Call user callback */
    if (link->recv_callback != NULL)
    {
        link->recv_callback(link, buffer, len, (ip_addr_t*)addr, port);
    }
    
    /* Free pbuf */
    pbuf_free(p);
}

/*******************************************************************************
 * Send Data
 ******************************************************************************/

boolean UDP_Link_Send(UDPLink *link, const uint8 *data, uint16 len,
                     const char *dest_ip, uint16 dest_port)
{
    char msg[64];
    
    if (link == NULL || link->pcb == NULL || data == NULL || len == 0)
    {
        return FALSE;
    }
    
    if (link->state != UDP_LINK_STATE_READY)
    {
        return FALSE;
    }
    
    /* Parse destination IP */
    ip_addr_t dest_addr;
    
    if (link->role == UDP_LINK_ROLE_BROADCAST && dest_ip == NULL)
    {
        /* Use broadcast address */
        IP4_ADDR(&dest_addr, 255, 255, 255, 255);
    }
    else if (dest_ip != NULL)
    {
        if (!ip4addr_aton(dest_ip, &dest_addr))
        {
            sprintf(msg, "[UDP Link] Invalid IP: %s\r\n", dest_ip);
            sendUARTMessage(msg, strlen(msg));
            return FALSE;
        }
    }
    else
    {
        sprintf(msg, "[UDP Link] Dest IP required\r\n");
        sendUARTMessage(msg, strlen(msg));
        return FALSE;
    }
    
    /* Allocate pbuf */
    struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, len, PBUF_RAM);
    if (p == NULL)
    {
        sprintf(msg, "[UDP Link] pbuf alloc failed\r\n");
        sendUARTMessage(msg, strlen(msg));
        return FALSE;
    }
    
    /* Copy data */
    memcpy(p->payload, data, len);
    
    /* Send */
    err_t err = udp_sendto(link->pcb, p, &dest_addr, dest_port);
    
    /* Free pbuf */
    pbuf_free(p);
    
    if (err != ERR_OK)
    {
        sprintf(msg, "[UDP Link] Send failed: %d\r\n", err);
        sendUARTMessage(msg, strlen(msg));
        return FALSE;
    }
    
    return TRUE;
}

/*******************************************************************************
 * Close Link
 ******************************************************************************/

void UDP_Link_Close(UDPLink *link)
{
    char msg[32];
    
    if (link == NULL)
    {
        return;
    }
    
    if (link->pcb != NULL)
    {
        udp_remove(link->pcb);
        link->pcb = NULL;
    }
    
    link->state = UDP_LINK_STATE_IDLE;
    
    sprintf(msg, "[UDP Link] Closed\r\n");
    sendUARTMessage(msg, strlen(msg));
}

/*******************************************************************************
 * Query Functions
 ******************************************************************************/

UDPLinkState UDP_Link_GetState(const UDPLink *link)
{
    if (link == NULL)
    {
        return UDP_LINK_STATE_IDLE;
    }
    
    return link->state;
}

