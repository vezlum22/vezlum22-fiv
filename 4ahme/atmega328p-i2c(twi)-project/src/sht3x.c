/**
 * @file sht3x.c
 * @author JR
 * @date 10.05.2026
 * @brief Minimal non-blocking SHT3x state machine
 */

/****************************************************/
// INCLUDES
/****************************************************/

#include "sht3x.h"
#include "twi.h"

/****************************************************/
// LOCAL DEFINES
/****************************************************/

#define SHT3X_CMD_MEASURE_HIGH_REPEATABILITY_MSB    0x24
#define SHT3X_CMD_MEASURE_HIGH_REPEATABILITY_LSB    0x00
#define SHT3X_READ_LENGTH                           6
#define SHT3X_MEASUREMENT_TIME_MICROS                   15000UL
#define SHT3X_DEFAULT_INTERVAL_MICROS                   1000000UL

/****************************************************/
// LOCAL STRUCT TYPE DEFINITION
/****************************************************/

struct Sht3x
{
    uint8_t address;
    uint32_t intervalMicros;
    TimebaseGetMicros getMicros;
    uint32_t timespanWaitMicros;
    uint32_t timestampWaitStartMicros;
    uint8_t rxIndex;
    uint8_t rxBuffer[SHT3X_READ_LENGTH];
    enum Sht3xState stateAfterYield;
    float tempC;
    float relativeHumidity;
    uint32_t updateDurationMicros;
    enum Sht3xState state;
    enum Sht3xError error;
};

/****************************************************/
// LOCAL STATIC STRUCTS and VARIABLES
/****************************************************/

static struct Sht3x sht3x =
{
    .address = SHT3X_DEFAULT_ADDRESS,
    .intervalMicros = SHT3X_DEFAULT_INTERVAL_MICROS,
    .getMicros = 0,
    .timespanWaitMicros = SHT3X_DEFAULT_INTERVAL_MICROS,
    .timestampWaitStartMicros = 0,
    .rxIndex = 0,
    .rxBuffer = {0},
    .stateAfterYield = SHT3X_STATE_IDLE,
    .tempC = 0,
    .relativeHumidity = 0,
    .updateDurationMicros = 0,
    .state = SHT3X_STATE_DISABLED,
    .error = SHT3X_ERROR_NONE
};

/****************************************************/
// LOCAL FUNCTIONS
/****************************************************/

static uint8_t crc8(const uint8_t *data, uint8_t length)
{
    uint8_t crc = 0xFF;

    for(uint8_t i = 0; i < length; i++)
    {
        crc ^= data[i];
        for(uint8_t bit = 0; bit < 8; bit++)
        {
            if(crc & 0x80)
                crc = (crc << 1) ^ 0x31;
            else
                crc <<= 1;
        }
    }

    return crc;
}

static void yieldTo(enum Sht3xState nextState)
{
    sht3x.stateAfterYield = nextState;
    sht3x.state = SHT3X_STATE_YIELD_BUS;
}

static void releaseAndYieldTo(enum Sht3xState nextState)
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
    sht3x.error = SHT3X_ERROR_TWI;
    sht3x.state = SHT3X_STATE_ERROR;
}

