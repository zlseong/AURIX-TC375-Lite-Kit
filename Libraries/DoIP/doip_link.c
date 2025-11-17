/**
 * @file doip_link.c
 * @brief DoIP Link Layer Implementation - Role-based unified Server/Client
 */

#include "doip_link.h"
#include "doip_message.h"
#include "Ifx_Lwip.h"
#include "IfxStm.h"
#include "UART_Logging.h"
#include <string.h>
#include <stdio.h>

/*******************************************************************************
 * Forward Declarations
 ******************************************************************************/

static err_t doip_link_accept_callback(void *arg, struct tcp_pcb *newpcb, err_t err);
static err_t doip_link_connected_callback(void *arg, struct tcp_pcb *tpcb, err_t err);
static err_t doip_link_recv_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
static void doip_link_error_callback(void *arg, err_t err);

/*******************************************************************************
 * Initialization
 ******************************************************************************/

boolean DoIP_Link_Init(DoIPLink *link, DoIPLinkRole role, uint16 local_port, uint16 logical_addr)
{
    char msg[80];
    
    if (link == NULL)
    {
        return FALSE;
    }
    
    /* Clear structure */
    memset(link, 0, sizeof(DoIPLink));
    
    /* Set configuration */
    link->role = role;
    link->local_port = local_port;
    link->logical_address = logical_addr;
    link->state = DOIP_LINK_STATE_IDLE;
    link->routing_activated = FALSE;
    
    sprintf(msg, "[DoIP Link] Init %s port %d Addr=0x%04X\r\n",
           (role == DOIP_ROLE_SERVER) ? "SERVER" : "CLIENT",
           local_port, logical_addr);
    sendUARTMessage(msg, strlen(msg));
    
    return TRUE;
}

boolean DoIP_Link_SetRemote(DoIPLink *link, const char *remote_ip, uint16 remote_port)
{
    char msg[80];
    
    if (link == NULL || link->role != DOIP_ROLE_CLIENT)
    {
        return FALSE;
    }
    
    /* Parse IP address */
    if (!ip4addr_aton(remote_ip, &link->remote_addr))
    {
        sprintf(msg, "[DoIP Link] Invalid IP: %s\r\n", remote_ip);
        sendUARTMessage(msg, strlen(msg));
        return FALSE;
    }
    
    link->remote_port = remote_port;
    
    sprintf(msg, "[DoIP Link] Remote=%s:%d\r\n", remote_ip, remote_port);
    sendUARTMessage(msg, strlen(msg));
    
    return TRUE;
}

void DoIP_Link_SetCallbacks(DoIPLink *link,
                            DoIPLink_RecvCallback recv_cb,
                            DoIPLink_ConnectedCallback connected_cb,
                            DoIPLink_DisconnectedCallback disconnected_cb)
{
    if (link == NULL)
    {
        return;
    }
    
    link->recv_callback = recv_cb;
    link->connected_callback = connected_cb;
    link->disconnected_callback = disconnected_cb;
}

/*******************************************************************************
 * Start Link (Server: Listen, Client: Connect)
 ******************************************************************************/

