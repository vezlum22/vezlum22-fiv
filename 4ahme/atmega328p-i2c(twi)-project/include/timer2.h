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

/**
 * @brief Capture points inside the Timer2 compare-match ISR.
 */
enum Timer2CaptureIsrMicros
{
    /** CLI receive handling starts. */
    TIMER2_CAPTURE_ISR_MICROS_CLI_RX_ENTRY,
    /** CLI receive handling ends. */
    TIMER2_CAPTURE_ISR_MICROS_CLI_RX_EXIT,
    /** ADC channel 0 is selected. */
    TIMER2_CAPTURE_ISR_MICROS_SET_ADC_CH_0,
    /** ADC channel 1 is selected. */
    TIMER2_CAPTURE_ISR_MICROS_SET_ADC_CH_1,
    /** Timer2 ISR exit capture point. */
    TIMER2_CAPTURE_ISR_MICROS_EXIT,
    /** Number of Timer2 ISR capture entries. */
    TIMER2_CAPTURE_ISR_MICROS_ENUM_COUNT
};

/**
 * @brief Capture points for storing Timer2 millisecond timestamps.
 */
enum Timer2CaptureMillis
{
    /** Main loop iteration starts. */
    TIMER2_CAPTURE_MILLIS_MAIN_LOOP_START,
    /** Main loop iteration ends. */
    TIMER2_CAPTURE_MILLIS_MAIN_LOOP_END,
    /** Last valid millis capture point. */
    TIMER2_CAPTURE_MILLIS_LAST,
    /** Number of millis capture entries. */
    TIMER2_CAPTURE_MILLIS_ENUM_COUNT
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

/**
 * @brief Initializes Timer2 to toggle OC2A at 10 kHz.
 */
void timer2_OC2A_Toggle10k();

/**
 * @brief Initializes Timer2 to toggle OC2A at the requested frequency.
 *
 * @param fsignal Output signal frequency in Hz.
 */
void timer2_OC2A_SetToggleFrequency(uint16_t fsignal);

/**
 * @brief Initializes Timer2 CTC mode and enables compare-match interrupts.
 *
 * @param periodMicros Interrupt period in microseconds, clamped to 1..128.
 */
void timer2CTCInit(uint32_t periodMicros);

/**
 * @brief Increments the Timer2 millisecond counter.
 */
void timer2IncreaseMillis();

/**
 * @brief Gets the current Timer2 millisecond counter.
 *
 * @return Milliseconds counted by Timer2.
 */
uint32_t timer2GetMillis();

/**
 * @brief Increments the Timer2 microsecond counter by the configured period.
 */
void timer2IncreaseMicros();

/**
 * @brief Gets the current Timer2 microsecond counter including TCNT2 offset.
 *
 * @return Microseconds counted by Timer2.
 */
uint32_t timer2GetMicros();

/**
 * @brief Increments the Timer2 second counter.
 */
void timer2IncreaseSeconds();

/**
 * @brief Gets the current Timer2 second counter.
 *
 * @return Seconds counted by Timer2.
 */
uint32_t timer2GetSeconds();

/**
 * @brief Saves the current TCNT2 value for an ISR capture point.
 *
 * The stored value is updated only if the current counter value is larger
 * than the previous value for the same capture point.
 *
 * @param index ISR capture point to update.
 */
void timer2SaveIsrCapture(enum Timer2CaptureIsrMicros index);

/**
 * @brief Gets the Timer2 ISR load at an indexed capture point.
 *
 * @param index ISR capture point to read.
 * @return Load as percent of the available compare-match period, or 0 for an invalid index.
 */
uint8_t timer2GetIsrLoad(enum Timer2CaptureIsrMicros index);

/**
 * @brief Clears all stored Timer2 ISR captures.
 */
void timer2ClearIsrCaptures();

/**
 * @brief Checks whether the Timer2 ISR overran one compare-match period.
 *
 * The overrun flag is latched. There is no reset function because an overrun
 * means the ISR should be shortened.
 */
void timer2CheckIsrOverrun();

/**
 * @brief Gets the latched Timer2 ISR overrun flag.
 *
 * @return 1 if an ISR overrun was detected, otherwise 0.
 */
uint8_t timer2GetIsrOverrunFlag();

/**
 * @brief Saves the current millisecond counter for a capture point.
 *
 * @param index Millis capture point to update.
 */
void timer2SaveMillisCapture(enum Timer2CaptureMillis index);

/**
 * @brief Gets a saved millisecond counter for a capture point.
 *
 * @param index Millis capture point to read.
 * @return Saved millisecond counter value, or 0 for an invalid index.
 */
uint32_t timer2GetMillisCapture(enum Timer2CaptureMillis index);

#if defined(__cplusplus)
} // extern "C"
#endif
