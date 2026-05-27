/**
 * @file sht3x.h
 * @author JR
 * @date 10.05.2026
 * @version 1.0
 * @brief Minimal non-blocking SHT3x state machine
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

#define SHT3X_DEFAULT_ADDRESS      0x44

/****************************************************/
// GLOBAL ENUMS
/****************************************************/

enum Sht3xState
{
    SHT3X_STATE_DISABLED,
    SHT3X_STATE_IDLE,
    SHT3X_STATE_YIELD_BUS,
    SHT3X_STATE_START_WRITE_TRANSACTION,
    SHT3X_STATE_WAIT_START_THEN_SEND_ADDRESS_WRITE,
    SHT3X_STATE_WAIT_ADDRESS_ACK_THEN_SEND_COMMAND_MSB,
    SHT3X_STATE_WAIT_COMMAND_MSB_ACK_THEN_SEND_COMMAND_LSB,
    SHT3X_STATE_WAIT_COMMAND_LSB_ACK_THEN_STOP_WRITE_TRANSACTION,
    SHT3X_STATE_WAIT_STOP_WRITE_TRANSACTION,
    SHT3X_STATE_WAIT_MEASUREMENT,
    SHT3X_STATE_START_READ_TRANSACTION,
    SHT3X_STATE_WAIT_START_THEN_SEND_ADDRESS_READ,
    SHT3X_STATE_WAIT_ADDRESS_ACK_THEN_READ_BYTE,
    SHT3X_STATE_WAIT_READ_BYTE_THEN_READ_NEXT_OR_STOP,
    SHT3X_STATE_WAIT_STOP_READ_TRANSACTION,
    SHT3X_STATE_PARSE,
    SHT3X_STATE_ERROR
};

enum Sht3xError
{
    SHT3X_ERROR_NONE,
    SHT3X_ERROR_TWI,
    SHT3X_ERROR_CRC,
    SHT3X_ERROR_TIMEBASE_FUNCTION
};

/****************************************************/
// GLOBAL FUNCTIONS
/****************************************************/

/**
 * @brief Initializes the SHT3x state machine.
 * @param address 7-bit TWI address, or 0 to use SHT3X_DEFAULT_ADDRESS.
 * @param intervalMicros Measurement period in microseconds, or 0 for the default period.
 * @param getMicros Function used for non-blocking timing.
 */
void sht3xInit(uint8_t address, uint32_t intervalMicros, TimebaseGetMicros getMicros);

/**
 * @brief Disables the SHT3x state machine.
 */
void sht3xDisable(void);

/**
 * @brief Advances the SHT3x state machine by at most one non-blocking step.
 */
void sht3xUpdateStateMachine(void);

/** @brief Returns the last measured temperature in degrees Celsius. */
float sht3xGetTempC(void);

/** @brief Returns the last measured relative humidity in percent. */
float sht3xGetRelativeHumidity(void);

/** @brief Returns the current SHT3x state-machine state. */
enum Sht3xState sht3xGetState(void);

/** @brief Returns the last SHT3x driver error. */
enum Sht3xError sht3xGetError(void);

/** @brief Returns the longest measured update duration in microseconds. */
uint32_t sht3xGetUpdateDurationMicros(void);

#if defined(__cplusplus)
} // extern "C"
#endif
