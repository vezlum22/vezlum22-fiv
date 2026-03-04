/*
 * File:            avrcomport.h
 * Author:          XYZ
 * Date:            24.12.2025
 *
 * Description:
 * Providing AVR UART transport helpers for the CLI
 *
 * License:
 * This code is released under Creative Commons Legal Code CC0 1.0 Universal
 *
 * Contact:
 * email
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
