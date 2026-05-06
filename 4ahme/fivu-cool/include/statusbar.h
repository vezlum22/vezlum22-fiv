/**
 * @file statusbar.h
 * @author author
 * @date date
 * @version 0.0
 * @brief h-template
 *
 * Features: Status bar functions
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

#include "cli.h"

/****************************************************/
// GLOBAL DEFINES
/****************************************************/

#define STATUS_BAR_WIDTH 80

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

void statusBar0(CliComPort* uart0);

#if defined(__cplusplus)
} // extern "C"
#endif
