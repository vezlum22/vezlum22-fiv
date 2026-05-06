/**
 * @file adc.h
 * @author JR
 * @date 15.04.2026
 * @version 1.0
 * @brief ADC functions 
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

/****************************************************/
// GLOBAL DEFINES
/****************************************************/

/****************************************************/
// GLOBAL ENUMS
/****************************************************/

enum AdcChannel
{
    ADC_CH_0 = 0,
    ADC_CH_1,
    ADC_CH_2,
    ADC_CH_3,
    ADC_CH_4,
    ADC_CH_5,
    ADC_CH_6,
    ADC_CH_7,
    ADC_CH_TEMP,
    RSVD_ADC_CH9,
    RSVD_ADC_CH10,
    RSVD_ADC_CH11,
    RSVD_ADC_CH12,
    RSVD_ADC_CH13,
    ADC_CH_BANDGAP,
    ADC_CH_GND
};

enum AdcVRef
{
    ADC_VREF_AREF = 0,
    ADC_VREF_AVCC,
    RSVD_ADC_VREF,
    ADC_VREF_BANDGAP
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

// Initializes the ADC hardware and internal ADC state
// for conversions with 8-bit resolution.
uint8_t adcInit(enum AdcVRef adcVRef, uint8_t didr0);

// Disables the ADC.
void adcDisable();

// Sets the channel to be sampled.
uint8_t adcSetNewChannel(enum AdcChannel adcChannel);

// Starts a new analog conversion on a channel
// set by adcSetNewChannel(enum AdcChannel adcChannel).
uint8_t adcStartNewConversion();

// Stores the last conversion result of a channel
// set by adcSetNewChannel(enum AdcChannel adcChannel).
uint8_t adcSaveLastConversion();

// Returns the analog conversion result of a channel
// set by adcSetNewChannel(enum AdcChannel adcChannel).
uint8_t adcGetLastConversion();

// Returns the last sampled AdcChannel enumerator.
enum AdcChannel adcGetLastChannel();

#if defined(__cplusplus)
} // extern "C"
#endif
