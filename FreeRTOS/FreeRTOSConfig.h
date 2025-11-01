/**
 * @file FreeRTOSConfig.h
 * @brief FreeRTOS Configuration for TC375 Zonal Gateway
 * 
 * This file contains all necessary configuration for FreeRTOS running on
 * AURIX TC375 Lite Kit.
 * 
 * Target: TC375 (TriCore 1.6.2 architecture)
 * Clock: 300 MHz (typical for TC375)
 * RAM: 512 KB
 * Flash: 6 MB
 */

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/******************************************************************************/
/*                          TC375 Hardware Configuration                      */
/******************************************************************************/

/* System clock configuration */
#define configCPU_CLOCK_HZ                      ( ( unsigned long ) 300000000 )  /* 300 MHz */
#define configPERIPHERAL_CLOCK_HZ               ( ( unsigned long ) 100000000 )  /* 100 MHz (STM) */

/******************************************************************************/
/*                          Kernel Configuration                              */
/******************************************************************************/

/* Scheduler configuration */
#define configUSE_PREEMPTION                    1       /* Preemptive scheduling */
#define configUSE_TIME_SLICING                  1       /* Time slicing for equal priority tasks */
#define configUSE_PORT_OPTIMISED_TASK_SELECTION 0       /* Generic selection (TriCore doesn't need this) */
#define configUSE_TICKLESS_IDLE                 0       /* No tickless idle (automotive needs determinism) */
#define configTICK_RATE_HZ                      ( ( TickType_t ) 1000 )  /* 1000 Hz = 1ms tick */

/* Task configuration */
#define configMAX_PRIORITIES                    16      /* 16 priority levels (0-15) */
#define configMINIMAL_STACK_SIZE                ( ( unsigned short ) 128 )  /* 128 words = 512 bytes */
#define configMAX_TASK_NAME_LEN                 16      /* Task name length */
#define configUSE_16_BIT_TICKS                  0       /* Use 32-bit tick counter */
#define configIDLE_SHOULD_YIELD                 1       /* Idle task yields to user tasks */

/* Memory configuration */
#define configTOTAL_HEAP_SIZE                   ( ( size_t ) ( 50 * 1024 ) )  /* 50 KB heap */
#define configSUPPORT_STATIC_ALLOCATION         0       /* Disable static allocation (use dynamic) */
#define configSUPPORT_DYNAMIC_ALLOCATION        1       /* Enable dynamic allocation */
#define configAPPLICATION_ALLOCATED_HEAP        0       /* FreeRTOS manages heap */

/******************************************************************************/
/*                          Hook Functions                                    */
/******************************************************************************/

#define configUSE_IDLE_HOOK                     0       /* No idle hook (can enable for debugging) */
#define configUSE_TICK_HOOK                     0       /* No tick hook */
#define configUSE_MALLOC_FAILED_HOOK            1       /* Enable malloc failure hook (important!) */
#define configUSE_DAEMON_TASK_STARTUP_HOOK      0       /* No daemon startup hook */
#define configCHECK_FOR_STACK_OVERFLOW          0       /* Cannot use stack overflow check on TriCore (CSA) */

/******************************************************************************/
/*                          Co-routine Configuration                          */
/******************************************************************************/

#define configUSE_CO_ROUTINES                   0       /* Disable co-routines (not needed) */
#define configMAX_CO_ROUTINE_PRIORITIES         2       /* Not used */

/******************************************************************************/
/*                          Software Timer Configuration                      */
/******************************************************************************/

#define configUSE_TIMERS                        1       /* Enable software timers */
#define configTIMER_TASK_PRIORITY               ( configMAX_PRIORITIES - 1 )  /* High priority for timer task */
#define configTIMER_QUEUE_LENGTH                10      /* Timer command queue length */
#define configTIMER_TASK_STACK_DEPTH            ( configMINIMAL_STACK_SIZE * 2 )  /* 1024 bytes */

/******************************************************************************/
/*                          API Function Configuration                        */
/******************************************************************************/

/* Task control API */
#define INCLUDE_vTaskPrioritySet                1
#define INCLUDE_uxTaskPriorityGet               1
#define INCLUDE_vTaskDelete                     1
#define INCLUDE_vTaskSuspend                    1
#define INCLUDE_vTaskDelayUntil                 1
#define INCLUDE_vTaskDelay                      1
#define INCLUDE_xTaskGetSchedulerState          1
#define INCLUDE_xTaskGetCurrentTaskHandle       1
#define INCLUDE_uxTaskGetStackHighWaterMark     1
#define INCLUDE_xTaskGetIdleTaskHandle          0
#define INCLUDE_eTaskGetState                   1
#define INCLUDE_xTimerPendFunctionCall          1
#define INCLUDE_xTaskAbortDelay                 0
#define INCLUDE_xTaskGetHandle                  0
#define INCLUDE_xSemaphoreGetMutexHolder        0

/******************************************************************************/
/*                          Queue Configuration                               */
/******************************************************************************/

