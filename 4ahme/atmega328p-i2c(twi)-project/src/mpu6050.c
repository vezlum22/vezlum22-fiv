/**
 * @file mpu6050.c
 * @author JR
 * @date 14.05.2026
 * @brief Minimal non-blocking MPU6050 state machine
 */

/****************************************************/
// INCLUDES
/****************************************************/

#include "mpu6050.h"
#include "twi.h"

/****************************************************/
// LOCAL DEFINES
/****************************************************/

#define MPU6050_PWR_MGMT_1             0x6B
#define MPU6050_GYRO_CONFIG            0x1B
#define MPU6050_ACCEL_CONFIG           0x1C
#define MPU6050_ACCEL_XOUT_H           0x3B
#define MPU6050_WAKE_VALUE             0x00
#define MPU6050_GYRO_250DPS_VALUE      0x00
#define MPU6050_ACCEL_2G_VALUE         0x00
#define MPU6050_READ_LENGTH            14
#define MPU6050_WAKE_UP_TIME_MICROS        100000UL
#define MPU6050_DEFAULT_INTERVAL_MICROS    100000UL
#define MPU6050_ACCEL_LSB_PER_G        16384.0f
#define MPU6050_GYRO_LSB_PER_DPS       131.0f
#define MPU6050_GRAVITY_MS2            9.80665f
#define MPU6050_DEG_TO_RAD             0.01745329252f

/****************************************************/
// LOCAL STRUCT TYPE DEFINITION
/****************************************************/

struct Mpu6050
{
    uint8_t address;
    uint8_t configured;
    uint32_t intervalMicros;
    TimebaseGetMicros getMicros;
    uint32_t timespanWaitMicros;
    uint32_t timestampWaitStartMicros;
    uint8_t rxIndex;
    uint8_t rxBuffer[MPU6050_READ_LENGTH];
    int16_t accelX;
    int16_t accelY;
    int16_t accelZ;
    int16_t temperatureDeciC;
    int16_t gyroX;
    int16_t gyroY;
    int16_t gyroZ;
    int16_t accelXOffset;
    int16_t accelYOffset;
    int16_t accelZOffset;
    int16_t gyroXOffset;
    int16_t gyroYOffset;
    int16_t gyroZOffset;
    int32_t calibrationAccelXSum;
    int32_t calibrationAccelYSum;
    int32_t calibrationAccelZSum;
    int32_t calibrationGyroXSum;
    int32_t calibrationGyroYSum;
    int32_t calibrationGyroZSum;
    uint16_t calibrationSampleCount;
    uint16_t calibrationSamplesLeft;
    uint32_t updateDurationMicros;
    enum Mpu6050State state;
    enum Mpu6050State stateAfterYield;
    enum Mpu6050Error error;
};

/****************************************************/
// LOCAL STATIC STRUCTS and VARIABLES
/****************************************************/

static struct Mpu6050 mpu6050 =
{
    .address = MPU6050_DEFAULT_ADDRESS,
    .configured = 0,
    .intervalMicros = MPU6050_DEFAULT_INTERVAL_MICROS,
    .getMicros = 0,
    .timespanWaitMicros = MPU6050_DEFAULT_INTERVAL_MICROS,
    .timestampWaitStartMicros = 0,
    .rxIndex = 0,
    .rxBuffer = {0},
    .accelX = 0,
    .accelY = 0,
    .accelZ = 0,
    .temperatureDeciC = 0,
    .gyroX = 0,
    .gyroY = 0,
    .gyroZ = 0,
    .accelXOffset = 0,
    .accelYOffset = 0,
    .accelZOffset = 0,
    .gyroXOffset = 0,
    .gyroYOffset = 0,
    .gyroZOffset = 0,
    .calibrationAccelXSum = 0,
    .calibrationAccelYSum = 0,
    .calibrationAccelZSum = 0,
    .calibrationGyroXSum = 0,
    .calibrationGyroYSum = 0,
    .calibrationGyroZSum = 0,
    .calibrationSampleCount = 0,
    .calibrationSamplesLeft = 0,
    .updateDurationMicros = 0,
    .state = MPU6050_STATE_DISABLED,
    .stateAfterYield = MPU6050_STATE_DISABLED,
    .error = MPU6050_ERROR_NONE
};

