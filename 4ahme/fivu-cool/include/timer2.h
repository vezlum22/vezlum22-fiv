/**
 * @file timer2.h
 * @author JR
 * @date 21.01.2026
 * @version 1.0
 * @brief Timer2 functions
 * 
 * Features: Functions to configure Timer/Counter2
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

enum Timer2IsrCapture
{
    // User enumerators
    TIMER2_ISR_CLI_RX_ENTRY,
    TIMER2_ISR_CLI_RX_EXIT,
    // Required trailing enumerators
    TIMER2_ISR_EXIT,
    TIMER2_ISR_CAPTURE_COUNT
};

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

// Initialises Timer2 to generate IRQs depending on
// period_us
void timer2CTCInit(uint32_t period_us);

// Sets value of timer2.seconds by increasing it
void timer2IncreaseSeconds();

// Gets the current value of timer2.seconds
uint32_t timer2GetSeconds();

// Captures the current value of TCNT2 and stores it in
// timer2.capture[index] if it is the largest value seen so far.
void timer2Capture(enum Timer2IsrCapture index);

// Returns the duration from Timer2 ISR entry to the indexed
// capture, relative to the maximum available ISR time
uint16_t timer2GetRelDuration(enum Timer2IsrCapture index);

// Clears all captures taken
void timer2ClearIsrCaptures();

// Indicates whether the ISR took longer to run than one
// Timer2 compare-match period. There is no reset function
// because if isrOverrunFlag is ever set to 1, the ISR
// should be shortened.
void timer2CheckOverrun();

// Returns the isrOverrunFlag
uint8_t timer2GetOverrunFlag();

#if defined(__cplusplus)
} // extern "C"
#endif