/**
 * @file twi.c
 * @author JR
 * @date 06.05.2026
 * @brief Non-blocking TWI helper functions
 */

/****************************************************/
// INCLUDES
/****************************************************/

#include <avr/io.h>
#include "twi.h"

/****************************************************/
// LOCAL DEFINES
/****************************************************/

#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#define TWI_WRITE                  0
#define TWI_READ                   1
#define TWI_MAX_7BIT_ADDRESS       0x7F

/****************************************************/
// LOCAL ENUMS
/****************************************************/

/****************************************************/
// LOCAL STRUCT TYPE DEFINITION
/****************************************************/

struct Twi
{
    enum TwiState state;
    enum TwiError error;
    uint8_t owned;
    uint8_t lastStatus;
    uint8_t lastByte;
};

/****************************************************/
// LOCAL STATIC STRUCTS and VARIABLES
/****************************************************/

static struct Twi twi =
{
    .state = TWI_STATE_DISABLED,
    .error = TWI_ERROR_NONE,
    .owned = 0,
    .lastStatus = 0,
    .lastByte = 0
};

/****************************************************/
// LOCAL FUNCTIONS
/****************************************************/

static uint8_t operationPending(void)
{
    return (twi.state == TWI_STATE_BUSY && !(TWCR & (1 << TWINT)));
}

static uint8_t setBusyIfUsable(void)
{
    if(twi.state == TWI_STATE_DISABLED)
    {
        twi.error = TWI_ERROR_DISABLED;
        return 0;
    }

    if(operationPending())
    {
        twi.error = TWI_ERROR_BUSY;
        return 0;
    }

    twi.error = TWI_ERROR_NONE;
    twi.state = TWI_STATE_BUSY;
    return 1;
}

static uint8_t initiateAddressAccess(uint8_t address, uint8_t readWriteBit)
{
    if(address > TWI_MAX_7BIT_ADDRESS)
    {
        twi.error = TWI_ERROR_PARAM;
        return 0;
    }

    if(!setBusyIfUsable())
        return 0;

    TWDR = (address << 1) | readWriteBit;
    TWCR = (1 << TWINT) | (1 << TWEN);
    return 1;
}

/****************************************************/
// LOCAL MACROS
/****************************************************/

/****************************************************/
// GLOBAL FUNCTIONS
/****************************************************/

/**
 * @brief TWI module state and ownership overview.
 *
 * The TWI module separates two concepts:
 *
 * - `owned`: cooperative software arbitration between foreground sensor state
 *   machines.
 * - `state`: whether the AVR TWI hardware currently has an operation pending.
 *
 * @dot
 * digraph TWI_OperationModel {
 *   rankdir=LR;
 *   node [shape=box, style="rounded"];
 *
 *   Free [label="owned = 0\nTWI bus available"];
 *   Owned [label="owned = 1\nsensor owns bus"];
 *   Busy [label="TWI_STATE_BUSY\nhardware operation pending"];
 *   Ready [label="TWI_STATE_READY\noperation finished"];
 *   Released [label="owned = 0\nsensor yielded"];
 *
 *   Free -> Owned [label="twiAcquire()"];
 *   Owned -> Busy [label="twiInitiateStart/write/read()"];
 *   Busy -> Ready [label="TWINT set"];
 *   Ready -> Busy [label="next transaction byte"];
 *   Ready -> Released [label="twiInitiateStop() + twiRelease()"];
 *   Released -> Free;
 * }
 * @enddot
 */

uint8_t twiInit(uint32_t sclFrequency)
{
    uint32_t twbrValue;

    if(sclFrequency == 0)
    {
        twi.error = TWI_ERROR_PARAM;
        return 0;
    }

    if(sclFrequency >= F_CPU / 16UL)
    {
        twi.error = TWI_ERROR_PARAM;
        return 0;
    }

    twbrValue = ((F_CPU / sclFrequency) - 16UL) / 2UL;
    if(twbrValue > 255UL)
    {
        twi.error = TWI_ERROR_PARAM;
        return 0;
    }

    TWSR &= ~((1 << TWPS1) | (1 << TWPS0));
    TWBR = (uint8_t)twbrValue;
    TWCR = (1 << TWEN);

    twi.state = TWI_STATE_READY;
    twi.error = TWI_ERROR_NONE;
    twi.owned = 0;
    twi.lastStatus = 0;
    twi.lastByte = 0;
    return 1;
}