/****************************************************/
// LOCAL FUNCTIONS
/****************************************************/

static void yieldTo(enum Mpu6050State nextState)
{
    mpu6050.stateAfterYield = nextState;
    mpu6050.state = MPU6050_STATE_YIELD_BUS;
}

static void releaseAndYieldTo(enum Mpu6050State nextState)
{
    twiRelease();
    yieldTo(nextState);
}

static void enterError(void)
{
    if(twiIsOwned())
        twiInitiateStop();
    else
        twiRelease();

    mpu6050.error = MPU6050_ERROR_TWI;
    mpu6050.timespanWaitMicros = mpu6050.intervalMicros;
    mpu6050.timestampWaitStartMicros = mpu6050.getMicros();
    mpu6050.configured = 0;
    mpu6050.state = MPU6050_STATE_ERROR;
}

static uint8_t acquireOrWait(void)
{
    if(twiAcquire())
        return 1;

    if(twiGetError() == TWI_ERROR_BUSY)
        return 0;

    enterError();
    return 0;
}

static int16_t readS16(uint8_t msbIndex)
{
    return (int16_t)(((uint16_t)mpu6050.rxBuffer[msbIndex] << 8) | mpu6050.rxBuffer[msbIndex + 1]);
}

static int32_t abs32(int32_t value)
{
    if(value < 0)
        return -value;

    return value;
}

static void finishCalibration(void)
{
    int16_t accelXAverage;
    int16_t accelYAverage;
    int16_t accelZAverage;
    int16_t accelXTarget = 0;
    int16_t accelYTarget = 0;
    int16_t accelZTarget = 0;
    uint16_t sampleCount = mpu6050.calibrationSampleCount;

    if(sampleCount == 0)
        return;

    accelXAverage = (int16_t)(mpu6050.calibrationAccelXSum / sampleCount);
    accelYAverage = (int16_t)(mpu6050.calibrationAccelYSum / sampleCount);
    accelZAverage = (int16_t)(mpu6050.calibrationAccelZSum / sampleCount);

    if(abs32(accelXAverage) >= abs32(accelYAverage) &&
       abs32(accelXAverage) >= abs32(accelZAverage))
        accelXTarget = accelXAverage < 0 ? -16384 : 16384;
    else if(abs32(accelYAverage) >= abs32(accelZAverage))
        accelYTarget = accelYAverage < 0 ? -16384 : 16384;
    else
        accelZTarget = accelZAverage < 0 ? -16384 : 16384;

    mpu6050.accelXOffset = accelXAverage - accelXTarget;
    mpu6050.accelYOffset = accelYAverage - accelYTarget;
    mpu6050.accelZOffset = accelZAverage - accelZTarget;
    mpu6050.gyroXOffset = (int16_t)(mpu6050.calibrationGyroXSum / sampleCount);
    mpu6050.gyroYOffset = (int16_t)(mpu6050.calibrationGyroYSum / sampleCount);
    mpu6050.gyroZOffset = (int16_t)(mpu6050.calibrationGyroZSum / sampleCount);
}

static void addCalibrationSample(void)
{
    if(mpu6050.calibrationSamplesLeft == 0)
        return;

    mpu6050.calibrationAccelXSum += mpu6050.accelX;
    mpu6050.calibrationAccelYSum += mpu6050.accelY;
    mpu6050.calibrationAccelZSum += mpu6050.accelZ;
    mpu6050.calibrationGyroXSum += mpu6050.gyroX;
    mpu6050.calibrationGyroYSum += mpu6050.gyroY;
    mpu6050.calibrationGyroZSum += mpu6050.gyroZ;
    mpu6050.calibrationSamplesLeft--;

    if(mpu6050.calibrationSamplesLeft == 0)
        finishCalibration();
}

