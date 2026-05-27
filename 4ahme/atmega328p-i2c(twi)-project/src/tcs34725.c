/**
 * @file tcs34725.c
 * @author JR
 * @date 09.05.2026
 * @brief Non-blocking TCS34725 state machine
 */

/****************************************************/
// INCLUDES
/****************************************************/

#include "tcs34725.h"
#include "twi.h"

/****************************************************/
// LOCAL DEFINES
/****************************************************/

#define TCS34725_ADDRESS                  0x29

#define TCS34725_COMMAND_BIT              0x80
#define TCS34725_COMMAND_AUTO_INCREMENT   0x20

#define TCS34725_ENABLE                   0x00
#define TCS34725_ENABLE_PON               0x01
#define TCS34725_ENABLE_AEN               0x02
#define TCS34725_STATUS                   0x13
#define TCS34725_STATUS_AVALID            0x01
#define TCS34725_CDATAL                   0x14

#define TCS34725_POWER_ON_WAIT_MICROS         3000UL
#define TCS34725_DATA_RETRY_MICROS            5000UL
#define TCS34725_RGBC_READ_LENGTH         8
#define TCS34725_DEFAULT_INTERVAL_MICROS      1000000UL

/****************************************************/
// LOCAL STRUCT TYPE DEFINITION
/****************************************************/

struct Tcs34725
{
    uint8_t configured;
    uint8_t status;
    uint8_t rxBuffer[TCS34725_RGBC_READ_LENGTH];
    uint8_t rxIndex;
    uint32_t intervalMicros;
    TimebaseGetMicros getMicros;
    uint32_t timespanWaitMicros;
    uint32_t timestampWaitStartMicros;
    uint16_t r, g, b, c, colorTemp, lux;
    uint8_t rnorm, gnorm, bnorm;
    uint32_t updateDurationMicros;
    enum Tcs34725State state;
    enum Tcs34725State stateAfterYield;
    enum Tcs34725Error error;
};

/****************************************************/
// LOCAL STATIC STRUCTS and VARIABLES
/****************************************************/

static struct Tcs34725 tcs34725 =
{
    .configured = 0,
    .status = 0,
    .rxBuffer = {0},
    .rxIndex = 0,
    .intervalMicros = TCS34725_DEFAULT_INTERVAL_MICROS,
    .getMicros = 0,
    .timespanWaitMicros = TCS34725_DEFAULT_INTERVAL_MICROS,
    .timestampWaitStartMicros = 0,
    .r = 0,
    .g = 0,
    .b = 0,
    .c = 0,
    .colorTemp = 0,
    .lux = 0,
    .rnorm = 0,
    .gnorm = 0,
    .bnorm = 0,
    .updateDurationMicros = 0,
    .state = TCS34725_STATE_DISABLED,
    .stateAfterYield = TCS34725_STATE_DISABLED,
    .error = TCS34725_ERROR_NONE
};

/****************************************************/
// LOCAL FUNCTIONS
/****************************************************/

static uint8_t command(uint8_t reg)
{
    return TCS34725_COMMAND_BIT | reg;
}

static uint8_t autoIncrementCommand(uint8_t reg)
{
    return TCS34725_COMMAND_BIT | TCS34725_COMMAND_AUTO_INCREMENT | reg;
}

static void yieldTo(enum Tcs34725State nextState)
{
    tcs34725.stateAfterYield = nextState;
    tcs34725.state = TCS34725_STATE_YIELD_BUS;
}