void twiDisable(void)
{
    TWCR &= ~(1 << TWEN);
    twi.state = TWI_STATE_DISABLED;
    twi.owned = 0;
}

enum TwiState twiGetState(void)
{
    return twi.state;
}

enum TwiError twiGetError(void)
{
    return twi.error;
}

void twiClearError(void)
{
    twi.error = TWI_ERROR_NONE;
    if(twi.state == TWI_STATE_ERROR)
        twi.state = TWI_STATE_READY;
}

uint8_t twiIsReady(void)
{
    return twi.state == TWI_STATE_READY;
}

uint8_t twiAcquire(void)
{
    if(twi.state == TWI_STATE_DISABLED)
    {
        twi.error = TWI_ERROR_DISABLED;
        return 0;
    }

    if(!twi.owned)
    {
        twi.owned = 1;
        twi.error = TWI_ERROR_NONE;
        return 1;
    }

    twi.error = TWI_ERROR_BUSY;
    return 0;
}

void twiRelease(void)
{
    twi.owned = 0;
}

uint8_t twiIsOwned(void)
{
    return twi.owned;
}

uint8_t twiIsFinished(void)
{
    if(twi.state == TWI_STATE_DISABLED)
        return 0;

    return (TWCR & (1 << TWINT)) ? 1 : 0;
}

uint8_t twiIsStopPending(void)
{
    return (TWCR & (1 << TWSTO)) ? 1 : 0;
}

uint8_t twiGetStatus(void)
{
    twi.lastStatus = TWSR & TWI_STATUS_MASK;

    if(twi.lastStatus == TWI_STATUS_BUS_ERROR || twi.lastStatus == TWI_STATUS_ARB_LOST)
    {
        twi.error = TWI_ERROR_BUS;
        twi.state = TWI_STATE_ERROR;
    }

    return twi.lastStatus;
}

uint8_t twiInitiateStart(void)
{
    if(!setBusyIfUsable())
        return 0;

    TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);
    return 1;
}

uint8_t twiInitiateRepeatedStart(void)
{
    return twiInitiateStart();
}

uint8_t twiInitiateStop(void)
{
    if(twi.state == TWI_STATE_DISABLED)
    {
        twi.error = TWI_ERROR_DISABLED;
        return 0;
    }

    TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN);
    twi.state = TWI_STATE_READY;
    twi.error = TWI_ERROR_NONE;
    return 1;
}

uint8_t twiInitiateAddressWrite(uint8_t address)
{
    return initiateAddressAccess(address, TWI_WRITE);
}

uint8_t twiInitiateAddressRead(uint8_t address)
{
    return initiateAddressAccess(address, TWI_READ);
}

uint8_t twiInitiateWrite(uint8_t data)
{
    if(!setBusyIfUsable())
        return 0;

    TWDR = data;
    TWCR = (1 << TWINT) | (1 << TWEN);
    return 1;
}

uint8_t twiInitiateReadAck(void)
{
    if(!setBusyIfUsable())
        return 0;

    TWCR = (1 << TWINT) | (1 << TWEA) | (1 << TWEN);
    return 1;
}

uint8_t twiInitiateReadNack(void)
{
    if(!setBusyIfUsable())
        return 0;

    TWCR = (1 << TWINT) | (1 << TWEN);
    return 1;
}

uint8_t twiSaveLastByte(void)
{
    if(!twiIsFinished())
        return 0;

    twi.lastByte = TWDR;
    twi.state = TWI_STATE_READY;
    return 1;
}

uint8_t twiGetLastByte(void)
{
    return twi.lastByte;
}