static void parseSample(void)
{
    int16_t rawTemperature;

    mpu6050.accelX = readS16(0);
    mpu6050.accelY = readS16(2);
    mpu6050.accelZ = readS16(4);
    rawTemperature = readS16(6);
    mpu6050.gyroX = readS16(8);
    mpu6050.gyroY = readS16(10);
    mpu6050.gyroZ = readS16(12);

    addCalibrationSample();

    mpu6050.temperatureDeciC = (int16_t)(((int32_t)rawTemperature * 10L) / 340L + 365L);
    mpu6050.error = MPU6050_ERROR_NONE;
    mpu6050.timespanWaitMicros = mpu6050.intervalMicros;
    mpu6050.timestampWaitStartMicros = mpu6050.getMicros();
    mpu6050.state = MPU6050_STATE_IDLE;
}

static void updateDurationMicros(uint32_t duration)
{
    if(mpu6050.updateDurationMicros < duration)
        mpu6050.updateDurationMicros = duration;
}

/****************************************************/
// GLOBAL FUNCTIONS
/****************************************************/

void mpu6050Init(uint8_t address, uint32_t intervalMicros, TimebaseGetMicros getMicros)
{
    mpu6050.address = address ? address : MPU6050_DEFAULT_ADDRESS;
    mpu6050.configured = 0;
    mpu6050.intervalMicros = intervalMicros ? intervalMicros : MPU6050_DEFAULT_INTERVAL_MICROS;
    mpu6050.getMicros = getMicros;
    mpu6050.timespanWaitMicros = mpu6050.intervalMicros;
    mpu6050.rxIndex = 0;
    mpu6050.accelX = 0;
    mpu6050.accelY = 0;
    mpu6050.accelZ = 0;
    mpu6050.temperatureDeciC = 0;
    mpu6050.gyroX = 0;
    mpu6050.gyroY = 0;
    mpu6050.gyroZ = 0;
    mpu6050.updateDurationMicros = 0;
    mpu6050ClearCalibration();
    mpu6050.stateAfterYield = MPU6050_STATE_IDLE;
    if(mpu6050.getMicros == 0)
    {
        mpu6050.timestampWaitStartMicros = 0;
        mpu6050.error = MPU6050_ERROR_TIMEBASE_FUNCTION;
        mpu6050.state = MPU6050_STATE_ERROR;
    }
    else
    {
        mpu6050.timestampWaitStartMicros = mpu6050.getMicros() - mpu6050.intervalMicros;
        mpu6050.error = MPU6050_ERROR_NONE;
        mpu6050.state = MPU6050_STATE_IDLE;
    }
}

void mpu6050Disable(void)
{
    if(twiIsOwned())
        twiInitiateStop();
    twiRelease();
    mpu6050.configured = 0;
    mpu6050.state = MPU6050_STATE_DISABLED;
}

void mpu6050StartCalibration(uint16_t sampleCount)
{
    if(sampleCount == 0)
        sampleCount = MPU6050_DEFAULT_CALIBRATION_SAMPLES;

    mpu6050.accelXOffset = 0;
    mpu6050.accelYOffset = 0;
    mpu6050.accelZOffset = 0;
    mpu6050.gyroXOffset = 0;
    mpu6050.gyroYOffset = 0;
    mpu6050.gyroZOffset = 0;
    mpu6050.calibrationAccelXSum = 0;
    mpu6050.calibrationAccelYSum = 0;
    mpu6050.calibrationAccelZSum = 0;
    mpu6050.calibrationGyroXSum = 0;
    mpu6050.calibrationGyroYSum = 0;
    mpu6050.calibrationGyroZSum = 0;
    mpu6050.calibrationSampleCount = sampleCount;
    mpu6050.calibrationSamplesLeft = sampleCount;
}

void mpu6050ClearCalibration(void)
{
    mpu6050.accelXOffset = 0;
    mpu6050.accelYOffset = 0;
    mpu6050.accelZOffset = 0;
    mpu6050.gyroXOffset = 0;
    mpu6050.gyroYOffset = 0;
    mpu6050.gyroZOffset = 0;
    mpu6050.calibrationAccelXSum = 0;
    mpu6050.calibrationAccelYSum = 0;
    mpu6050.calibrationAccelZSum = 0;
    mpu6050.calibrationGyroXSum = 0;
    mpu6050.calibrationGyroYSum = 0;
    mpu6050.calibrationGyroZSum = 0;
    mpu6050.calibrationSampleCount = 0;
    mpu6050.calibrationSamplesLeft = 0;
}

