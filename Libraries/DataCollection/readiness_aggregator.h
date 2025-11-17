/**
 * @file readiness_aggregator.h
 * @brief Readiness Aggregator - Orchestrates readiness checks from Zone ECUs using UDS Client
 * @details Manages readiness collection by sending UDS 0x31 RoutineControl requests via DoIP
 */

#ifndef READINESS_AGGREGATOR_H
#define READINESS_AGGREGATOR_H

#include "Ifx_Types.h"
#include "doip_types.h"

/*******************************************************************************
 * Constants
 ******************************************************************************/

#define READINESS_COLLECTION_TIMEOUT  5000  /* 5 seconds */

/*******************************************************************************
 * Readiness Information Structure
 ******************************************************************************/

typedef struct
{
    char ecu_id[16];            /* ECU ID */
    uint8 battery_soc;          /* Battery State of Charge (%) */
    uint8 temperature;          /* Temperature (°C + 40 offset) */
    uint8 engine_state;         /* 0: Off, 1: On */
    uint8 parking_brake;        /* 0: Released, 1: Applied */
    uint32 free_space_kb;       /* Free storage space (KB) */
} Readiness_Info;

/*******************************************************************************
 * Function Prototypes
 ******************************************************************************/

/**
 * @brief Initialize Readiness Aggregator
 * @return TRUE if successful, FALSE otherwise
 */
boolean Readiness_Aggregator_Init(void);

/**
 * @brief Start readiness collection from all Zone ECUs
 * @return TRUE if collection started successfully, FALSE otherwise
 */
boolean Readiness_Aggregator_Start(void);

/**
 * @brief Get collected readiness information
 * @param info_array Output array for readiness info
 * @param max_count Maximum number of entries
 * @return Number of collected readiness entries
 */
uint8 Readiness_Aggregator_GetResults(Readiness_Info *info_array, uint8 max_count);

/**
 * @brief Clear collected readiness data
 */
void Readiness_Aggregator_Clear(void);

/**
 * @brief Check if collection is in progress
 * @return TRUE if collecting, FALSE otherwise
 */
boolean Readiness_Aggregator_IsActive(void);

#endif /* READINESS_AGGREGATOR_H */

