/**
 * @file tcs34725.h
 * @author JR
 * @date 09.05.2026
 * @version 1.0
 * @brief TCS34725 functions
 *
 * Features: Reading of RGBC values and colour temperature calculations
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
#include "timebase.h"

/****************************************************/
// GLOBAL DEFINES
/****************************************************/

/****************************************************/
// GLOBAL ENUMS
/****************************************************/

enum Tcs34725State
{
    TCS34725_STATE_DISABLED,
    TCS34725_STATE_IDLE,
    TCS34725_STATE_YIELD_BUS,
    TCS34725_STATE_START_POWER_ON_TRANSACTION,
    TCS34725_STATE_WAIT_START_THEN_SEND_ADDRESS_POWER_ON,
    TCS34725_STATE_WAIT_ADDRESS_ACK_THEN_SEND_REGISTER_POWER_ON,
    TCS34725_STATE_WAIT_REGISTER_ACK_THEN_SEND_VALUE_POWER_ON,
    TCS34725_STATE_WAIT_VALUE_ACK_THEN_STOP_POWER_ON,
    TCS34725_STATE_WAIT_STOP_POWER_ON_TRANSACTION,
    TCS34725_STATE_WAIT_POWER_ON,
    TCS34725_STATE_START_ENABLE_ADC_TRANSACTION,
    TCS34725_STATE_WAIT_START_THEN_SEND_ADDRESS_ENABLE_ADC,
    TCS34725_STATE_WAIT_ADDRESS_ACK_THEN_SEND_REGISTER_ENABLE_ADC,
    TCS34725_STATE_WAIT_REGISTER_ACK_THEN_SEND_VALUE_ENABLE_ADC,
    TCS34725_STATE_WAIT_VALUE_ACK_THEN_STOP_ENABLE_ADC,
    TCS34725_STATE_WAIT_STOP_ENABLE_ADC_TRANSACTION,
    TCS34725_STATE_WAIT_MEASUREMENT,
    TCS34725_STATE_START_STATUS_TRANSACTION,
    TCS34725_STATE_WAIT_START_THEN_SEND_ADDRESS_STATUS_WRITE,
    TCS34725_STATE_WAIT_ADDRESS_ACK_THEN_SEND_REGISTER_STATUS,
    TCS34725_STATE_WAIT_REGISTER_ACK_THEN_REPEATED_START_STATUS,
    TCS34725_STATE_WAIT_REPEATED_START_THEN_SEND_ADDRESS_STATUS_READ,
    TCS34725_STATE_WAIT_ADDRESS_ACK_THEN_READ_STATUS_BYTE,
    TCS34725_STATE_WAIT_STATUS_BYTE_THEN_STOP_STATUS,
    TCS34725_STATE_WAIT_STOP_STATUS_TRANSACTION,
    TCS34725_STATE_CHECK_STATUS,
    TCS34725_STATE_START_RGBC_TRANSACTION,
    TCS34725_STATE_WAIT_START_THEN_SEND_ADDRESS_RGBC_WRITE,
    TCS34725_STATE_WAIT_ADDRESS_ACK_THEN_SEND_REGISTER_RGBC,
    TCS34725_STATE_WAIT_REGISTER_ACK_THEN_REPEATED_START_RGBC,
    TCS34725_STATE_WAIT_REPEATED_START_THEN_SEND_ADDRESS_RGBC_READ,
    TCS34725_STATE_WAIT_ADDRESS_ACK_THEN_READ_RGBC_BYTE,
    TCS34725_STATE_WAIT_RGBC_BYTE_THEN_READ_NEXT_OR_STOP,
    TCS34725_STATE_WAIT_STOP_RGBC_TRANSACTION,
    TCS34725_STATE_PARSE_RGBC,
    TCS34725_STATE_ERROR
};

enum Tcs34725Error
{
    TCS34725_ERROR_NONE,
    TCS34725_ERROR_TWI,
    TCS34725_ERROR_TIMEBASE_FUNCTION
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
 * @brief Initializes the TCS34725 state machine.
 * @param intervalMicros Read period in microseconds, or 0 for the default period.
 * @param getMicros Function used for non-blocking timing.
 */
void tcs34725Init(uint32_t intervalMicros, TimebaseGetMicros getMicros);

/** @brief Disables the TCS34725 state machine. */
void tcs34725Disable(void);

/** @brief Advances the TCS34725 state machine by at most one non-blocking step. */
void tcs34725UpdateStateMachine(void);

/** @brief Returns the last raw red channel value. */
uint16_t tcs34725GetRawR(void);

/** @brief Returns the last raw green channel value. */
uint16_t tcs34725GetRawG(void);

/** @brief Returns the last raw blue channel value. */
uint16_t tcs34725GetRawB(void);

/** @brief Returns the last raw clear channel value. */
uint16_t tcs34725GetC(void);

/** @brief Returns the red channel normalized to the clear channel. */
uint8_t tcs34725GetNormedR(void);

/** @brief Returns the green channel normalized to the clear channel. */
uint8_t tcs34725GetNormedG(void);

/** @brief Returns the blue channel normalized to the clear channel. */
uint8_t tcs34725GetNormedB(void);

/** @brief Returns the calculated color temperature in kelvin. */
uint16_t tcs34725GetColorTemp(void);

/** @brief Returns the calculated illuminance in lux. */
uint16_t tcs34725GetLux(void);

/** @brief Returns the current TCS34725 state-machine state. */
enum Tcs34725State tcs34725GetState(void);

/** @brief Returns the last TCS34725 driver error. */
enum Tcs34725Error tcs34725GetError(void);

/** @brief Returns the longest measured update duration in microseconds. */
uint32_t tcs34725GetUpdateDurationMicros(void);

#if defined(__cplusplus)
} // extern "C"
#endif
