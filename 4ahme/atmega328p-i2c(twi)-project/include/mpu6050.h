/**
 * @file mpu6050.h
 * @author JR
 * @date 14.05.2026
 * @version 1.0
 * @brief Minimal non-blocking MPU6050 state machine
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

#define MPU6050_DEFAULT_ADDRESS      0x68
#define MPU6050_ALT_ADDRESS          0x69
#define MPU6050_DEFAULT_CALIBRATION_SAMPLES  50

/****************************************************/
// GLOBAL ENUMS
/****************************************************/

enum Mpu6050State
{
    MPU6050_STATE_DISABLED,
    MPU6050_STATE_IDLE,
    MPU6050_STATE_YIELD_BUS,
    MPU6050_STATE_START_WAKE_TRANSACTION,
    MPU6050_STATE_WAIT_START_THEN_SEND_ADDRESS_WAKE,
    MPU6050_STATE_WAIT_ADDRESS_ACK_THEN_SEND_REGISTER_WAKE,
    MPU6050_STATE_WAIT_REGISTER_ACK_THEN_SEND_VALUE_WAKE,
    MPU6050_STATE_WAIT_VALUE_ACK_THEN_STOP_WAKE,
    MPU6050_STATE_WAIT_STOP_WAKE_TRANSACTION,
    MPU6050_STATE_WAIT_WAKE_UP,
    MPU6050_STATE_START_CONFIG_GYRO_TRANSACTION,
    MPU6050_STATE_WAIT_START_THEN_SEND_ADDRESS_CONFIG_GYRO,
    MPU6050_STATE_WAIT_ADDRESS_ACK_THEN_SEND_REGISTER_CONFIG_GYRO,
    MPU6050_STATE_WAIT_REGISTER_ACK_THEN_SEND_VALUE_CONFIG_GYRO,
    MPU6050_STATE_WAIT_VALUE_ACK_THEN_STOP_CONFIG_GYRO,
    MPU6050_STATE_WAIT_STOP_CONFIG_GYRO_TRANSACTION,
    MPU6050_STATE_START_CONFIG_ACCEL_TRANSACTION,
    MPU6050_STATE_WAIT_START_THEN_SEND_ADDRESS_CONFIG_ACCEL,
    MPU6050_STATE_WAIT_ADDRESS_ACK_THEN_SEND_REGISTER_CONFIG_ACCEL,
    MPU6050_STATE_WAIT_REGISTER_ACK_THEN_SEND_VALUE_CONFIG_ACCEL,
    MPU6050_STATE_WAIT_VALUE_ACK_THEN_STOP_CONFIG_ACCEL,
    MPU6050_STATE_WAIT_STOP_CONFIG_ACCEL_TRANSACTION,
    MPU6050_STATE_WAIT_MEASUREMENT,
    MPU6050_STATE_START_READ_TRANSACTION,
    MPU6050_STATE_WAIT_START_THEN_SEND_ADDRESS_WRITE,
    MPU6050_STATE_WAIT_ADDRESS_ACK_THEN_SEND_REGISTER_READ,
    MPU6050_STATE_WAIT_REGISTER_ACK_THEN_REPEATED_START_READ,
    MPU6050_STATE_WAIT_REPEATED_START_THEN_SEND_ADDRESS_READ,
    MPU6050_STATE_WAIT_ADDRESS_ACK_THEN_READ_BYTE,
    MPU6050_STATE_WAIT_READ_BYTE_THEN_READ_NEXT_OR_STOP,
    MPU6050_STATE_WAIT_STOP_READ_TRANSACTION,
    MPU6050_STATE_PARSE,
    MPU6050_STATE_ERROR
};

enum Mpu6050Error
{
    MPU6050_ERROR_NONE,
    MPU6050_ERROR_TWI,
    MPU6050_ERROR_TIMEBASE_FUNCTION
};

/****************************************************/
// GLOBAL FUNCTIONS
/****************************************************/

/**
 * @brief Initializes the MPU6050 state machine.
 * @param address 7-bit TWI address, or 0 to use MPU6050_DEFAULT_ADDRESS.
 * @param intervalMicros Read period in microseconds, or 0 for the default period.
 * @param getMicros Function used for non-blocking timing.
 */
void mpu6050Init(uint8_t address, uint32_t intervalMicros, TimebaseGetMicros getMicros);

/**
 * @brief Disables the MPU6050 state machine.
 */
void mpu6050Disable(void);

/**
 * @brief Advances the MPU6050 state machine by at most one non-blocking step.
 */
void mpu6050UpdateStateMachine(void);

/**
 * @brief Starts a non-blocking still calibration.
 * @param sampleCount Number of samples to average, or 0 for MPU6050_DEFAULT_CALIBRATION_SAMPLES.
 *
 * The sensor must rest still while samples are collected. Gyro offsets are
 * averaged to zero. Acceleration is adjusted so the dominant resting axis reads
 * +/- 1 g.
 */
void mpu6050StartCalibration(uint16_t sampleCount);

/**
 * @brief Clears all accelerometer and gyroscope calibration offsets.
 */
void mpu6050ClearCalibration(void);

/**
 * @brief Returns 1 while calibration samples are still being collected.
 */
uint8_t mpu6050IsCalibrating(void);

/**
 * @brief Returns the number of calibration samples still missing.
 */
uint16_t mpu6050GetCalibrationSamplesLeft(void);

/** @brief Returns X acceleration in m/s^2. */
float mpu6050GetAccelX(void);
/** @brief Returns Y acceleration in m/s^2. */
float mpu6050GetAccelY(void);
/** @brief Returns Z acceleration in m/s^2. */
float mpu6050GetAccelZ(void);

/** @brief Returns X angular velocity in rad/s. */
float mpu6050GetGyroX(void);
/** @brief Returns Y angular velocity in rad/s. */
float mpu6050GetGyroY(void);
/** @brief Returns Z angular velocity in rad/s. */
float mpu6050GetGyroZ(void);

/**
 * @brief Returns the last measured MPU6050 die temperature in degrees Celsius.
 */
float mpu6050GetTempC(void);

/**
 * @brief Returns the current MPU6050 state-machine state.
 */
enum Mpu6050State mpu6050GetState(void);

/**
 * @brief Returns the last MPU6050 driver error.
 */
enum Mpu6050Error mpu6050GetError(void);

/**
 * @brief Returns the longest measured update duration in microseconds.
 */
uint32_t mpu6050GetUpdateDurationMicros(void);

#if defined(__cplusplus)
} // extern "C"
#endif