boolean DoIP_Link_Start(DoIPLink *link)
{
    char msg[64];
    
    if (link == NULL)
    {
        return FALSE;
    }
    
    if (link->role == DOIP_ROLE_SERVER)
    {
        /* Server mode: Create listening socket */
        link->listen_pcb = tcp_new();
        if (link->listen_pcb == NULL)
        {
            sprintf(msg, "[DoIP Link] tcp_new() failed\r\n");
            sendUARTMessage(msg, strlen(msg));
            return FALSE;
        }
        
        /* Bind to local port */
        err_t err = tcp_bind(link->listen_pcb, IP_ADDR_ANY, link->local_port);
        if (err != ERR_OK)
        {
            sprintf(msg, "[DoIP Link] Bind failed: %d\r\n", err);
            sendUARTMessage(msg, strlen(msg));
            tcp_close(link->listen_pcb);
            return FALSE;
        }
        
        /* Start listening */
        link->listen_pcb = tcp_listen(link->listen_pcb);
        if (link->listen_pcb == NULL)
        {
            sprintf(msg, "[DoIP Link] tcp_listen() failed\r\n");
            sendUARTMessage(msg, strlen(msg));
            return FALSE;
        }
        
        /* Set accept callback */
        tcp_arg(link->listen_pcb, link);
        tcp_accept(link->listen_pcb, doip_link_accept_callback);
        
        link->state = DOIP_LINK_STATE_LISTENING;
        
        sprintf(msg, "[DoIP Link] Server listening on :%d\r\n", link->local_port);
        sendUARTMessage(msg, strlen(msg));
    }
    else  /* DOIP_ROLE_CLIENT */
    {
        /* Client mode: Create PCB and connect */
        link->conn_pcb = tcp_new();
        if (link->conn_pcb == NULL)
        {
            sprintf(msg, "[DoIP Link] tcp_new() failed\r\n");
            sendUARTMessage(msg, strlen(msg));
            return FALSE;
        }
        
        /* Set callbacks */
        tcp_arg(link->conn_pcb, link);
        tcp_err(link->conn_pcb, doip_link_error_callback);
        
        /* Initiate connection */
        err_t err = tcp_connect(link->conn_pcb, &link->remote_addr, 
                               link->remote_port, doip_link_connected_callback);
        if (err != ERR_OK)
        {
            sprintf(msg, "[DoIP Link] tcp_connect() failed: %d\r\n", err);
            sendUARTMessage(msg, strlen(msg));
            tcp_close(link->conn_pcb);
            link->conn_pcb = NULL;
            return FALSE;
        }
        
        link->state = DOIP_LINK_STATE_CONNECTING;
        
        sprintf(msg, "[DoIP Link] Connecting...\r\n");
        sendUARTMessage(msg, strlen(msg));
    }
    
    return TRUE;
}

/*******************************************************************************
 * lwIP Callbacks
 ******************************************************************************/

static err_t doip_link_accept_callback(void *arg, struct tcp_pcb *newpcb, err_t err)
{
    char msg[64];
    DoIPLink *link = (DoIPLink*)arg;
    
    if (link == NULL || newpcb == NULL || err != ERR_OK)
    {
        return ERR_VAL;
    }
    
    /* Accept only one connection */
    if (link->conn_pcb != NULL)
    {
        sprintf(msg, "[DoIP Link] Connection rejected (busy)\r\n");
        sendUARTMessage(msg, strlen(msg));
        tcp_close(newpcb);
        return ERR_CONN;
    }
    
    link->conn_pcb = newpcb;
    link->state = DOIP_LINK_STATE_CONNECTED;
    
    /* Set callbacks */
    tcp_arg(newpcb, link);
    tcp_recv(newpcb, doip_link_recv_callback);
    tcp_err(newpcb, doip_link_error_callback);
    
    sprintf(msg, "[DoIP Link] Client connected\r\n");
    sendUARTMessage(msg, strlen(msg));
    
    /* Call user callback */
    if (link->connected_callback != NULL)
    {
        link->connected_callback(link);
    }
    
    return ERR_OK;
}

static err_t doip_link_connected_callback(void *arg, struct tcp_pcb *tpcb, err_t err)
{
    char msg[64];
    DoIPLink *link = (DoIPLink*)arg;
    
    if (link == NULL || err != ERR_OK)
    {
        sprintf(msg, "[DoIP Link] Connect failed: %d\r\n", err);
        sendUARTMessage(msg, strlen(msg));
        return err;
    }
    
    link->state = DOIP_LINK_STATE_CONNECTED;
    
    /* Set recv callback */
    tcp_recv(tpcb, doip_link_recv_callback);
    
    sprintf(msg, "[DoIP Link] Connected!\r\n");
    sendUARTMessage(msg, strlen(msg));
    
    /* Call user callback */
    if (link->connected_callback != NULL)
    {
        link->connected_callback(link);
    }
    
    return ERR_OK;
}

