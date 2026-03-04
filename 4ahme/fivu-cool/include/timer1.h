/**
 * @file timer1.h
 * @author JR
 * @date 21.01.2026
 * @version 1.0
 * @brief Timer1 functions
 *
 * Features: Functions to confifgure Timer/Counter1
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

// Initialises Timer1 to output a PWM servo signal
// with a frequency of 50 Hz (T = 20 ms)
void timer1PWMInit();

//Init Timer to toggle PB1 each second
void timer1_OC1A_Toggle1s();
#if defined(__cplusplus)
} // extern "C"
#endif
