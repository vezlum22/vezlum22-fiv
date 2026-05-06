/**
 * @file template.h
 * @author author
 * @date date
 * @version 0.0
 * @brief h-template
 *
 * Features:
 *  
 * @copyright
 * Released under the Apache License, Version 2.0, January 2004.
 * http://www.apache.org/licenses/
 */

#pragma once

#if defined(__cplusplus)
extern "C" {
#endif

/****************************************************/
// INCLUDES
/****************************************************/

#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include "cli_transport.h"

/****************************************************/
// GLOBAL DEFINES
/****************************************************/

/****************************************************/
// GLOBAL ENUMS
/****************************************************/

/****************************************************/
// GLOBAL STRUCT TYPE DEFINITION
/****************************************************/

/****************************************************/
// GLOBAL STATIC STRUCTS and VARIABLES
/****************************************************/

/****************************************************/
// GLOBAL MACROS
/****************************************************/

/****************************************************/
// GLOBAL FUNCTIONS
/****************************************************/

extern CliComPort *uart0;
uint8_t cmdExecuteCommand(CliComPort *cliComPort);

#if defined(__cplusplus)
} // extern "C"
#endif
