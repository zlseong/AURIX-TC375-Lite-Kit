/**
 * @file lwipopts.h
 * @brief lwIP Configuration for TC375 Zonal Gateway
 * 
 * This file contains lwIP stack configuration options optimized for:
 * - TC375 Lite Kit (6MB Flash, 512KB RAM)
 * - Zonal Gateway application (DoIP, JSON over TCP)
 * - FreeRTOS integration
 */

#ifndef LWIPOPTS_H
#define LWIPOPTS_H

/******************************************************************************/
/*                          Platform Configuration                            */
/******************************************************************************/

/* Target: AURIX TC375 (TriCore) */
#define LWIP_TRICORE                    1

/* No standard library by default (bare-metal friendly) */
#define NO_SYS                          0       /* 0 = Use FreeRTOS */
#define LWIP_SOCKET                     0       /* 0 = No BSD sockets (use raw API) */
#define LWIP_NETCONN                    1       /* 1 = Use netconn API */

/******************************************************************************/
/*                          Memory Configuration                              */
/******************************************************************************/

/* Memory alignment (TC375: 32-bit aligned) */
#define MEM_ALIGNMENT                   4

/* Heap memory size (bytes) - adjust based on RAM availability */
#define MEM_SIZE                        (16 * 1024)     /* 16KB heap */

/* MEMP (Memory Pool) Configuration */
#define MEMP_NUM_PBUF                   16      /* Number of pbufs */
#define MEMP_NUM_TCP_PCB                8       /* Active TCP connections */
#define MEMP_NUM_TCP_PCB_LISTEN         4       /* Listening TCP sockets */
#define MEMP_NUM_UDP_PCB                4       /* Active UDP sockets */
#define MEMP_NUM_NETBUF                 8       /* Netconn API buffers */
#define MEMP_NUM_NETCONN                8       /* Netconn structures */

/* Pbuf (Packet Buffer) Configuration */
#define PBUF_POOL_SIZE                  16      /* Number of pbuf pool elements */
#define PBUF_POOL_BUFSIZE               1536    /* Size of each pbuf (1536 = Ethernet MTU + headers) */

/******************************************************************************/
/*                          Protocol Configuration                            */
/******************************************************************************/

/* ARP Configuration */
#define LWIP_ARP                        1       /* Enable ARP */
#define ARP_TABLE_SIZE                  10      /* Number of ARP cache entries */
#define ARP_QUEUEING                    1       /* Queue packets while ARP resolves */

/* IP Configuration */
#define LWIP_IPV4                       1       /* Enable IPv4 */
#define LWIP_IPV6                       0       /* Disable IPv6 (not needed for Zonal Gateway) */
#define IP_FORWARD                      0       /* No IP forwarding */
#define IP_REASSEMBLY                   1       /* Enable IP fragment reassembly */
#define IP_FRAG                         1       /* Enable IP fragmentation */

/* ICMP Configuration */
#define LWIP_ICMP                       1       /* Enable ICMP (for ping) */

/* DHCP Configuration */
#define LWIP_DHCP                       0       /* 0 = Static IP (recommended for automotive) */
#define LWIP_AUTOIP                     0       /* Disable AutoIP */

/* UDP Configuration */
#define LWIP_UDP                        1       /* Enable UDP */
#define LWIP_UDPLITE                    0       /* Disable UDP-Lite */

/* TCP Configuration */
#define LWIP_TCP                        1       /* Enable TCP */
#define TCP_MSS                         1460    /* Maximum Segment Size (1460 = 1500 - 40) */
#define TCP_WND                         (4 * TCP_MSS)   /* TCP window size (4 MSS = 5840 bytes) */
#define TCP_SND_BUF                     (4 * TCP_MSS)   /* TCP send buffer */
#define TCP_SND_QUEUELEN                (2 * TCP_SND_BUF / TCP_MSS)  /* TCP send queue length */
#define TCP_QUEUE_OOSEQ                 1       /* Queue out-of-sequence packets */
#define LWIP_TCP_KEEPALIVE              1       /* Enable TCP keepalive */

/* TCP Timeouts (milliseconds) */
#define TCP_TMR_INTERVAL                250     /* TCP timer interval (250ms) */

/******************************************************************************/
/*                          Network Interface Configuration                   */
/******************************************************************************/

/* Ethernet Configuration */
#define LWIP_ETHERNET                   1       /* Enable Ethernet support */
#define ETH_PAD_SIZE                    0       /* No padding (TC375 GETH handles alignment) */

/* Checksum Configuration (offload to hardware if possible) */
#define CHECKSUM_GEN_IP                 1       /* Generate IP checksums in software */
#define CHECKSUM_GEN_UDP                1       /* Generate UDP checksums */
#define CHECKSUM_GEN_TCP                1       /* Generate TCP checksums */
#define CHECKSUM_CHECK_IP               1       /* Check IP checksums */
#define CHECKSUM_CHECK_UDP              1       /* Check UDP checksums */
#define CHECKSUM_CHECK_TCP              1       /* Check TCP checksums */