static err_t doip_link_recv_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
    DoIPLink *link;
    char msg[64];
    uint16 len;
    DoIP_Header header;
    uint32 total_len;
    
    link = (DoIPLink*)arg;
    
    if (link == NULL)
    {
        if (p != NULL)
        {
            pbuf_free(p);
        }
        return ERR_ARG;
    }
    
    /* Connection closed */
    if (p == NULL)
    {
        sprintf(msg, "[DoIP Link] Connection closed\r\n");
        sendUARTMessage(msg, strlen(msg));
        
        link->state = DOIP_LINK_STATE_IDLE;
        link->routing_activated = FALSE;
        
        if (link->disconnected_callback != NULL)
        {
            link->disconnected_callback(link);
        }
        
        return ERR_OK;
    }
    
    /* Copy data to buffer */
    len = p->tot_len;
    if (len > (DOIP_MAX_MESSAGE_SIZE - link->rx_length))
    {
        len = DOIP_MAX_MESSAGE_SIZE - link->rx_length;
    }
    
    pbuf_copy_partial(p, &link->rx_buffer[link->rx_length], len, 0);
    link->rx_length += len;
    
    tcp_recved(tpcb, p->tot_len);
    pbuf_free(p);
    
    /* Process complete DoIP messages */
    while (link->rx_length >= DOIP_HEADER_SIZE)
    {
        if (!DoIP_ParseHeader(link->rx_buffer, &header))
        {
            break;  /* Invalid header */
        }
        
        total_len = DOIP_HEADER_SIZE + header.payloadLength;
        
        if (link->rx_length < total_len)
        {
            break;  /* Wait for more data */
        }
        
        /* Call user callback */
        if (link->recv_callback != NULL)
        {
            link->recv_callback(link, link->rx_buffer, (uint16)total_len);
        }
        
        /* Remove processed message */
        if (link->rx_length > total_len)
        {
            memmove(link->rx_buffer, &link->rx_buffer[total_len], link->rx_length - total_len);
        }
        link->rx_length -= (uint16)total_len;
    }
    
    return ERR_OK;
}

static void doip_link_error_callback(void *arg, err_t err)
{
    char msg[64];
    DoIPLink *link = (DoIPLink*)arg;
    
    if (link == NULL)
    {
        return;
    }
    
    sprintf(msg, "[DoIP Link] Error: %d\r\n", err);
    sendUARTMessage(msg, strlen(msg));
    
    link->state = DOIP_LINK_STATE_ERROR;
    link->conn_pcb = NULL;
    
    if (link->disconnected_callback != NULL)
    {
        link->disconnected_callback(link);
    }
}

/*******************************************************************************
 * Send Functions
 ******************************************************************************/

boolean DoIP_Link_Send(DoIPLink *link, const uint8 *data, uint16 len)
{
    char msg[64];
    
    if (link == NULL || link->conn_pcb == NULL || data == NULL || len == 0)
    {
        return FALSE;
    }
    
    if (link->state != DOIP_LINK_STATE_CONNECTED && 
        link->state != DOIP_LINK_STATE_AUTHENTICATED)
    {
        return FALSE;
    }
    
    /* Allocate pbuf */
    struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, len, PBUF_RAM);
    if (p == NULL)
    {
        sprintf(msg, "[DoIP Link] pbuf alloc failed\r\n");
        sendUARTMessage(msg, strlen(msg));
        return FALSE;
    }
    
    /* Copy data */
    memcpy(p->payload, data, len);
    
    /* Send */
    err_t err = tcp_write(link->conn_pcb, p->payload, len, TCP_WRITE_FLAG_COPY);
    
    pbuf_free(p);
    
    if (err != ERR_OK)
    {
        sprintf(msg, "[DoIP Link] tcp_write() failed: %d\r\n", err);
        sendUARTMessage(msg, strlen(msg));
        return FALSE;
    }
    
    /* Flush */
    tcp_output(link->conn_pcb);
    
    return TRUE;
}

/*******************************************************************************
 * DoIP Protocol Functions
 ******************************************************************************/

boolean DoIP_Link_SendRoutingActivation(DoIPLink *link)
{
    char msg[64];
    uint8 buffer[DOIP_HEADER_SIZE + 11];  /* RoutingActivation payload = 11 bytes */
    uint16 total_len;
    
    if (link == NULL)
    {
        return FALSE;
    }
    
    /* Create Routing Activation Request */
    total_len = DoIP_CreateRoutingActivationRequest(buffer, link->logical_address);
    
    if (total_len == 0)
    {
        sprintf(msg, "[DoIP Link] Failed to create RA request\r\n");
        sendUARTMessage(msg, strlen(msg));
        return FALSE;
    }
    
    sprintf(msg, "[DoIP Link] Sending RA request...\r\n");
    sendUARTMessage(msg, strlen(msg));
    
    return DoIP_Link_Send(link, buffer, total_len);
}

