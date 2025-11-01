/**
 * @file sys_arch.h
 * @brief lwIP System Architecture for FreeRTOS on TC375
 * 
 * This file provides OS abstraction layer for lwIP to work with FreeRTOS.
 */

#ifndef LWIP_ARCH_SYS_ARCH_H
#define LWIP_ARCH_SYS_ARCH_H

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

/******************************************************************************/
/*                          Type Definitions                                  */
/******************************************************************************/

/* Semaphore type (maps to FreeRTOS semaphore) */
typedef SemaphoreHandle_t sys_sem_t;

/* Mutex type (maps to FreeRTOS mutex) */
typedef SemaphoreHandle_t sys_mutex_t;

/* Mailbox type (maps to FreeRTOS queue) */
typedef QueueHandle_t sys_mbox_t;

/* Thread type (maps to FreeRTOS task) */
typedef TaskHandle_t sys_thread_t;

/******************************************************************************/
/*                          Macros                                            */
/******************************************************************************/

/* Invalid handle values */
#define SYS_SEM_NULL        NULL
#define SYS_MUTEX_NULL      NULL
#define SYS_MBOX_NULL       NULL
#define SYS_THREAD_NULL     NULL

/* Default stack size for lwIP threads (in words, not bytes) */
#define LWIP_TASK_STACK_SIZE    1024

/* Priority definitions (adjust based on application needs) */
#define LWIP_TASK_PRIORITY_HIGH     (configMAX_PRIORITIES - 2)
#define LWIP_TASK_PRIORITY_MID      (configMAX_PRIORITIES / 2)
#define LWIP_TASK_PRIORITY_LOW      (tskIDLE_PRIORITY + 1)

#endif /* LWIP_ARCH_SYS_ARCH_H */