uint8_t mpu6050IsCalibrating(void)
{
    return mpu6050.calibrationSamplesLeft != 0;
}

uint16_t mpu6050GetCalibrationSamplesLeft(void)
{
    return mpu6050.calibrationSamplesLeft;
}

/**
 * @brief Cooperative MPU6050 wake-up and 14-byte motion-read state machine.
 *
 * @dot
 * digraph MPU6050_StateMachine {
 *   rankdir=LR;
 *   node [shape=box, style="rounded"];
 *   stateAfterYield [shape=ellipse, label="stateAfterYield"];
 *
 *   DISABLED;
 *   IDLE -> START_WAKE_TRANSACTION [label="not configured"];
 *   IDLE -> START_READ_TRANSACTION [label="period elapsed"];
 *   START_WAKE_TRANSACTION -> WAIT_START_THEN_SEND_ADDRESS_WAKE;
 *   WAIT_START_THEN_SEND_ADDRESS_WAKE -> WAIT_ADDRESS_ACK_THEN_SEND_REGISTER_WAKE;
 *   WAIT_ADDRESS_ACK_THEN_SEND_REGISTER_WAKE -> WAIT_REGISTER_ACK_THEN_SEND_VALUE_WAKE;
 *   WAIT_REGISTER_ACK_THEN_SEND_VALUE_WAKE -> WAIT_VALUE_ACK_THEN_STOP_WAKE;
 *   WAIT_VALUE_ACK_THEN_STOP_WAKE -> WAIT_STOP_WAKE_TRANSACTION;
 *   WAIT_STOP_WAKE_TRANSACTION -> YIELD_BUS [label="release TWI"];
 *   YIELD_BUS -> stateAfterYield;
 *   stateAfterYield -> WAIT_WAKE_UP [style=dashed];
 *   WAIT_WAKE_UP -> IDLE;
 *   WAIT_MEASUREMENT -> IDLE;
 *
 *   START_READ_TRANSACTION -> WAIT_START_THEN_SEND_ADDRESS_WRITE;
 *   WAIT_START_THEN_SEND_ADDRESS_WRITE -> WAIT_ADDRESS_ACK_THEN_SEND_REGISTER_READ;
 *   WAIT_ADDRESS_ACK_THEN_SEND_REGISTER_READ -> WAIT_REGISTER_ACK_THEN_REPEATED_START_READ;
 *   WAIT_REGISTER_ACK_THEN_REPEATED_START_READ -> WAIT_REPEATED_START_THEN_SEND_ADDRESS_READ;
 *   WAIT_REPEATED_START_THEN_SEND_ADDRESS_READ -> WAIT_ADDRESS_ACK_THEN_READ_BYTE;
 *   WAIT_ADDRESS_ACK_THEN_READ_BYTE -> WAIT_READ_BYTE_THEN_READ_NEXT_OR_STOP;
 *   WAIT_READ_BYTE_THEN_READ_NEXT_OR_STOP -> WAIT_ADDRESS_ACK_THEN_READ_BYTE [label="more bytes"];
 *   WAIT_READ_BYTE_THEN_READ_NEXT_OR_STOP -> WAIT_STOP_READ_TRANSACTION [label="14 bytes"];
 *   WAIT_STOP_READ_TRANSACTION -> YIELD_BUS [label="release TWI"];
 *   YIELD_BUS -> stateAfterYield;
 *   stateAfterYield -> PARSE [style=dashed];
 *   PARSE -> IDLE;
 *   ERROR -> IDLE;
 * }
 * @enddot
 */
