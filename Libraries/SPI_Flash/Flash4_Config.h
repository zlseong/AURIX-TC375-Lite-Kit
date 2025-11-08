/**********************************************************************************************************************
 * \file Flash4_Config.h
 * \copyright Copyright (C) Infineon Technologies AG 2019
 * 
 * Configuration file for Flash4 driver
 * Adjust these parameters according to your hardware setup and requirements
 *********************************************************************************************************************/

#ifndef FLASH4_CONFIG_H_
#define FLASH4_CONFIG_H_

/*********************************************************************************************************************/
/*----------------------------------Hardware Configuration-----------------------------------------------------------*/
/*********************************************************************************************************************/

/* QSPI Module Selection */
#define FLASH4_QSPI_MODULE              &MODULE_QSPI2        /* QSPI module to use (mikroBUS) */

/* Baudrate Configuration */
#define FLASH4_QSPI_MAX_BAUDRATE        25000000UL   /* Max baudrate: 25 MHz (실제 안정 최대값) */
#define FLASH4_QSPI_BAUDRATE            25000000.0f  /* Channel baudrate: 25 MHz */
#define FLASH4_BAUDRATE                 25000000     /* Baudrate macro: 25 MHz */

/* Interrupt Priorities (0-255, lower number = higher priority) */
#define ISR_PRIORITY_FLASH4_TX          10          /* Transmit interrupt priority */
#define ISR_PRIORITY_FLASH4_RX          11          /* Receive interrupt priority */
#define ISR_PRIORITY_FLASH4_ER          12          /* Error interrupt priority */

/* Interrupt priority defines for ISR macros */
#define IFX_INTPRIO_QSPI2_TX            ISR_PRIORITY_FLASH4_TX
#define IFX_INTPRIO_QSPI2_RX            ISR_PRIORITY_FLASH4_RX
#define IFX_INTPRIO_QSPI2_ER            ISR_PRIORITY_FLASH4_ER

#endif /* FLASH4_CONFIG_H_ */
