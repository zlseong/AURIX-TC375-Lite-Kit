/**
 * @file tc375_eth.h
 * @brief TC375 Ethernet Driver for lwIP
 * 
 * This file provides the network interface driver that connects
 * TC375 GETH hardware with lwIP stack.
 * 
 * Features:
 * - GETH (Gigabit Ethernet) hardware initialization
 * - PHY configuration (TLK110 or similar)
 * - DMA setup for packet TX/RX
 * - lwIP netif integration
 */

#ifndef TC375_ETH_H
#define TC375_ETH_H

#include "Ifx_Types.h"
#include "lwip/err.h"
#include "lwip/netif.h"

/******************************************************************************/
/*                          Configuration                                     */
/******************************************************************************/

/* Network Configuration */
#define TC375_ETH_MAC_ADDR0             0x02    /* MAC Address byte 0 (locally administered) */
#define TC375_ETH_MAC_ADDR1             0x00
#define TC375_ETH_MAC_ADDR2             0x00
#define TC375_ETH_MAC_ADDR3             0x00
#define TC375_ETH_MAC_ADDR4             0x00
#define TC375_ETH_MAC_ADDR5             0x01    /* MAC: 02:00:00:00:00:01 */

/* DMA Configuration */
#define TC375_ETH_RX_BUFFERS            8       /* Number of RX DMA buffers */
#define TC375_ETH_TX_BUFFERS            4       /* Number of TX DMA buffers */
#define TC375_ETH_BUFFER_SIZE           1536    /* Size of each buffer (MTU + headers) */

/* PHY Configuration */
#define TC375_ETH_PHY_ADDR              0       /* PHY address on MDIO bus */
#define TC375_ETH_PHY_TIMEOUT_MS        5000    /* PHY initialization timeout */

/******************************************************************************/
/*                          Data Structures                                   */
/******************************************************************************/

/**
 * @brief Ethernet driver handle
 * 
 * Contains hardware state, DMA descriptors, and lwIP netif pointer.
 */
typedef struct
{
    struct netif *netif;                        /**< lwIP network interface */
    
    /* Hardware state */
    boolean initialized;                        /**< Initialization status */
    boolean linkUp;                             /**< Link status (up/down) */
    uint32 speed;                               /**< Link speed (10/100/1000 Mbps) */
    boolean fullDuplex;                         /**< Duplex mode */
    
    /* MAC address */
    uint8 macAddr[6];                           /**< MAC address */
    
    /* Statistics */
    uint32 txPackets;                           /**< Transmitted packets */
    uint32 rxPackets;                           /**< Received packets */
    uint32 txErrors;                            /**< Transmit errors */
    uint32 rxErrors;                            /**< Receive errors */
    
} tc375_eth_t;

/******************************************************************************/
/*                          Function Prototypes                               */
/******************************************************************************/

/**
 * @brief Initialize TC375 Ethernet driver
 * 
 * Initializes GETH hardware, PHY, DMA, and registers with lwIP.
 * 
 * @param netif lwIP network interface structure
 * @return ERR_OK on success, error code otherwise
 */
err_t tc375_eth_init(struct netif *netif);

/**
 * @brief Set up network interface (called by lwIP)
 * 
 * @param netif lwIP network interface structure
 * @return ERR_OK on success, error code otherwise
 */
err_t tc375_eth_netif_init(struct netif *netif);

/**
 * @brief Send a packet (called by lwIP)
 * 
 * @param netif lwIP network interface structure
 * @param p pbuf chain to transmit
 * @return ERR_OK on success, error code otherwise
 */
err_t tc375_eth_output(struct netif *netif, struct pbuf *p);

/**
 * @brief Receive packets and pass to lwIP (called periodically or from IRQ)
 * 
 * @param netif lwIP network interface structure
 */
void tc375_eth_input(struct netif *netif);

/**
 * @brief Check link status and update netif
 * 
 * Should be called periodically to detect link up/down events.
 * 
 * @param netif lwIP network interface structure
 */
void tc375_eth_check_link(struct netif *netif);

/**
 * @brief Get driver handle
 * 
 * @return Pointer to driver handle
 */
tc375_eth_t* tc375_eth_get_handle(void);

/**
 * @brief Print driver statistics
 * 
 * Prints TX/RX packet counts, errors, and link status.
 */
void tc375_eth_print_stats(void);

/******************************************************************************/
/*                          Helper Functions                                  */
/******************************************************************************/

/**
 * @brief Initialize GETH hardware
 * 
 * Configures GETH module, pins, and clocks.
 * 
 * @return TRUE on success, FALSE otherwise
 */
boolean tc375_eth_hw_init(void);

/**
 * @brief Initialize PHY
 * 
 * Resets PHY, configures auto-negotiation, and waits for link up.
 * 
 * @return TRUE on success, FALSE otherwise
 */
boolean tc375_eth_phy_init(void);

/**
 * @brief Read PHY register
 * 
 * @param phyAddr PHY address on MDIO bus
 * @param regAddr Register address
 * @param value Pointer to store read value
 * @return TRUE on success, FALSE otherwise
 */
boolean tc375_eth_phy_read(uint8 phyAddr, uint8 regAddr, uint16 *value);

/**
 * @brief Write PHY register
 * 
 * @param phyAddr PHY address on MDIO bus
 * @param regAddr Register address
 * @param value Value to write
 * @return TRUE on success, FALSE otherwise
 */
boolean tc375_eth_phy_write(uint8 phyAddr, uint8 regAddr, uint16 value);

#endif /* TC375_ETH_H */

