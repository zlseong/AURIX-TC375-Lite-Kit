/**
 * @file vci_aggregator.h
 * @brief VCI Aggregator - Orchestrates VCI collection from Zone ECUs using UDS Client
 * @details Manages VCI collection by sending UDS 0x22 requests to Zone ECUs via DoIP
 */

#ifndef VCI_AGGREGATOR_H
#define VCI_AGGREGATOR_H

#include "Ifx_Types.h"
#include "doip_types.h"

/*******************************************************************************
 * Constants
 ******************************************************************************/

#define VCI_COLLECTION_TIMEOUT  5000  /* 5 seconds */

/*******************************************************************************
 * Function Prototypes
 ******************************************************************************/

/**
 * @brief Initialize VCI Aggregator
 * @return TRUE if successful, FALSE otherwise
 */
boolean VCI_Aggregator_Init(void);

/**
 * @brief Start VCI collection from all Zone ECUs
 * @return TRUE if collection started successfully, FALSE otherwise
 */
boolean VCI_Aggregator_Start(void);

/**
 * @brief Poll VCI aggregator (check timeout)
 * Should be called periodically in main loop
 */
void VCI_Aggregator_Poll(void);

/**
 * @brief Get collected VCI count
 * @return Number of collected VCI entries
 */
uint8 VCI_Aggregator_GetCount(void);

/**
 * @brief Check if collection is complete
 * @return TRUE if collection is complete, FALSE otherwise
 */
boolean VCI_Aggregator_IsComplete(void);

/**
 * @brief Clear collected VCI data
 */
void VCI_Aggregator_Clear(void);

/**
 * @brief Check if collection is in progress
 * @return TRUE if collecting, FALSE otherwise
 */
boolean VCI_Aggregator_IsActive(void);

#endif /* VCI_AGGREGATOR_H */