/* Note: TC375 GETH may support hardware checksum offload - configure later */
/* #define CHECKSUM_GEN_IP              0 */  /* If HW offload enabled */

/******************************************************************************/
/*                          Operating System Configuration                    */
/******************************************************************************/

/* FreeRTOS Integration */
#define LWIP_FREERTOS                   1       /* Use FreeRTOS */

/* Thread Configuration */
#define TCPIP_THREAD_NAME               "tcpip_thread"
#define TCPIP_THREAD_STACKSIZE          1024    /* Stack size in words (4KB for TC375) */
#define TCPIP_THREAD_PRIO               5       /* Priority (adjust based on application) */

#define DEFAULT_THREAD_STACKSIZE        512     /* Stack for other threads */
#define DEFAULT_THREAD_PRIO             1

/* Mailbox Configuration */
#define TCPIP_MBOX_SIZE                 16      /* TCPIP thread mailbox size */
#define DEFAULT_RAW_RECVMBOX_SIZE       8
#define DEFAULT_UDP_RECVMBOX_SIZE       8
#define DEFAULT_TCP_RECVMBOX_SIZE       8
#define DEFAULT_ACCEPTMBOX_SIZE         8

/******************************************************************************/
/*                          Debug Configuration                               */
/******************************************************************************/

/* Debug Options (set to LWIP_DBG_ON for debugging) */
#define LWIP_DEBUG                      1       /* Enable debug output */

#if LWIP_DEBUG
    #define LWIP_DBG_MIN_LEVEL          LWIP_DBG_LEVEL_ALL
    #define LWIP_DBG_TYPES_ON           LWIP_DBG_ON
    
    /* Debug specific modules */
    #define ETHARP_DEBUG                LWIP_DBG_OFF
    #define NETIF_DEBUG                 LWIP_DBG_ON
    #define PBUF_DEBUG                  LWIP_DBG_OFF
    #define IP_DEBUG                    LWIP_DBG_ON
    #define ICMP_DEBUG                  LWIP_DBG_ON
    #define UDP_DEBUG                   LWIP_DBG_OFF
    #define TCP_DEBUG                   LWIP_DBG_OFF
    #define TCP_INPUT_DEBUG             LWIP_DBG_OFF
    #define TCP_OUTPUT_DEBUG            LWIP_DBG_OFF
    #define TCPIP_DEBUG                 LWIP_DBG_ON
#endif

/******************************************************************************/
/*                          Statistics Configuration                          */
/******************************************************************************/

/* Statistics (enable for monitoring) */
#define LWIP_STATS                      1       /* Enable statistics */
#define LWIP_STATS_DISPLAY              1       /* Enable stats display */

#if LWIP_STATS
    #define LINK_STATS                  1
    #define IP_STATS                    1
    #define ICMP_STATS                  1
    #define UDP_STATS                   1
    #define TCP_STATS                   1
    #define MEM_STATS                   1
    #define MEMP_STATS                  1
    #define PBUF_STATS                  1
    #define SYS_STATS                   1
#endif

/******************************************************************************/
/*                          Application-Specific Configuration                */
/******************************************************************************/

/* Zonal Gateway Specific */
#define LWIP_CALLBACK_API               1       /* Enable callback-style API */

/* Enable necessary modules for DoIP/JSON */
#define LWIP_RAW                        1       /* Raw API (for custom protocols) */

/* Disable unnecessary features to save memory */
#define LWIP_NETIF_HOSTNAME             1       /* Enable hostname support */
#define LWIP_NETIF_STATUS_CALLBACK      1       /* Enable netif status callbacks */
#define LWIP_NETIF_LINK_CALLBACK        1       /* Enable link status callbacks */

/* IGMP (multicast) - disabled for now */
#define LWIP_IGMP                       0

/* DNS - disabled (automotive ECUs typically use static config) */
#define LWIP_DNS                        0

/* SNMP - disabled */
#define LWIP_SNMP                       0

/* PPP - disabled */
#define PPP_SUPPORT                     0

/******************************************************************************/
/*                          Performance Tuning                                */
/******************************************************************************/

/* Optimization */
#define LWIP_NOASSERT                   0       /* Enable assertions (disable in production) */
#define LWIP_PERF                       0       /* Disable performance measurement */

/* Timeouts */
#define LWIP_SO_RCVTIMEO                1       /* Enable receive timeout on sockets */
#define LWIP_SO_SNDTIMEO                1       /* Enable send timeout on sockets */

#endif /* LWIPOPTS_H */

