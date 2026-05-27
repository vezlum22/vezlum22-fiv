/**
 * @file statusbar.h
 * @author author
 * @date date
 * @version 0.0
 * @brief Public status bar interface.
 *
 * Features: Status bar functions.
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

/**
 * @brief Width of one formatted status bar line in characters.
 */
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

/**
 * @brief Print the complete status bar to CLI port 0.
 *
 * The output includes runtime, timer load, ADC, SHT3x, MPU6050, and TCS34725
 * diagnostic values. Each line is padded to @ref STATUS_BAR_WIDTH.
 *
 * @param uart0 CLI communication port used for output.
 */
void statusBar0(CliComPort* uart0);

#if defined(__cplusplus)
} // extern "C"
#endif