void mpu6050UpdateStateMachine(void)
{
    uint32_t timestampNowMicros;

    if(mpu6050.getMicros == 0)
    {
        mpu6050.error = MPU6050_ERROR_TIMEBASE_FUNCTION;
        mpu6050.state = MPU6050_STATE_ERROR;
        return;
    }

    timestampNowMicros = mpu6050.getMicros();

    switch(mpu6050.state)
    {
        case MPU6050_STATE_DISABLED:
            break;

        case MPU6050_STATE_IDLE:
            if(!mpu6050.configured)
                mpu6050.state = MPU6050_STATE_START_WAKE_TRANSACTION;
            else if(timestampNowMicros - mpu6050.timestampWaitStartMicros >= mpu6050.timespanWaitMicros)
                mpu6050.state = MPU6050_STATE_START_READ_TRANSACTION;
            break;

        case MPU6050_STATE_YIELD_BUS:
            mpu6050.state = mpu6050.stateAfterYield;
            break;

        case MPU6050_STATE_START_WAKE_TRANSACTION:
            if(!acquireOrWait())
                break;
            if(twiInitiateStart())
                mpu6050.state = MPU6050_STATE_WAIT_START_THEN_SEND_ADDRESS_WAKE;
            else
                enterError();
            break;

        case MPU6050_STATE_WAIT_START_THEN_SEND_ADDRESS_WAKE:
            if(!twiIsFinished())
                break;
            if(twiGetStatus() == TWI_STATUS_START || twiGetStatus() == TWI_STATUS_REP_START)
            {
                if(twiInitiateAddressWrite(mpu6050.address))
                    mpu6050.state = MPU6050_STATE_WAIT_ADDRESS_ACK_THEN_SEND_REGISTER_WAKE;
                else
                    enterError();
            }
            else
                enterError();
            break;

        case MPU6050_STATE_WAIT_ADDRESS_ACK_THEN_SEND_REGISTER_WAKE:
            if(!twiIsFinished())
                break;
            if(twiGetStatus() == TWI_STATUS_MT_SLA_ACK)
            {
                // Select PWR_MGMT_1 so the next byte can wake the sensor.
                if(twiInitiateWrite(MPU6050_PWR_MGMT_1))
                    mpu6050.state = MPU6050_STATE_WAIT_REGISTER_ACK_THEN_SEND_VALUE_WAKE;
                else
                    enterError();
            }
            else
                enterError();
            break;

        case MPU6050_STATE_WAIT_REGISTER_ACK_THEN_SEND_VALUE_WAKE:
            if(!twiIsFinished())
                break;
            if(twiGetStatus() == TWI_STATUS_MT_DATA_ACK)
            {
                // Clear sleep mode and use the internal 8 MHz oscillator default.
                if(twiInitiateWrite(MPU6050_WAKE_VALUE))
                    mpu6050.state = MPU6050_STATE_WAIT_VALUE_ACK_THEN_STOP_WAKE;
                else
                    enterError();
            }
            else
                enterError();
            break;

        case MPU6050_STATE_WAIT_VALUE_ACK_THEN_STOP_WAKE:
            if(!twiIsFinished())
                break;
            if(twiGetStatus() == TWI_STATUS_MT_DATA_ACK)
            {
                if(twiInitiateStop())
                    mpu6050.state = MPU6050_STATE_WAIT_STOP_WAKE_TRANSACTION;
                else
                    enterError();
            }
            else
                enterError();
            break;

        case MPU6050_STATE_WAIT_STOP_WAKE_TRANSACTION:
            if(twiIsStopPending())
                break;
            mpu6050.timespanWaitMicros = MPU6050_WAKE_UP_TIME_MICROS;
            mpu6050.timestampWaitStartMicros = timestampNowMicros;
            releaseAndYieldTo(MPU6050_STATE_WAIT_WAKE_UP);
            break;

        case MPU6050_STATE_WAIT_WAKE_UP:
            if(timestampNowMicros - mpu6050.timestampWaitStartMicros >= mpu6050.timespanWaitMicros)
            {
                mpu6050.state = MPU6050_STATE_START_CONFIG_GYRO_TRANSACTION;
            }
            break;

        case MPU6050_STATE_START_CONFIG_GYRO_TRANSACTION:
            if(!acquireOrWait())
                break;
            if(twiInitiateStart())
                mpu6050.state = MPU6050_STATE_WAIT_START_THEN_SEND_ADDRESS_CONFIG_GYRO;
            else
                enterError();
            break;

        case MPU6050_STATE_WAIT_START_THEN_SEND_ADDRESS_CONFIG_GYRO:
            if(!twiIsFinished())
                break;
            if(twiGetStatus() == TWI_STATUS_START || twiGetStatus() == TWI_STATUS_REP_START)
            {
                if(twiInitiateAddressWrite(mpu6050.address))
                    mpu6050.state = MPU6050_STATE_WAIT_ADDRESS_ACK_THEN_SEND_REGISTER_CONFIG_GYRO;
                else
                    enterError();
            }
            else
                enterError();
            break;

        case MPU6050_STATE_WAIT_ADDRESS_ACK_THEN_SEND_REGISTER_CONFIG_GYRO:
            if(!twiIsFinished())
                break;
            if(twiGetStatus() == TWI_STATUS_MT_SLA_ACK)
            {
                // Select +/-250 dps full-scale range.
                if(twiInitiateWrite(MPU6050_GYRO_CONFIG))
                    mpu6050.state = MPU6050_STATE_WAIT_REGISTER_ACK_THEN_SEND_VALUE_CONFIG_GYRO;
                else
                    enterError();
            }
            else
                enterError();
            break;

        case MPU6050_STATE_WAIT_REGISTER_ACK_THEN_SEND_VALUE_CONFIG_GYRO:
            if(!twiIsFinished())
                break;
            if(twiGetStatus() == TWI_STATUS_MT_DATA_ACK)
            {
                if(twiInitiateWrite(MPU6050_GYRO_250DPS_VALUE))
                    mpu6050.state = MPU6050_STATE_WAIT_VALUE_ACK_THEN_STOP_CONFIG_GYRO;
                else
                    enterError();
            }
            else
                enterError();
            break;

        case MPU6050_STATE_WAIT_VALUE_ACK_THEN_STOP_CONFIG_GYRO:
            if(!twiIsFinished())
                break;
            if(twiGetStatus() == TWI_STATUS_MT_DATA_ACK)
            {
                if(twiInitiateStop())
                    mpu6050.state = MPU6050_STATE_WAIT_STOP_CONFIG_GYRO_TRANSACTION;
                else
                    enterError();
            }
            else
                enterError();
            break;

        case MPU6050_STATE_WAIT_STOP_CONFIG_GYRO_TRANSACTION:
            if(twiIsStopPending())
                break;
            releaseAndYieldTo(MPU6050_STATE_START_CONFIG_ACCEL_TRANSACTION);
            break;

        case MPU6050_STATE_START_CONFIG_ACCEL_TRANSACTION:
            if(!acquireOrWait())
                break;
            if(twiInitiateStart())
                mpu6050.state = MPU6050_STATE_WAIT_START_THEN_SEND_ADDRESS_CONFIG_ACCEL;
            else
                enterError();
            break;

        case MPU6050_STATE_WAIT_START_THEN_SEND_ADDRESS_CONFIG_ACCEL:
            if(!twiIsFinished())
                break;
            if(twiGetStatus() == TWI_STATUS_START || twiGetStatus() == TWI_STATUS_REP_START)
            {
                if(twiInitiateAddressWrite(mpu6050.address))
                    mpu6050.state = MPU6050_STATE_WAIT_ADDRESS_ACK_THEN_SEND_REGISTER_CONFIG_ACCEL;
                else
                    enterError();
            }
            else
                enterError();
            break;

        case MPU6050_STATE_WAIT_ADDRESS_ACK_THEN_SEND_REGISTER_CONFIG_ACCEL:
            if(!twiIsFinished())
                break;
            if(twiGetStatus() == TWI_STATUS_MT_SLA_ACK)
            {
                // Select +/-2 g full-scale range.
                if(twiInitiateWrite(MPU6050_ACCEL_CONFIG))
                    mpu6050.state = MPU6050_STATE_WAIT_REGISTER_ACK_THEN_SEND_VALUE_CONFIG_ACCEL;
                else
                    enterError();
            }
            else
                enterError();
            break;

        case MPU6050_STATE_WAIT_REGISTER_ACK_THEN_SEND_VALUE_CONFIG_ACCEL:
            if(!twiIsFinished())
                break;
            if(twiGetStatus() == TWI_STATUS_MT_DATA_ACK)
            {
                if(twiInitiateWrite(MPU6050_ACCEL_2G_VALUE))
                    mpu6050.state = MPU6050_STATE_WAIT_VALUE_ACK_THEN_STOP_CONFIG_ACCEL;
                else
                    enterError();
            }
            else
                enterError();
            break;

        case MPU6050_STATE_WAIT_VALUE_ACK_THEN_STOP_CONFIG_ACCEL:
            if(!twiIsFinished())
                break;
            if(twiGetStatus() == TWI_STATUS_MT_DATA_ACK)
            {
                if(twiInitiateStop())
                    mpu6050.state = MPU6050_STATE_WAIT_STOP_CONFIG_ACCEL_TRANSACTION;
                else
                    enterError();
            }
            else
                enterError();
            break;

        case MPU6050_STATE_WAIT_STOP_CONFIG_ACCEL_TRANSACTION:
            if(twiIsStopPending())
                break;
            mpu6050.configured = 1;
            mpu6050.timespanWaitMicros = mpu6050.intervalMicros;
            mpu6050.timestampWaitStartMicros = timestampNowMicros - mpu6050.intervalMicros;
            releaseAndYieldTo(MPU6050_STATE_IDLE);
            break;

        case MPU6050_STATE_WAIT_MEASUREMENT:
            if(timestampNowMicros - mpu6050.timestampWaitStartMicros >= mpu6050.timespanWaitMicros)
                mpu6050.state = MPU6050_STATE_IDLE;
            break;

        case MPU6050_STATE_START_READ_TRANSACTION:
            if(!acquireOrWait())
                break;
            if(twiInitiateStart())
                mpu6050.state = MPU6050_STATE_WAIT_START_THEN_SEND_ADDRESS_WRITE;
            else
                enterError();
            break;

        case MPU6050_STATE_WAIT_START_THEN_SEND_ADDRESS_WRITE:
            if(!twiIsFinished())
                break;
            if(twiGetStatus() == TWI_STATUS_START || twiGetStatus() == TWI_STATUS_REP_START)
            {
                if(twiInitiateAddressWrite(mpu6050.address))
                    mpu6050.state = MPU6050_STATE_WAIT_ADDRESS_ACK_THEN_SEND_REGISTER_READ;
                else
                    enterError();
            }
            else
                enterError();
            break;

        case MPU6050_STATE_WAIT_ADDRESS_ACK_THEN_SEND_REGISTER_READ:
            if(!twiIsFinished())
                break;
            if(twiGetStatus() == TWI_STATUS_MT_SLA_ACK)
            {
                // Select ACCEL_XOUT_H; the following read auto-increments through temp and gyro registers.
                if(twiInitiateWrite(MPU6050_ACCEL_XOUT_H))
                    mpu6050.state = MPU6050_STATE_WAIT_REGISTER_ACK_THEN_REPEATED_START_READ;
                else
                    enterError();
            }
            else
                enterError();
            break;

        case MPU6050_STATE_WAIT_REGISTER_ACK_THEN_REPEATED_START_READ:
            if(!twiIsFinished())
                break;
            if(twiGetStatus() == TWI_STATUS_MT_DATA_ACK)
            {
                if(twiInitiateRepeatedStart())
                    mpu6050.state = MPU6050_STATE_WAIT_REPEATED_START_THEN_SEND_ADDRESS_READ;
                else
                    enterError();
            }
            else
                enterError();
            break;

        case MPU6050_STATE_WAIT_REPEATED_START_THEN_SEND_ADDRESS_READ:
            if(!twiIsFinished())
                break;
            if(twiGetStatus() == TWI_STATUS_START || twiGetStatus() == TWI_STATUS_REP_START)
            {
                if(twiInitiateAddressRead(mpu6050.address))
                {
                    mpu6050.rxIndex = 0;
                    mpu6050.state = MPU6050_STATE_WAIT_ADDRESS_ACK_THEN_READ_BYTE;
                }
                else
                    enterError();
            }
            else
                enterError();
            break;

        case MPU6050_STATE_WAIT_ADDRESS_ACK_THEN_READ_BYTE:
            if(!twiIsFinished())
                break;
            if(twiGetStatus() == TWI_STATUS_MR_SLA_ACK)
            {
                if(mpu6050.rxIndex < MPU6050_READ_LENGTH - 1)
                {
                    if(twiInitiateReadAck())
                        mpu6050.state = MPU6050_STATE_WAIT_READ_BYTE_THEN_READ_NEXT_OR_STOP;
                    else
                        enterError();
                }
                else
                {
                    if(twiInitiateReadNack())
                        mpu6050.state = MPU6050_STATE_WAIT_READ_BYTE_THEN_READ_NEXT_OR_STOP;
                    else
                        enterError();
                }
            }
            else
                enterError();
            break;

        case MPU6050_STATE_WAIT_READ_BYTE_THEN_READ_NEXT_OR_STOP:
            if(!twiIsFinished())
                break;

            if(mpu6050.rxIndex < MPU6050_READ_LENGTH - 1)
            {
                if(twiGetStatus() != TWI_STATUS_MR_DATA_ACK)
                {
                    enterError();
                    break;
                }
            }
            else if(twiGetStatus() != TWI_STATUS_MR_DATA_NACK)
            {
                enterError();
                break;
            }

            twiSaveLastByte();
            // Receive 14 bytes: accel XYZ, temperature, gyro XYZ; each value is signed MSB first.
            mpu6050.rxBuffer[mpu6050.rxIndex] = twiGetLastByte();
            mpu6050.rxIndex++;

            if(mpu6050.rxIndex < MPU6050_READ_LENGTH)
            {
                if(mpu6050.rxIndex < MPU6050_READ_LENGTH - 1)
                {
                    if(!twiInitiateReadAck())
                        enterError();
                }
                else if(!twiInitiateReadNack())
                    enterError();
            }
            else if(twiInitiateStop())
                mpu6050.state = MPU6050_STATE_WAIT_STOP_READ_TRANSACTION;
            else
                enterError();
            break;

        case MPU6050_STATE_WAIT_STOP_READ_TRANSACTION:
            if(twiIsStopPending())
                break;
            releaseAndYieldTo(MPU6050_STATE_PARSE);
            break;

        case MPU6050_STATE_PARSE:
            parseSample();
            break;

        case MPU6050_STATE_ERROR:
            if(twiIsStopPending())
                break;
            twiRelease();
            if(timestampNowMicros - mpu6050.timestampWaitStartMicros >= mpu6050.timespanWaitMicros)
                mpu6050.state = MPU6050_STATE_IDLE;
            break;

        default:
            enterError();
            break;
    }

    updateDurationMicros(mpu6050.getMicros() - timestampNowMicros);
}