static void enterError(void)
{
    if(twiIsOwned())
        twiInitiateStop();
    else
        twiRelease();

    tcs34725.error = TCS34725_ERROR_TWI;
    tcs34725.timespanWaitMicros = tcs34725.intervalMicros;
    tcs34725.timestampWaitStartMicros = tcs34725.getMicros();
    tcs34725.configured = 0;
    tcs34725.state = TCS34725_STATE_ERROR;
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

static void releaseAndYieldTo(enum Tcs34725State nextState)
{
    twiRelease();
    yieldTo(nextState);
}

static uint16_t calculateColorTemperatureDn40(uint16_t r, uint16_t g, uint16_t b, uint16_t c)
{
    uint32_t rgbSum;
    uint32_t ir;
    int32_t r2;
    int32_t b2;

    if(c == 0)
        return 0;

    if(c >= 65535)
        return 0;

    rgbSum = (uint32_t)r + g + b;
    ir = (rgbSum > c) ? (rgbSum - c) / 2 : 0;
    r2 = (int32_t)r - (int32_t)ir;
    b2 = (int32_t)b - (int32_t)ir;

    if(r2 <= 0 || b2 <= 0)
        return 0;

    return (uint16_t)((3810UL * (uint32_t)b2) / (uint32_t)r2 + 1391UL);
}

static uint16_t calculateLux(uint16_t r, uint16_t g, uint16_t b)
{
    int32_t illuminance;

    illuminance = (-3247L * r) + (15784L * g) - (7319L * b);
    illuminance /= 10000L;

    if(illuminance < 0)
        return 0;
    if(illuminance > 65535L)
        return 65535;

    return (uint16_t)illuminance;
}

static uint8_t normalizeToClear(uint16_t channel, uint16_t clear)
{
    uint32_t normed;

    if(clear == 0)
        return 0;

    normed = ((uint32_t)channel * 255UL) / clear;
    if(normed > 255UL)
        return 255;

    return (uint8_t)normed;
}

static void parseRgbc(void)
{
    tcs34725.c = ((uint16_t)tcs34725.rxBuffer[1] << 8) | tcs34725.rxBuffer[0];
    tcs34725.r = ((uint16_t)tcs34725.rxBuffer[3] << 8) | tcs34725.rxBuffer[2];
    tcs34725.g = ((uint16_t)tcs34725.rxBuffer[5] << 8) | tcs34725.rxBuffer[4];
    tcs34725.b = ((uint16_t)tcs34725.rxBuffer[7] << 8) | tcs34725.rxBuffer[6];

    tcs34725.colorTemp = calculateColorTemperatureDn40(tcs34725.r, tcs34725.g, tcs34725.b, tcs34725.c);
    tcs34725.lux = calculateLux(tcs34725.r, tcs34725.g, tcs34725.b);
    tcs34725.rnorm = normalizeToClear(tcs34725.r, tcs34725.c);
    tcs34725.gnorm = normalizeToClear(tcs34725.g, tcs34725.c);
    tcs34725.bnorm = normalizeToClear(tcs34725.b, tcs34725.c);
}

static void updateDurationMicros(uint32_t duration)
{
    if(tcs34725.updateDurationMicros < duration)
        tcs34725.updateDurationMicros = duration;
}

/****************************************************/
// GLOBAL FUNCTIONS
/****************************************************/

void tcs34725Init(uint32_t intervalMicros, TimebaseGetMicros getMicros)
{
    tcs34725.configured = 0;
    tcs34725.status = 0;
    tcs34725.rxIndex = 0;
    tcs34725.intervalMicros = intervalMicros ? intervalMicros : TCS34725_DEFAULT_INTERVAL_MICROS;
    tcs34725.getMicros = getMicros;
    tcs34725.timespanWaitMicros = tcs34725.intervalMicros;
    tcs34725.r = 0;
    tcs34725.g = 0;
    tcs34725.b = 0;
    tcs34725.c = 0;
    tcs34725.colorTemp = 0;
    tcs34725.lux = 0;
    tcs34725.rnorm = 0;
    tcs34725.gnorm = 0;
    tcs34725.bnorm = 0;
    tcs34725.updateDurationMicros = 0;
    tcs34725.stateAfterYield = TCS34725_STATE_IDLE;
    if(tcs34725.getMicros == 0)
    {
        tcs34725.timestampWaitStartMicros = 0;
        tcs34725.error = TCS34725_ERROR_TIMEBASE_FUNCTION;
        tcs34725.state = TCS34725_STATE_ERROR;
    }
    else
    {
        tcs34725.timestampWaitStartMicros = tcs34725.getMicros() - tcs34725.intervalMicros;
        tcs34725.error = TCS34725_ERROR_NONE;
        tcs34725.state = TCS34725_STATE_IDLE;
    }
}

void tcs34725Disable(void)
{
    if(twiIsOwned())
        twiInitiateStop();
    twiRelease();
    tcs34725.configured = 0;
    tcs34725.state = TCS34725_STATE_DISABLED;
}

/**
 * @brief Cooperative TCS34725 power-up, status-polling, and RGBC-read state machine.
 *
 * @dot
 * digraph TCS34725_StateMachine {
 *   rankdir=LR;
 *   node [shape=box, style="rounded"];
 *   stateAfterYield [shape=ellipse, label="stateAfterYield"];
 *
 *   DISABLED;
 *   IDLE -> START_POWER_ON_TRANSACTION [label="not configured"];
 *   IDLE -> START_STATUS_TRANSACTION [label="period elapsed"];
 *   START_POWER_ON_TRANSACTION -> WAIT_START_THEN_SEND_ADDRESS_POWER_ON;
 *   WAIT_START_THEN_SEND_ADDRESS_POWER_ON -> WAIT_ADDRESS_ACK_THEN_SEND_REGISTER_POWER_ON;
 *   WAIT_ADDRESS_ACK_THEN_SEND_REGISTER_POWER_ON -> WAIT_REGISTER_ACK_THEN_SEND_VALUE_POWER_ON;
 *   WAIT_REGISTER_ACK_THEN_SEND_VALUE_POWER_ON -> WAIT_VALUE_ACK_THEN_STOP_POWER_ON;
 *   WAIT_VALUE_ACK_THEN_STOP_POWER_ON -> WAIT_STOP_POWER_ON_TRANSACTION;
 *   WAIT_STOP_POWER_ON_TRANSACTION -> YIELD_BUS [label="release TWI"];
 *   YIELD_BUS -> stateAfterYield;
 *   stateAfterYield -> WAIT_POWER_ON [style=dashed];
 *   WAIT_POWER_ON -> START_ENABLE_ADC_TRANSACTION;
 *
 *   START_ENABLE_ADC_TRANSACTION -> WAIT_START_THEN_SEND_ADDRESS_ENABLE_ADC;
 *   WAIT_START_THEN_SEND_ADDRESS_ENABLE_ADC -> WAIT_ADDRESS_ACK_THEN_SEND_REGISTER_ENABLE_ADC;
 *   WAIT_ADDRESS_ACK_THEN_SEND_REGISTER_ENABLE_ADC -> WAIT_REGISTER_ACK_THEN_SEND_VALUE_ENABLE_ADC;
 *   WAIT_REGISTER_ACK_THEN_SEND_VALUE_ENABLE_ADC -> WAIT_VALUE_ACK_THEN_STOP_ENABLE_ADC;
 *   WAIT_VALUE_ACK_THEN_STOP_ENABLE_ADC -> WAIT_STOP_ENABLE_ADC_TRANSACTION;
 *   WAIT_STOP_ENABLE_ADC_TRANSACTION -> YIELD_BUS [label="release TWI"];
 *   YIELD_BUS -> stateAfterYield;
 *   stateAfterYield -> WAIT_MEASUREMENT [style=dashed];
 *
 *   WAIT_MEASUREMENT -> IDLE;
 *   START_STATUS_TRANSACTION -> WAIT_START_THEN_SEND_ADDRESS_STATUS_WRITE;
 *   WAIT_START_THEN_SEND_ADDRESS_STATUS_WRITE -> WAIT_ADDRESS_ACK_THEN_SEND_REGISTER_STATUS;
 *   WAIT_ADDRESS_ACK_THEN_SEND_REGISTER_STATUS -> WAIT_REGISTER_ACK_THEN_REPEATED_START_STATUS;
 *   WAIT_REGISTER_ACK_THEN_REPEATED_START_STATUS -> WAIT_REPEATED_START_THEN_SEND_ADDRESS_STATUS_READ;
 *   WAIT_REPEATED_START_THEN_SEND_ADDRESS_STATUS_READ -> WAIT_ADDRESS_ACK_THEN_READ_STATUS_BYTE;
 *   WAIT_ADDRESS_ACK_THEN_READ_STATUS_BYTE -> WAIT_STATUS_BYTE_THEN_STOP_STATUS;
 *   WAIT_STATUS_BYTE_THEN_STOP_STATUS -> WAIT_STOP_STATUS_TRANSACTION;
 *   WAIT_STOP_STATUS_TRANSACTION -> YIELD_BUS [label="release TWI"];
 *   YIELD_BUS -> stateAfterYield;
 *   stateAfterYield -> CHECK_STATUS [style=dashed];
 *   CHECK_STATUS -> IDLE [label="not valid"];
 *   CHECK_STATUS -> START_RGBC_TRANSACTION [label="valid"];
 *
 *   START_RGBC_TRANSACTION -> WAIT_START_THEN_SEND_ADDRESS_RGBC_WRITE;
 *   WAIT_START_THEN_SEND_ADDRESS_RGBC_WRITE -> WAIT_ADDRESS_ACK_THEN_SEND_REGISTER_RGBC;
 *   WAIT_ADDRESS_ACK_THEN_SEND_REGISTER_RGBC -> WAIT_REGISTER_ACK_THEN_REPEATED_START_RGBC;
 *   WAIT_REGISTER_ACK_THEN_REPEATED_START_RGBC -> WAIT_REPEATED_START_THEN_SEND_ADDRESS_RGBC_READ;
 *   WAIT_REPEATED_START_THEN_SEND_ADDRESS_RGBC_READ -> WAIT_ADDRESS_ACK_THEN_READ_RGBC_BYTE;
 *   WAIT_ADDRESS_ACK_THEN_READ_RGBC_BYTE -> WAIT_RGBC_BYTE_THEN_READ_NEXT_OR_STOP;
 *   WAIT_RGBC_BYTE_THEN_READ_NEXT_OR_STOP -> WAIT_ADDRESS_ACK_THEN_READ_RGBC_BYTE [label="more bytes"];
 *   WAIT_RGBC_BYTE_THEN_READ_NEXT_OR_STOP -> WAIT_STOP_RGBC_TRANSACTION [label="8 bytes"];
 *   WAIT_STOP_RGBC_TRANSACTION -> YIELD_BUS [label="release TWI"];
 *   YIELD_BUS -> stateAfterYield;
 *   stateAfterYield -> PARSE_RGBC [style=dashed];
 *   PARSE_RGBC -> IDLE;
 *   ERROR -> IDLE;
 * }
 * @enddot
 */
void tcs34725UpdateStateMachine(void)
{
    uint32_t timestampNowMicros;

    if(tcs34725.getMicros == 0)
    {
        tcs34725.error = TCS34725_ERROR_TIMEBASE_FUNCTION;
        tcs34725.state = TCS34725_STATE_ERROR;
        return;
    }

    timestampNowMicros = tcs34725.getMicros();

    switch(tcs34725.state)
    {
        case TCS34725_STATE_DISABLED:
            break;

        case TCS34725_STATE_IDLE:
            if(!tcs34725.configured)
                tcs34725.state = TCS34725_STATE_START_POWER_ON_TRANSACTION;
            else if(timestampNowMicros - tcs34725.timestampWaitStartMicros >= tcs34725.timespanWaitMicros)
                tcs34725.state = TCS34725_STATE_START_STATUS_TRANSACTION;
            break;

        case TCS34725_STATE_START_POWER_ON_TRANSACTION:
            if(!acquireOrWait())
                break;
            if(twiInitiateStart())
                tcs34725.state = TCS34725_STATE_WAIT_START_THEN_SEND_ADDRESS_POWER_ON;
            else
                enterError();
            break;

        case TCS34725_STATE_WAIT_START_THEN_SEND_ADDRESS_POWER_ON:
            if(!twiIsFinished())
                break;
            if(twiGetStatus() == TWI_STATUS_START || twiGetStatus() == TWI_STATUS_REP_START)
            {
                if(twiInitiateAddressWrite(TCS34725_ADDRESS))
                    tcs34725.state = TCS34725_STATE_WAIT_ADDRESS_ACK_THEN_SEND_REGISTER_POWER_ON;
                else
                    enterError();
            }
            else
                enterError();
            break;

        case TCS34725_STATE_WAIT_ADDRESS_ACK_THEN_SEND_REGISTER_POWER_ON:
            if(!twiIsFinished())
                break;
            if(twiGetStatus() == TWI_STATUS_MT_SLA_ACK)
            {
                // Select ENABLE register; command bit tells TCS34725 this is a register access.
                if(twiInitiateWrite(command(TCS34725_ENABLE)))
                    tcs34725.state = TCS34725_STATE_WAIT_REGISTER_ACK_THEN_SEND_VALUE_POWER_ON;
                else
                    enterError();
            }
            else
                enterError();
            break;

        case TCS34725_STATE_WAIT_REGISTER_ACK_THEN_SEND_VALUE_POWER_ON:
            if(!twiIsFinished())
                break;
            if(twiGetStatus() == TWI_STATUS_MT_DATA_ACK)
            {
                // Set PON first; the oscillator needs a short delay before AEN is enabled.
                if(twiInitiateWrite(TCS34725_ENABLE_PON))
                    tcs34725.state = TCS34725_STATE_WAIT_VALUE_ACK_THEN_STOP_POWER_ON;
                else
                    enterError();
            }
            else
                enterError();
            break;

        case TCS34725_STATE_WAIT_VALUE_ACK_THEN_STOP_POWER_ON:
            if(!twiIsFinished())
                break;
            if(twiGetStatus() == TWI_STATUS_MT_DATA_ACK)
            {
                if(twiInitiateStop())
                    tcs34725.state = TCS34725_STATE_WAIT_STOP_POWER_ON_TRANSACTION;
                else
                    enterError();
            }
            else
                enterError();
            break;

        case TCS34725_STATE_WAIT_STOP_POWER_ON_TRANSACTION:
            if(twiIsStopPending())
                break;
            tcs34725.timespanWaitMicros = TCS34725_POWER_ON_WAIT_MICROS;
            tcs34725.timestampWaitStartMicros = timestampNowMicros;
            releaseAndYieldTo(TCS34725_STATE_WAIT_POWER_ON);
            break;

        case TCS34725_STATE_WAIT_POWER_ON:
            if(timestampNowMicros - tcs34725.timestampWaitStartMicros >= tcs34725.timespanWaitMicros)
                tcs34725.state = TCS34725_STATE_START_ENABLE_ADC_TRANSACTION;
            break;

        case TCS34725_STATE_START_ENABLE_ADC_TRANSACTION:
            if(!acquireOrWait())
                break;
            if(twiInitiateStart())
                tcs34725.state = TCS34725_STATE_WAIT_START_THEN_SEND_ADDRESS_ENABLE_ADC;
            else
                enterError();
            break;

        case TCS34725_STATE_WAIT_START_THEN_SEND_ADDRESS_ENABLE_ADC:
            if(!twiIsFinished())
                break;
            if(twiGetStatus() == TWI_STATUS_START || twiGetStatus() == TWI_STATUS_REP_START)
            {
                if(twiInitiateAddressWrite(TCS34725_ADDRESS))
                    tcs34725.state = TCS34725_STATE_WAIT_ADDRESS_ACK_THEN_SEND_REGISTER_ENABLE_ADC;
                else
                    enterError();
            }
            else
                enterError();
            break;

        case TCS34725_STATE_WAIT_ADDRESS_ACK_THEN_SEND_REGISTER_ENABLE_ADC:
            if(!twiIsFinished())
                break;
            if(twiGetStatus() == TWI_STATUS_MT_SLA_ACK)
            {
                if(twiInitiateWrite(command(TCS34725_ENABLE)))
                    tcs34725.state = TCS34725_STATE_WAIT_REGISTER_ACK_THEN_SEND_VALUE_ENABLE_ADC;
                else
                    enterError();
            }
            else
                enterError();
            break;

        case TCS34725_STATE_WAIT_REGISTER_ACK_THEN_SEND_VALUE_ENABLE_ADC:
            if(!twiIsFinished())
                break;
            if(twiGetStatus() == TWI_STATUS_MT_DATA_ACK)
            {
                // Keep power on and enable RGBC ADC integration.
                if(twiInitiateWrite(TCS34725_ENABLE_PON | TCS34725_ENABLE_AEN))
                    tcs34725.state = TCS34725_STATE_WAIT_VALUE_ACK_THEN_STOP_ENABLE_ADC;
                else
                    enterError();
            }
            else
                enterError();
            break;

        case TCS34725_STATE_WAIT_VALUE_ACK_THEN_STOP_ENABLE_ADC:
            if(!twiIsFinished())
                break;
            if(twiGetStatus() == TWI_STATUS_MT_DATA_ACK)
            {
                if(twiInitiateStop())
                    tcs34725.state = TCS34725_STATE_WAIT_STOP_ENABLE_ADC_TRANSACTION;
                else
                    enterError();
            }
            else
                enterError();
            break;

        case TCS34725_STATE_WAIT_STOP_ENABLE_ADC_TRANSACTION:
            if(twiIsStopPending())
                break;
            tcs34725.configured = 1;
            tcs34725.timespanWaitMicros = TCS34725_DATA_RETRY_MICROS;
            tcs34725.timestampWaitStartMicros = timestampNowMicros;
            releaseAndYieldTo(TCS34725_STATE_WAIT_MEASUREMENT);
            break;

        case TCS34725_STATE_WAIT_MEASUREMENT:
            if(timestampNowMicros - tcs34725.timestampWaitStartMicros >= tcs34725.timespanWaitMicros)
                tcs34725.state = TCS34725_STATE_IDLE;
            break;

        case TCS34725_STATE_START_STATUS_TRANSACTION:
            if(!acquireOrWait())
                break;
            if(twiInitiateStart())
                tcs34725.state = TCS34725_STATE_WAIT_START_THEN_SEND_ADDRESS_STATUS_WRITE;
            else
                enterError();
            break;

        case TCS34725_STATE_WAIT_START_THEN_SEND_ADDRESS_STATUS_WRITE:
            if(!twiIsFinished())
                break;
            if(twiGetStatus() == TWI_STATUS_START || twiGetStatus() == TWI_STATUS_REP_START)
            {
                if(twiInitiateAddressWrite(TCS34725_ADDRESS))
                    tcs34725.state = TCS34725_STATE_WAIT_ADDRESS_ACK_THEN_SEND_REGISTER_STATUS;
                else
                    enterError();
            }
            else
                enterError();
            break;

        case TCS34725_STATE_WAIT_ADDRESS_ACK_THEN_SEND_REGISTER_STATUS:
            if(!twiIsFinished())
                break;
            if(twiGetStatus() == TWI_STATUS_MT_SLA_ACK)
            {
                // Select STATUS register to poll the AVALID bit.
                if(twiInitiateWrite(command(TCS34725_STATUS)))
                    tcs34725.state = TCS34725_STATE_WAIT_REGISTER_ACK_THEN_REPEATED_START_STATUS;
                else
                    enterError();
            }
            else
                enterError();
            break;

        case TCS34725_STATE_WAIT_REGISTER_ACK_THEN_REPEATED_START_STATUS:
            if(!twiIsFinished())
                break;
            if(twiGetStatus() == TWI_STATUS_MT_DATA_ACK)
            {
                if(twiInitiateRepeatedStart())
                    tcs34725.state = TCS34725_STATE_WAIT_REPEATED_START_THEN_SEND_ADDRESS_STATUS_READ;
                else
                    enterError();
            }
            else
                enterError();
            break;

        case TCS34725_STATE_WAIT_REPEATED_START_THEN_SEND_ADDRESS_STATUS_READ:
            if(!twiIsFinished())
                break;
            if(twiGetStatus() == TWI_STATUS_START || twiGetStatus() == TWI_STATUS_REP_START)
            {
                if(twiInitiateAddressRead(TCS34725_ADDRESS))
                    tcs34725.state = TCS34725_STATE_WAIT_ADDRESS_ACK_THEN_READ_STATUS_BYTE;
                else
                    enterError();
            }
            else
                enterError();
            break;

        case TCS34725_STATE_WAIT_ADDRESS_ACK_THEN_READ_STATUS_BYTE:
            if(!twiIsFinished())
                break;
            if(twiGetStatus() == TWI_STATUS_MR_SLA_ACK)
            {
                if(twiInitiateReadNack())
                    tcs34725.state = TCS34725_STATE_WAIT_STATUS_BYTE_THEN_STOP_STATUS;
                else
                    enterError();
            }
            else
                enterError();
            break;

        case TCS34725_STATE_WAIT_STATUS_BYTE_THEN_STOP_STATUS:
            if(!twiIsFinished())
                break;
            if(twiGetStatus() == TWI_STATUS_MR_DATA_NACK)
            {
                twiSaveLastByte();
                // Receive STATUS; bit 0 signals whether RGBC data is valid.
                tcs34725.status = twiGetLastByte();
                if(twiInitiateStop())
                    tcs34725.state = TCS34725_STATE_WAIT_STOP_STATUS_TRANSACTION;
                else
                    enterError();
            }
            else
                enterError();
            break;

        case TCS34725_STATE_WAIT_STOP_STATUS_TRANSACTION:
            if(twiIsStopPending())
                break;
            releaseAndYieldTo(TCS34725_STATE_CHECK_STATUS);
            break;

        case TCS34725_STATE_CHECK_STATUS:
            if(tcs34725.status & TCS34725_STATUS_AVALID)
                tcs34725.state = TCS34725_STATE_START_RGBC_TRANSACTION;
            else
            {
                tcs34725.timespanWaitMicros = TCS34725_DATA_RETRY_MICROS;
                tcs34725.timestampWaitStartMicros = timestampNowMicros;
                tcs34725.state = TCS34725_STATE_IDLE;
            }
            break;

        case TCS34725_STATE_START_RGBC_TRANSACTION:
            if(!acquireOrWait())
                break;
            if(twiInitiateStart())
                tcs34725.state = TCS34725_STATE_WAIT_START_THEN_SEND_ADDRESS_RGBC_WRITE;
            else
                enterError();
            break;

        case TCS34725_STATE_WAIT_START_THEN_SEND_ADDRESS_RGBC_WRITE:
            if(!twiIsFinished())
                break;
            if(twiGetStatus() == TWI_STATUS_START || twiGetStatus() == TWI_STATUS_REP_START)
            {
                if(twiInitiateAddressWrite(TCS34725_ADDRESS))
                    tcs34725.state = TCS34725_STATE_WAIT_ADDRESS_ACK_THEN_SEND_REGISTER_RGBC;
                else
                    enterError();
            }
            else
                enterError();
            break;

        case TCS34725_STATE_WAIT_ADDRESS_ACK_THEN_SEND_REGISTER_RGBC:
            if(!twiIsFinished())
                break;
            if(twiGetStatus() == TWI_STATUS_MT_SLA_ACK)
            {
                // Select CDATAL and enable auto-increment through C/R/G/B low/high bytes.
                if(twiInitiateWrite(autoIncrementCommand(TCS34725_CDATAL)))
                    tcs34725.state = TCS34725_STATE_WAIT_REGISTER_ACK_THEN_REPEATED_START_RGBC;
                else
                    enterError();
            }
            else
                enterError();
            break;

        case TCS34725_STATE_WAIT_REGISTER_ACK_THEN_REPEATED_START_RGBC:
            if(!twiIsFinished())
                break;
            if(twiGetStatus() == TWI_STATUS_MT_DATA_ACK)
            {
                if(twiInitiateRepeatedStart())
                    tcs34725.state = TCS34725_STATE_WAIT_REPEATED_START_THEN_SEND_ADDRESS_RGBC_READ;
                else
                    enterError();
            }
            else
                enterError();
            break;

        case TCS34725_STATE_WAIT_REPEATED_START_THEN_SEND_ADDRESS_RGBC_READ:
            if(!twiIsFinished())
                break;
            if(twiGetStatus() == TWI_STATUS_START || twiGetStatus() == TWI_STATUS_REP_START)
            {
                if(twiInitiateAddressRead(TCS34725_ADDRESS))
                {
                    tcs34725.rxIndex = 0;
                    tcs34725.state = TCS34725_STATE_WAIT_ADDRESS_ACK_THEN_READ_RGBC_BYTE;
                }
                else
                    enterError();
            }
            else
                enterError();
            break;

        case TCS34725_STATE_WAIT_ADDRESS_ACK_THEN_READ_RGBC_BYTE:
            if(!twiIsFinished())
                break;
            if(twiGetStatus() == TWI_STATUS_MR_SLA_ACK)
            {
                if(tcs34725.rxIndex < TCS34725_RGBC_READ_LENGTH - 1)
                {
                    if(twiInitiateReadAck())
                        tcs34725.state = TCS34725_STATE_WAIT_RGBC_BYTE_THEN_READ_NEXT_OR_STOP;
                    else
                        enterError();
                }
                else
                {
                    if(twiInitiateReadNack())
                        tcs34725.state = TCS34725_STATE_WAIT_RGBC_BYTE_THEN_READ_NEXT_OR_STOP;
                    else
                        enterError();
                }
            }
            else
                enterError();
            break;

        case TCS34725_STATE_WAIT_RGBC_BYTE_THEN_READ_NEXT_OR_STOP:
            if(!twiIsFinished())
                break;
            if(tcs34725.rxIndex < TCS34725_RGBC_READ_LENGTH - 1)
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
            // Receive 8 bytes: C_L, C_H, R_L, R_H, G_L, G_H, B_L, B_H.
            tcs34725.rxBuffer[tcs34725.rxIndex] = twiGetLastByte();
            tcs34725.rxIndex++;
            if(tcs34725.rxIndex < TCS34725_RGBC_READ_LENGTH)
            {
                if(tcs34725.rxIndex < TCS34725_RGBC_READ_LENGTH - 1)
                {
                    if(!twiInitiateReadAck())
                        enterError();
                }
                else if(!twiInitiateReadNack())
                    enterError();
            }
            else if(twiInitiateStop())
                tcs34725.state = TCS34725_STATE_WAIT_STOP_RGBC_TRANSACTION;
            else
                enterError();
            break;

        case TCS34725_STATE_WAIT_STOP_RGBC_TRANSACTION:
            if(twiIsStopPending())
                break;
            releaseAndYieldTo(TCS34725_STATE_PARSE_RGBC);
            break;

        case TCS34725_STATE_PARSE_RGBC:
            parseRgbc();
            tcs34725.error = TCS34725_ERROR_NONE;
            tcs34725.timespanWaitMicros = tcs34725.intervalMicros;
            tcs34725.timestampWaitStartMicros = timestampNowMicros;
            tcs34725.state = TCS34725_STATE_IDLE;
            break;

        case TCS34725_STATE_ERROR:
            if(twiIsStopPending())
                break;
            twiRelease();
            if(timestampNowMicros - tcs34725.timestampWaitStartMicros >= tcs34725.timespanWaitMicros)
                tcs34725.state = TCS34725_STATE_IDLE;
            break;

        case TCS34725_STATE_YIELD_BUS:
            tcs34725.state = tcs34725.stateAfterYield;
            break;

        default:
            enterError();
            break;
    }

    updateDurationMicros(tcs34725.getMicros() - timestampNowMicros);
}

uint16_t tcs34725GetRawR(void)
{
    return tcs34725.r;
}

uint16_t tcs34725GetRawG(void)
{
    return tcs34725.g;
}

uint16_t tcs34725GetRawB(void)
{
    return tcs34725.b;
}

uint16_t tcs34725GetC(void)
{
    return tcs34725.c;
}

uint8_t tcs34725GetNormedR(void)
{
    return tcs34725.rnorm;
}

uint8_t tcs34725GetNormedG(void)
{
    return tcs34725.gnorm;
}

uint8_t tcs34725GetNormedB(void)
{
    return tcs34725.bnorm;
}

uint16_t tcs34725GetColorTemp(void)
{
    return tcs34725.colorTemp;
}

uint16_t tcs34725GetLux(void)
{
    return tcs34725.lux;
}

enum Tcs34725State tcs34725GetState(void)
{
    return tcs34725.state;
}

enum Tcs34725Error tcs34725GetError(void)
{
    return tcs34725.error;
}

uint32_t tcs34725GetUpdateDurationMicros(void)
{
    return tcs34725.updateDurationMicros;
}

