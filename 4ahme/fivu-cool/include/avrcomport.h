 /**
 * @file avrcomport.h
 * @author Thomas Jerman
 * @date 24.12.2025
 * @version 2.0 (10.01.2026)
 * @brief UART-backed CLI transport implementation for AVR targets
 *
 * Features:
 *  - Hardware specific interface
 *
 * @copyright
 * Released under the Apache License, Version 2.0, January 2004.
 * http://www.apache.org/licenses/
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************/
// INCLUDES
/****************************************************/

#include <cli_transport.h>

/****************************************************/
// GLOBAL DEFINES
/****************************************************/

/****************************************************/
// GLOBAL ENUMS
/****************************************************/

/****************************************************/
// GLOBAL STRUCTURE TYPE DEFINITION
/****************************************************/

/****************************************************/
// GLOBAL STRUCTURE VARIABLE DECLARATION, INIT
/****************************************************/

/****************************************************/
// GLOBAL MACROS
/****************************************************/

/****************************************************/
// GLOBAL FUNCTIONS
/****************************************************/

#ifdef __AVR__
// Create a transport descriptor for an AVR UART instance
CliTransport avrcomportCreateUartTx(uint8_t uartId, uint32_t bps);

// Configures an AVR communication port
uint8_t avrcomportConfig(CliComPort** cliComPort, uint8_t uartId, uint32_t bps);
#endif

#ifdef __cplusplus
}
#endif
