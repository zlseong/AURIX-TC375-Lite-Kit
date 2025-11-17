/**
 * @file SystemMain.c
 * @brief System Main Loop Manager Implementation
 */

#include "SystemMain.h"
#include "Ifx_Lwip.h"
#include "Libraries/DoIP/doip_client.h"
#include "Libraries/DoIP/doip_link.h"
#include "Libraries/DataCollection/vci_aggregator.h"
#include "Libraries/DataCollection/readiness_aggregator.h"
#include "Libraries/Flash/FlashBankManager.h"
#include "IfxStm.h"

/* SW Synchronization state */
static boolean g_syncRequested = FALSE;
static boolean g_syncCompleted = FALSE;
static uint32 g_bootTimestamp = 0;

/* Delay SW Sync by 60 seconds after boot for system stability */
/* DISABLED: SW Sync takes too long (5+ minutes), blocking all other functions */
/* TODO: Implement incremental/chunked synchronization for production */
#define SW_SYNC_DELAY_MS    60000  /* 60 seconds */
#define SW_SYNC_ENABLED     0      /* 0 = Disabled, 1 = Enabled */

void SystemMain_Loop(void)
{
    /* Record boot timestamp for SW Sync delay */
    g_bootTimestamp = IfxStm_getLower(&MODULE_STM0);
    
    /* Suppress unused variable warnings when SW_SYNC_ENABLED = 0 */
    (void)g_syncRequested;
    (void)g_syncCompleted;
    
    while (1)
    {
        /* Network and DoIP polling */
        Ifx_Lwip_pollTimerFlags();
        Ifx_Lwip_pollReceiveFlags();
        DoIP_Client_Poll();
        
        /* VCI and Readiness aggregator polling */
        VCI_Aggregator_Poll();
        /* Readiness polling done automatically via UDS Client callback */
        
        /* SW Synchronization Background Task (DISABLED) */
#if SW_SYNC_ENABLED
        if (!g_syncCompleted && !g_syncRequested)
        {
            uint32 currentTime = IfxStm_getLower(&MODULE_STM0);
            uint32 elapsedMs = IfxStm_getTicksFromMilliseconds(&MODULE_STM0, 
                (currentTime - g_bootTimestamp) / IfxStm_getTicksFromMilliseconds(&MODULE_STM0, 1));
            
            /* Check if delay period has passed */
            if (elapsedMs >= SW_SYNC_DELAY_MS)
            {
                FlashBankStatus_t status = FlashBank_GetStatusFlags();
                
                /* Only sync if banks are not identical */
                if (status.bits.banksIdentical == 0)
                {
                    g_syncRequested = TRUE;
                    
                    /* Execute SW Synchronization */
                    if (FlashBank_SWSynchronization())
                    {
                        g_syncCompleted = TRUE;
                    }
                    else
                    {
                        /* Retry after 60 seconds on failure */
                        g_syncRequested = FALSE;
                        g_bootTimestamp = IfxStm_getLower(&MODULE_STM0);
                    }
                }
                else
                {
                    /* Already synchronized */
                    g_syncCompleted = TRUE;
                }
            }
        }
#endif
    }
}

