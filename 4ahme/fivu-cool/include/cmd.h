/**
 * @file cmd.h
 * @author author
 * @date date
 * @version 0.0
 * @brief Execution of cli commands
 *
 * Features a function which checks the command
 * line input for any commands implemented and executes them.
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
#include "cli.h"

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

// Command Execution function
uint8_t cmdExecuteCommand(CliComPort *cliComPort);

#if defined(__cplusplus)
} // extern "C"
#endif