#define configUSE_QUEUE_SETS                    1       /* Enable queue sets */
#define configQUEUE_REGISTRY_SIZE               10      /* Queue registry size (for debugging) */
#define configUSE_MUTEXES                       1       /* Enable mutexes */
#define configUSE_RECURSIVE_MUTEXES             1       /* Enable recursive mutexes */
#define configUSE_COUNTING_SEMAPHORES           1       /* Enable counting semaphores */

/******************************************************************************/
/*                          Debug and Trace Configuration                     */
/******************************************************************************/

#define configUSE_TRACE_FACILITY                1       /* Enable trace facility */
#define configUSE_STATS_FORMATTING_FUNCTIONS    1       /* Enable task stats formatting */
#define configGENERATE_RUN_TIME_STATS           0       /* Disable runtime stats (can enable later) */

/* Assertions (enable for development, disable for production) */
#define configASSERT( x )                       if( ( x ) == 0 ) { taskDISABLE_INTERRUPTS(); for( ;; ); }

/******************************************************************************/
/*                          TriCore-Specific Configuration                    */
/******************************************************************************/

/* Interrupt configuration */
#define configINTERRUPT_PRIORITY_MAX            1       /* Highest interrupt priority (1-255 for TriCore) */
#define configMAX_SYSCALL_INTERRUPT_PRIORITY    10      /* FreeRTOS API calls allowed above this priority */
#define configKERNEL_INTERRUPT_PRIORITY         20      /* Kernel interrupt priority (lower than max syscall) */

/* TriCore Context Save Area (CSA) */
/* Note: TriCore uses CSA for context switching, not traditional stack */
/* Ensure sufficient CSA is allocated in linker script */

/******************************************************************************/
/*                          lwIP Integration                                  */
/******************************************************************************/

/* Thread safety for lwIP */
#define configUSE_NEWLIB_REENTRANT              0       /* Not using newlib */

/******************************************************************************/
/*                          Memory Protection Unit (MPU)                      */
/******************************************************************************/

#define configENABLE_MPU                        0       /* Disable MPU (can enable for security) */
#define configENABLE_FPU                        1       /* Enable FPU (TC375 has FPU) */
#define configENABLE_TRUSTZONE                  0       /* No TrustZone on TriCore */

/******************************************************************************/
/*                          Application-Specific Configuration                */
/******************************************************************************/

/* Task priorities (application-defined) */
#define PRIORITY_IDLE                           0                               /* Idle task (automatic) */
#define PRIORITY_LOW                            ( tskIDLE_PRIORITY + 1 )        /* Low priority tasks */
#define PRIORITY_NORMAL                         ( tskIDLE_PRIORITY + 5 )        /* Normal priority */
#define PRIORITY_HIGH                           ( tskIDLE_PRIORITY + 10 )       /* High priority tasks */
#define PRIORITY_REALTIME                       ( configMAX_PRIORITIES - 2 )    /* Real-time tasks */

/* Zonal Gateway specific tasks */
#define TASK_LWIP_STACK_SIZE                    ( 1024 )    /* lwIP stack task: 4KB */
#define TASK_LWIP_PRIORITY                      ( PRIORITY_HIGH )

#define TASK_DOIP_SERVER_STACK_SIZE             ( 512 )     /* DoIP server task: 2KB */
#define TASK_DOIP_SERVER_PRIORITY               ( PRIORITY_NORMAL )

#define TASK_JSON_SERVER_STACK_SIZE             ( 512 )     /* JSON server task: 2KB */
#define TASK_JSON_SERVER_PRIORITY               ( PRIORITY_NORMAL )

#define TASK_OTA_MANAGER_STACK_SIZE             ( 512 )     /* OTA manager task: 2KB */
#define TASK_OTA_MANAGER_PRIORITY               ( PRIORITY_NORMAL )

#define TASK_HEARTBEAT_STACK_SIZE               ( 256 )     /* Heartbeat task: 1KB */
#define TASK_HEARTBEAT_PRIORITY                 ( PRIORITY_LOW )

#define TASK_LED_BLINK_STACK_SIZE               ( 128 )     /* LED blink task: 512B */
#define TASK_LED_BLINK_PRIORITY                 ( PRIORITY_LOW )

/******************************************************************************/
/*                          Runtime Checks                                    */
/******************************************************************************/

/* Compile-time checks */
#if configUSE_PREEMPTION == 0
    #error "Preemptive scheduling is required for automotive applications"
#endif

#if configTICK_RATE_HZ != 1000
    #warning "Tick rate should be 1000 Hz (1ms) for deterministic timing"
#endif

#if configTOTAL_HEAP_SIZE > (50 * 1024)
    #warning "Heap size should not exceed 50KB to leave room for stacks and CSA"
#endif

/******************************************************************************/
/*                          Optional Features (Future)                        */
/******************************************************************************/

/* Event groups (not currently used) */
#define configUSE_EVENT_GROUPS                  1       /* Enable for future use */

/* Stream buffers (not currently used) */
#define configUSE_STREAM_BUFFERS                1       /* Enable for future use */

/* Message buffers (built on stream buffers) */
#define configUSE_MESSAGE_BUFFERS               1       /* Enable for future use */

#endif /* FREERTOS_CONFIG_H */

