/**
 * @file sys_arch.c
 * @brief lwIP System Architecture for FreeRTOS - Implementation
 * 
 * This file implements OS abstraction layer for lwIP using FreeRTOS.
 */

#include "sys_arch.h"
#include "lwip/sys.h"
#include "lwip/opt.h"
#include "lwip/stats.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

/******************************************************************************/
/*                          Time Functions                                    */
/******************************************************************************/

/**
 * @brief Get current time in milliseconds
 * 
 * @return Current time in ms (wraps around every ~49 days)
 */
u32_t sys_now(void)
{
    return (u32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
}

/******************************************************************************/
/*                          Semaphore Functions                               */
/******************************************************************************/

/**
 * @brief Create a new semaphore
 * 
 * @param sem Pointer to store semaphore handle
 * @param count Initial count
 * @return ERR_OK on success
 */
err_t sys_sem_new(sys_sem_t *sem, u8_t count)
{
    *sem = xSemaphoreCreateBinary();
    
    if(*sem == NULL)
    {
#if SYS_STATS
        SYS_STATS_INC(sem.err);
#endif
        return ERR_MEM;
    }
    
    /* If count > 0, give semaphore initially */
    if(count > 0)
    {
        xSemaphoreGive(*sem);
    }
    
#if SYS_STATS
    SYS_STATS_INC_USED(sem);
#endif
    
    return ERR_OK;
}

/**
 * @brief Wait for semaphore with timeout
 * 
 * @param sem Semaphore handle
 * @param timeout Timeout in milliseconds (0 = wait forever)
 * @return Time waited in ms, or SYS_ARCH_TIMEOUT on timeout
 */
u32_t sys_arch_sem_wait(sys_sem_t *sem, u32_t timeout)
{
    TickType_t startTime, endTime, elapsed;
    TickType_t timeoutTicks;
    
    startTime = xTaskGetTickCount();
    
    if(timeout == 0)
    {
        /* Wait forever */
        timeoutTicks = portMAX_DELAY;
    }
    else
    {
        /* Convert milliseconds to ticks */
        timeoutTicks = timeout / portTICK_PERIOD_MS;
        if(timeoutTicks == 0)
        {
            timeoutTicks = 1;
        }
    }
    
    /* Wait for semaphore */
    if(xSemaphoreTake(*sem, timeoutTicks) == pdTRUE)
    {
        endTime = xTaskGetTickCount();
        elapsed = (endTime - startTime) * portTICK_PERIOD_MS;
        return (u32_t)elapsed;
    }
    else
    {
        /* Timeout */
        return SYS_ARCH_TIMEOUT;
    }
}

/**
 * @brief Signal semaphore
 * 
 * @param sem Semaphore handle
 */
void sys_sem_signal(sys_sem_t *sem)
{
    xSemaphoreGive(*sem);
}

/**
 * @brief Delete semaphore
 * 
 * @param sem Semaphore handle
 */
void sys_sem_free(sys_sem_t *sem)
{
    if(*sem != SYS_SEM_NULL)
    {
#if SYS_STATS
        SYS_STATS_DEC(sem.used);
#endif
        vSemaphoreDelete(*sem);
        *sem = SYS_SEM_NULL;
    }
}

/******************************************************************************/
/*                          Mutex Functions                                   */
/******************************************************************************/

/**
 * @brief Create a new mutex
 * 
 * @param mutex Pointer to store mutex handle
 * @return ERR_OK on success
 */
err_t sys_mutex_new(sys_mutex_t *mutex)
{
    *mutex = xSemaphoreCreateMutex();
    
    if(*mutex == NULL)
    {
#if SYS_STATS
        SYS_STATS_INC(mutex.err);
#endif
        return ERR_MEM;
    }
    
#if SYS_STATS
    SYS_STATS_INC_USED(mutex);
#endif
    
    return ERR_OK;
}

/**
 * @brief Lock mutex
 * 
 * @param mutex Mutex handle
 */
void sys_mutex_lock(sys_mutex_t *mutex)
{
    xSemaphoreTake(*mutex, portMAX_DELAY);
}

/**
 * @brief Unlock mutex
 * 
 * @param mutex Mutex handle
 */
void sys_mutex_unlock(sys_mutex_t *mutex)
{
    xSemaphoreGive(*mutex);
}

/**
 * @brief Delete mutex
 * 
 * @param mutex Mutex handle
 */
void sys_mutex_free(sys_mutex_t *mutex)
{
    if(*mutex != SYS_MUTEX_NULL)
    {
#if SYS_STATS
        SYS_STATS_DEC(mutex.used);
#endif
        vSemaphoreDelete(*mutex);
        *mutex = SYS_MUTEX_NULL;
    }
}

/******************************************************************************/
/*                          Mailbox Functions                                 */
/******************************************************************************/

/**
 * @brief Create a new mailbox (message queue)
 * 
 * @param mbox Pointer to store mailbox handle
 * @param size Maximum number of messages
 * @return ERR_OK on success
 */
err_t sys_mbox_new(sys_mbox_t *mbox, int size)
{
    if(size == 0)
    {
        size = 16;  /* Default size */
    }
    
    *mbox = xQueueCreate(size, sizeof(void *));
    
    if(*mbox == NULL)
    {
#if SYS_STATS
        SYS_STATS_INC(mbox.err);
#endif
        return ERR_MEM;
    }
    
#if SYS_STATS
    SYS_STATS_INC_USED(mbox);
#endif
    
    return ERR_OK;
}

/**
 * @brief Post a message to mailbox
 * 
 * @param mbox Mailbox handle
 * @param msg Message pointer
 */
void sys_mbox_post(sys_mbox_t *mbox, void *msg)
{
    while(xQueueSend(*mbox, &msg, portMAX_DELAY) != pdTRUE)
    {
        /* Keep trying (should not happen with portMAX_DELAY) */
    }
}

/**
 * @brief Try to post a message to mailbox (non-blocking)
 * 
 * @param mbox Mailbox handle
 * @param msg Message pointer
 * @return ERR_OK on success, ERR_MEM if mailbox full
 */
err_t sys_mbox_trypost(sys_mbox_t *mbox, void *msg)
{
    if(xQueueSend(*mbox, &msg, 0) == pdTRUE)
    {
        return ERR_OK;
    }
    else
    {
#if SYS_STATS
        SYS_STATS_INC(mbox.err);
#endif
        return ERR_MEM;
    }
}

/**
 * @brief Wait for a message from mailbox with timeout
 * 
 * @param mbox Mailbox handle
 * @param msg Pointer to store received message
 * @param timeout Timeout in milliseconds (0 = wait forever)
 * @return Time waited in ms, or SYS_ARCH_TIMEOUT on timeout
 */
u32_t sys_arch_mbox_fetch(sys_mbox_t *mbox, void **msg, u32_t timeout)
{
    TickType_t startTime, endTime, elapsed;
    TickType_t timeoutTicks;
    void *tempMsg;
    
    startTime = xTaskGetTickCount();
    
    if(timeout == 0)
    {
        timeoutTicks = portMAX_DELAY;
    }
    else
    {
        timeoutTicks = timeout / portTICK_PERIOD_MS;
        if(timeoutTicks == 0)
        {
            timeoutTicks = 1;
        }
    }
    
    /* Wait for message */
    if(xQueueReceive(*mbox, &tempMsg, timeoutTicks) == pdTRUE)
    {
        if(msg != NULL)
        {
            *msg = tempMsg;
        }
        
        endTime = xTaskGetTickCount();
        elapsed = (endTime - startTime) * portTICK_PERIOD_MS;
        return (u32_t)elapsed;
    }
    else
    {
        /* Timeout */
        if(msg != NULL)
        {
            *msg = NULL;
        }
        return SYS_ARCH_TIMEOUT;
    }
}

/**
 * @brief Try to fetch a message from mailbox (non-blocking)
 * 
 * @param mbox Mailbox handle
 * @param msg Pointer to store received message
 * @return Time waited (0 if immediate), or SYS_MBOX_EMPTY if no message
 */
u32_t sys_arch_mbox_tryfetch(sys_mbox_t *mbox, void **msg)
{
    void *tempMsg;
    
    if(xQueueReceive(*mbox, &tempMsg, 0) == pdTRUE)
    {
        if(msg != NULL)
        {
            *msg = tempMsg;
        }
        return 0;
    }
    else
    {
        return SYS_MBOX_EMPTY;
    }
}

/**
 * @brief Delete mailbox
 * 
 * @param mbox Mailbox handle
 */
void sys_mbox_free(sys_mbox_t *mbox)
{
    if(*mbox != SYS_MBOX_NULL)
    {
#if SYS_STATS
        SYS_STATS_DEC(mbox.used);
#endif
        vQueueDelete(*mbox);
        *mbox = SYS_MBOX_NULL;
    }
}

/******************************************************************************/
/*                          Thread Functions                                  */
/******************************************************************************/

/**
 * @brief Create a new thread
 * 
 * @param name Thread name
 * @param thread Thread function
 * @param arg Argument to pass to thread function
 * @param stacksize Stack size in bytes
 * @param prio Thread priority
 * @return Thread handle
 */
sys_thread_t sys_thread_new(const char *name, lwip_thread_fn thread, void *arg, 
                            int stacksize, int prio)
{
    TaskHandle_t taskHandle;
    
    /* Convert stacksize from bytes to words (TC375: 32-bit = 4 bytes) */
    UBaseType_t stackWords = stacksize / sizeof(StackType_t);
    
    if(xTaskCreate(thread, name, stackWords, arg, prio, &taskHandle) != pdPASS)
    {
        return SYS_THREAD_NULL;
    }
    
    return taskHandle;
}

/******************************************************************************/
/*                          Protection (Critical Section)                     */
/******************************************************************************/

/**
 * @brief Enter critical section
 * 
 * @return Protection state (to be restored later)
 */
sys_prot_t sys_arch_protect(void)
{
    taskENTER_CRITICAL();
    return 1;  /* Dummy return value (FreeRTOS doesn't need it) */
}

/**
 * @brief Exit critical section
 * 
 * @param pval Protection state from sys_arch_protect()
 */
void sys_arch_unprotect(sys_prot_t pval)
{
    (void)pval;  /* Unused */
    taskEXIT_CRITICAL();
}

/******************************************************************************/
/*                          Initialization                                    */
/******************************************************************************/

/**
 * @brief Initialize sys_arch layer
 * 
 * Called once at startup before any other sys_arch functions.
 */
void sys_init(void)
{
    /* Nothing to initialize for FreeRTOS-based sys_arch */
    /* FreeRTOS scheduler should already be started */
}