boolean DoIP_Link_SendRoutingActivationResponse(DoIPLink *link, uint8 response_code)
{
    char msg[64];
    uint8 buffer[DOIP_HEADER_SIZE + 13];  /* RoutingActivation response = 13 bytes */
    uint8 *payload;
    uint32 payload_len;
    
    if (link == NULL)
    {
        return FALSE;
    }
    
    /* Build DoIP header */
    payload_len = 13;
    DoIP_CreateHeader(buffer, DOIP_ROUTING_ACTIVATION_RES, payload_len);
    
    /* Build payload */
    payload = buffer + DOIP_HEADER_SIZE;
    payload[0] = (link->logical_address >> 8) & 0xFF;  /* Tester logical address */
    payload[1] = link->logical_address & 0xFF;
    payload[2] = (link->remote_logical_address >> 8) & 0xFF;  /* ECU logical address */
    payload[3] = link->remote_logical_address & 0xFF;
    payload[4] = response_code;  /* 0x10 = Success */
    payload[5] = 0x00;  /* Reserved */
    payload[6] = 0x00;
    payload[7] = 0x00;
    payload[8] = 0x00;
    /* OEM specific (4 bytes) */
    payload[9] = 0x00;
    payload[10] = 0x00;
    payload[11] = 0x00;
    payload[12] = 0x00;
    
    if (response_code == 0x10)
    {
        link->routing_activated = TRUE;
        link->state = DOIP_LINK_STATE_AUTHENTICATED;
    }
    
    sprintf(msg, "[DoIP Link] Sending RA response: 0x%02X\r\n", response_code);
    sendUARTMessage(msg, strlen(msg));
    
    return DoIP_Link_Send(link, buffer, DOIP_HEADER_SIZE + 13);
}

boolean DoIP_Link_SendDiagnosticMessage(DoIPLink *link, uint16 target_addr, 
                                       const uint8 *uds_data, uint16 uds_len)
{
    char msg[64];
    uint8 buffer[DOIP_MAX_MESSAGE_SIZE];
    uint8 *payload;
    uint32 payload_len;
    
    if (link == NULL || uds_data == NULL || uds_len == 0)
    {
        return FALSE;
    }
    
    if (uds_len > (DOIP_MAX_MESSAGE_SIZE - DOIP_HEADER_SIZE - 4))
    {
        sprintf(msg, "[DoIP Link] UDS data too large\r\n");
        sendUARTMessage(msg, strlen(msg));
        return FALSE;
    }
    
    /* Build DoIP header */
    payload_len = 4 + uds_len;  /* SA(2) + TA(2) + UDS */
    DoIP_CreateHeader(buffer, DOIP_DIAGNOSTIC_MESSAGE, payload_len);
    
    /* Build payload */
    payload = buffer + DOIP_HEADER_SIZE;
    payload[0] = (link->logical_address >> 8) & 0xFF;  /* Source Address */
    payload[1] = link->logical_address & 0xFF;
    payload[2] = (target_addr >> 8) & 0xFF;  /* Target Address */
    payload[3] = target_addr & 0xFF;
    
    /* Copy UDS data */
    memcpy(&payload[4], uds_data, uds_len);
    
    return DoIP_Link_Send(link, buffer, DOIP_HEADER_SIZE + payload_len);
}

/*******************************************************************************
 * Close Link
 ******************************************************************************/

void DoIP_Link_Close(DoIPLink *link)
{
    char msg[32];
    
    if (link == NULL)
    {
        return;
    }
    
    if (link->conn_pcb != NULL)
    {
        tcp_close(link->conn_pcb);
        link->conn_pcb = NULL;
    }
    
    if (link->listen_pcb != NULL && link->role == DOIP_ROLE_SERVER)
    {
        tcp_close(link->listen_pcb);
        link->listen_pcb = NULL;
    }
    
    link->state = DOIP_LINK_STATE_IDLE;
    link->routing_activated = FALSE;
    
    sprintf(msg, "[DoIP Link] Closed\r\n");
    sendUARTMessage(msg, strlen(msg));
}

/*******************************************************************************
 * Query Functions
 ******************************************************************************/

DoIPLinkState DoIP_Link_GetState(const DoIPLink *link)
{
    if (link == NULL)
    {
        return DOIP_LINK_STATE_IDLE;
    }
    
    return link->state;
}

boolean DoIP_Link_IsConnected(const DoIPLink *link)
{
    if (link == NULL)
    {
        return FALSE;
    }
    
    return (link->state == DOIP_LINK_STATE_CONNECTED || 
            link->state == DOIP_LINK_STATE_AUTHENTICATED);
}

boolean DoIP_Link_IsAuthenticated(const DoIPLink *link)
{
    if (link == NULL)
    {
        return FALSE;
    }
    
    return (link->state == DOIP_LINK_STATE_AUTHENTICATED && 
            link->routing_activated);
}