float mpu6050GetAccelX(void)
{
    return ((float)(mpu6050.accelX - mpu6050.accelXOffset) * MPU6050_GRAVITY_MS2) / MPU6050_ACCEL_LSB_PER_G;
}

float mpu6050GetAccelY(void)
{
    return ((float)(mpu6050.accelY - mpu6050.accelYOffset) * MPU6050_GRAVITY_MS2) / MPU6050_ACCEL_LSB_PER_G;
}

float mpu6050GetAccelZ(void)
{
    return ((float)(mpu6050.accelZ - mpu6050.accelZOffset) * MPU6050_GRAVITY_MS2) / MPU6050_ACCEL_LSB_PER_G;
}

float mpu6050GetGyroX(void)
{
    return ((float)(mpu6050.gyroX - mpu6050.gyroXOffset) * MPU6050_DEG_TO_RAD) / MPU6050_GYRO_LSB_PER_DPS;
}

float mpu6050GetGyroY(void)
{
    return ((float)(mpu6050.gyroY - mpu6050.gyroYOffset) * MPU6050_DEG_TO_RAD) / MPU6050_GYRO_LSB_PER_DPS;
}

float mpu6050GetGyroZ(void)
{
    return ((float)(mpu6050.gyroZ - mpu6050.gyroZOffset) * MPU6050_DEG_TO_RAD) / MPU6050_GYRO_LSB_PER_DPS;
}

float mpu6050GetTempC(void)
{
    return (float)mpu6050.temperatureDeciC / 10.0f;
}

enum Mpu6050State mpu6050GetState(void)
{
    return mpu6050.state;
}

enum Mpu6050Error mpu6050GetError(void)
{
    return mpu6050.error;
}

uint32_t mpu6050GetUpdateDurationMicros(void)
{
    return mpu6050.updateDurationMicros;
}
