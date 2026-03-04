/**
 * @file timer2.h
 * @author JR
 * @date 21.01.2026
 * @version 1.0
 * @brief Timer2 functions
 * 
 * Features: Functions to confifgure Timer/Counter2
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

//use stdint.h if typedefs like unit8_t are used
#include <stdint.h>

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

// Initialises Timer2 to toggle OC2A
// with a frequency of 10 kHz
void timer2_OC2A_Toggle10k();

// Initialises Timer2 to toggle OC2A
// with a frequency set by fsignal
void timer2_OC2A_SetToggleFrequency(uint16_t fsignal);

//Initializes Timer2 to generate IRQs depending on period_us
void timer2CTCInit(uint32_t period_us);
#if defined(__cplusplus)
} // extern "C"
#endif