static void enterCrcError(void)
{
    if(twiIsOwned())
        twiInitiateStop();
    else
        twiRelease();
    sht3x.error = SHT3X_ERROR_CRC;
    sht3x.state = SHT3X_STATE_ERROR;
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

static void parseSample(void)
{
    uint16_t rawTemperature;
    uint16_t rawHumidity;
    float tempC;
    float relativeHumidity;

    if(crc8(&sht3x.rxBuffer[0], 2) != sht3x.rxBuffer[2])
    {
        enterCrcError();
        return;
    }

    if(crc8(&sht3x.rxBuffer[3], 2) != sht3x.rxBuffer[5])
    {
        enterCrcError();
        return;
    }

    rawTemperature = ((uint16_t)sht3x.rxBuffer[0] << 8) | sht3x.rxBuffer[1];
    rawHumidity = ((uint16_t)sht3x.rxBuffer[3] << 8) | sht3x.rxBuffer[4];

    tempC = -45.0f + (175.0f * (float)rawTemperature) / 65535.0f;
    relativeHumidity = (100.0f * (float)rawHumidity) / 65535.0f;

    if(relativeHumidity < 0.0f)
        relativeHumidity = 0.0f;
    if(relativeHumidity > 100.0f)
        relativeHumidity = 100.0f;

    sht3x.tempC = tempC;
    sht3x.relativeHumidity = relativeHumidity;
    sht3x.error = SHT3X_ERROR_NONE;
    sht3x.timespanWaitMicros = sht3x.intervalMicros;
    sht3x.timestampWaitStartMicros = sht3x.getMicros();
    sht3x.state = SHT3X_STATE_IDLE;
}

static void updateDurationMicros(uint32_t duration)
{
    if(sht3x.updateDurationMicros < duration)
        sht3x.updateDurationMicros = duration;
}

/****************************************************/
// GLOBAL FUNCTIONS
/****************************************************/

void sht3xInit(uint8_t address, uint32_t intervalMicros, TimebaseGetMicros getMicros)
{
    sht3x.address = address ? address : SHT3X_DEFAULT_ADDRESS;
    sht3x.intervalMicros = intervalMicros ? intervalMicros : SHT3X_DEFAULT_INTERVAL_MICROS;
    sht3x.getMicros = getMicros;
    sht3x.timespanWaitMicros = sht3x.intervalMicros;
    sht3x.rxIndex = 0;
    sht3x.stateAfterYield = SHT3X_STATE_IDLE;
    sht3x.tempC = 0;
    sht3x.relativeHumidity = 0;
    sht3x.updateDurationMicros = 0;
    if(sht3x.getMicros == 0)
    {
        sht3x.timestampWaitStartMicros = 0;
        sht3x.error = SHT3X_ERROR_TIMEBASE_FUNCTION;
        sht3x.state = SHT3X_STATE_ERROR;
    }
    else
    {
        sht3x.timestampWaitStartMicros = sht3x.getMicros() - sht3x.intervalMicros;
        sht3x.error = SHT3X_ERROR_NONE;
        sht3x.state = SHT3X_STATE_IDLE;
    }
}

void sht3xDisable(void)
{
    if(twiIsOwned())
        twiInitiateStop();
    twiRelease();
    sht3x.state = SHT3X_STATE_DISABLED;
}

/**
 * @brief Cooperative SHT3x measurement state machine.
 *
 * @dot
 * digraph SHT3X_StateMachine {
 *   rankdir=LR;
 *   node [shape=box, style="rounded"];
 *   stateAfterYield [shape=ellipse, label="stateAfterYield"];
 *
 *   DISABLED;
 *   IDLE -> START_WRITE_TRANSACTION [label="period elapsed"];
 *   START_WRITE_TRANSACTION -> WAIT_START_THEN_SEND_ADDRESS_WRITE;
 *   WAIT_START_THEN_SEND_ADDRESS_WRITE -> WAIT_ADDRESS_ACK_THEN_SEND_COMMAND_MSB;
 *   WAIT_ADDRESS_ACK_THEN_SEND_COMMAND_MSB -> WAIT_COMMAND_MSB_ACK_THEN_SEND_COMMAND_LSB;
 *   WAIT_COMMAND_MSB_ACK_THEN_SEND_COMMAND_LSB -> WAIT_COMMAND_LSB_ACK_THEN_STOP_WRITE_TRANSACTION;
 *   WAIT_COMMAND_LSB_ACK_THEN_STOP_WRITE_TRANSACTION -> WAIT_STOP_WRITE_TRANSACTION;
 *   WAIT_STOP_WRITE_TRANSACTION -> YIELD_BUS [label="release TWI"];
 *   YIELD_BUS -> stateAfterYield;
 *   stateAfterYield -> WAIT_MEASUREMENT [style=dashed];
 *   WAIT_MEASUREMENT -> START_READ_TRANSACTION [label="conversion ready"];
 *   START_READ_TRANSACTION -> WAIT_START_THEN_SEND_ADDRESS_READ;
 *   WAIT_START_THEN_SEND_ADDRESS_READ -> WAIT_ADDRESS_ACK_THEN_READ_BYTE;
 *   WAIT_ADDRESS_ACK_THEN_READ_BYTE -> WAIT_READ_BYTE_THEN_READ_NEXT_OR_STOP;
 *   WAIT_READ_BYTE_THEN_READ_NEXT_OR_STOP -> WAIT_ADDRESS_ACK_THEN_READ_BYTE [label="more bytes"];
 *   WAIT_READ_BYTE_THEN_READ_NEXT_OR_STOP -> WAIT_STOP_READ_TRANSACTION [label="6 bytes"];
 *   WAIT_STOP_READ_TRANSACTION -> YIELD_BUS [label="release TWI"];
 *   YIELD_BUS -> stateAfterYield;
 *   stateAfterYield -> PARSE [style=dashed];
 *   PARSE -> IDLE;
 *   ERROR -> IDLE;
 * }
 * @enddot
 */
void sht3xUpdateStateMachine(void)
{
    uint32_t timestampNowMicros;

    if(sht3x.getMicros == 0)
    {
        sht3x.error = SHT3X_ERROR_TIMEBASE_FUNCTION;
        sht3x.state = SHT3X_STATE_ERROR;
        return;
    }

    timestampNowMicros = sht3x.getMicros();

    switch(sht3x.state)
    {
        case SHT3X_STATE_DISABLED:
            break;

        case SHT3X_STATE_IDLE:
            if(timestampNowMicros - sht3x.timestampWaitStartMicros >= sht3x.timespanWaitMicros)
                sht3x.state = SHT3X_STATE_START_WRITE_TRANSACTION;
            break;

        case SHT3X_STATE_YIELD_BUS:
            sht3x.state = sht3x.stateAfterYield;
            break;

        case SHT3X_STATE_START_WRITE_TRANSACTION:
            if(!acquireOrWait())
                break;
            if(twiInitiateStart())
                sht3x.state = SHT3X_STATE_WAIT_START_THEN_SEND_ADDRESS_WRITE;
            else
                enterError();
            break;

        case SHT3X_STATE_WAIT_START_THEN_SEND_ADDRESS_WRITE:
            if(!twiIsFinished())
                break;
            if(twiGetStatus() == TWI_STATUS_START)
            {
                if(twiInitiateAddressWrite(sht3x.address))
                    sht3x.state = SHT3X_STATE_WAIT_ADDRESS_ACK_THEN_SEND_COMMAND_MSB;
                else
                    enterError();
            }
            else
                enterError();
            break;

        case SHT3X_STATE_WAIT_ADDRESS_ACK_THEN_SEND_COMMAND_MSB:
            if(!twiIsFinished())
                break;
            if(twiGetStatus() == TWI_STATUS_MT_SLA_ACK)
            {
                // Send command byte 0: high-repeatability single-shot measurement.
                if(twiInitiateWrite(SHT3X_CMD_MEASURE_HIGH_REPEATABILITY_MSB))
                    sht3x.state = SHT3X_STATE_WAIT_COMMAND_MSB_ACK_THEN_SEND_COMMAND_LSB;
                else
                    enterError();
            }
            else
                enterError();
            break;

        case SHT3X_STATE_WAIT_COMMAND_MSB_ACK_THEN_SEND_COMMAND_LSB:
            if(!twiIsFinished())
                break;
            if(twiGetStatus() == TWI_STATUS_MT_DATA_ACK)
            {
                // Send command byte 1: clock-stretching disabled variant.
                if(twiInitiateWrite(SHT3X_CMD_MEASURE_HIGH_REPEATABILITY_LSB))
                    sht3x.state = SHT3X_STATE_WAIT_COMMAND_LSB_ACK_THEN_STOP_WRITE_TRANSACTION;
                else
                    enterError();
            }
            else
                enterError();
            break;

        case SHT3X_STATE_WAIT_COMMAND_LSB_ACK_THEN_STOP_WRITE_TRANSACTION:
            if(!twiIsFinished())
                break;
            if(twiGetStatus() == TWI_STATUS_MT_DATA_ACK)
            {
                if(twiInitiateStop())
                    sht3x.state = SHT3X_STATE_WAIT_STOP_WRITE_TRANSACTION;
                else
                    enterError();
            }
            else
                enterError();
            break;

        case SHT3X_STATE_WAIT_STOP_WRITE_TRANSACTION:
            if(twiIsStopPending())
                break;
            sht3x.timespanWaitMicros = SHT3X_MEASUREMENT_TIME_MICROS;
            sht3x.timestampWaitStartMicros = timestampNowMicros;
            releaseAndYieldTo(SHT3X_STATE_WAIT_MEASUREMENT);
            break;

        case SHT3X_STATE_WAIT_MEASUREMENT:
            if(timestampNowMicros - sht3x.timestampWaitStartMicros >= sht3x.timespanWaitMicros)
                sht3x.state = SHT3X_STATE_START_READ_TRANSACTION;
            break;

        case SHT3X_STATE_START_READ_TRANSACTION:
            if(!acquireOrWait())
                break;
            if(twiInitiateStart())
                sht3x.state = SHT3X_STATE_WAIT_START_THEN_SEND_ADDRESS_READ;
            else
                enterError();
            break;

        case SHT3X_STATE_WAIT_START_THEN_SEND_ADDRESS_READ:
            if(!twiIsFinished())
                break;
            if(twiGetStatus() == TWI_STATUS_START || twiGetStatus() == TWI_STATUS_REP_START)
            {
                if(twiInitiateAddressRead(sht3x.address))
                {
                    sht3x.rxIndex = 0;
                    sht3x.state = SHT3X_STATE_WAIT_ADDRESS_ACK_THEN_READ_BYTE;
                }
                else
                    enterError();
            }
            else
                enterError();
            break;

        case SHT3X_STATE_WAIT_ADDRESS_ACK_THEN_READ_BYTE:
            if(!twiIsFinished())
                break;
            if(twiGetStatus() == TWI_STATUS_MR_SLA_ACK)
            {
                if(sht3x.rxIndex < SHT3X_READ_LENGTH - 1)
                {
                    if(twiInitiateReadAck())
                        sht3x.state = SHT3X_STATE_WAIT_READ_BYTE_THEN_READ_NEXT_OR_STOP;
                    else
                        enterError();
                }
                else
                {
                    if(twiInitiateReadNack())
                        sht3x.state = SHT3X_STATE_WAIT_READ_BYTE_THEN_READ_NEXT_OR_STOP;
                    else
                        enterError();
                }
            }
            else
                enterError();
            break;

        case SHT3X_STATE_WAIT_READ_BYTE_THEN_READ_NEXT_OR_STOP:
            if(!twiIsFinished())
                break;

            if(sht3x.rxIndex < SHT3X_READ_LENGTH - 1)
            {
                if(twiGetStatus() != TWI_STATUS_MR_DATA_ACK)
                {
                    enterError();
                    break;
                }
            }
            else
            {
                if(twiGetStatus() != TWI_STATUS_MR_DATA_NACK)
                {
                    enterError();
                    break;
                }
            }

            twiSaveLastByte();
            // Receive 6 bytes: T_MSB, T_LSB, T_CRC, RH_MSB, RH_LSB, RH_CRC.
            sht3x.rxBuffer[sht3x.rxIndex] = twiGetLastByte();
            sht3x.rxIndex++;

            if(sht3x.rxIndex < SHT3X_READ_LENGTH)
            {
                if(sht3x.rxIndex < SHT3X_READ_LENGTH - 1)
                {
                    if(!twiInitiateReadAck())
                        enterError();
                }
                else if(!twiInitiateReadNack())
                    enterError();
            }
            else if(twiInitiateStop())
                sht3x.state = SHT3X_STATE_WAIT_STOP_READ_TRANSACTION;
            else
                enterError();
            break;

        case SHT3X_STATE_WAIT_STOP_READ_TRANSACTION:
            if(twiIsStopPending())
                break;
            releaseAndYieldTo(SHT3X_STATE_PARSE);
            break;

        case SHT3X_STATE_PARSE:
            parseSample();
            break;

        case SHT3X_STATE_ERROR:
            if(twiIsStopPending())
                break;
            twiRelease();
            sht3x.timespanWaitMicros = sht3x.intervalMicros;
            sht3x.timestampWaitStartMicros = timestampNowMicros;
            sht3x.state = SHT3X_STATE_IDLE;
            break;

        default:
            enterError();
            break;
    }

    updateDurationMicros(sht3x.getMicros() - timestampNowMicros);
}

float sht3xGetTempC(void)
{
    return sht3x.tempC;
}

float sht3xGetRelativeHumidity(void)
{
    return sht3x.relativeHumidity;
}

enum Sht3xState sht3xGetState(void)
{
    return sht3x.state;
}

enum Sht3xError sht3xGetError(void)
{
    return sht3x.error;
}

uint32_t sht3xGetUpdateDurationMicros(void)
{
    return sht3x.updateDurationMicros;
}
