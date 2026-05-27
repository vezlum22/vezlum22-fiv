/**
 * @file twi.h
 * @author JR
 * @date 06.05.2026
 * @version 1.0
 * @brief Non-blocking TWI helper functions
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

#define TWI_STATUS_MASK             0xF8

#define TWI_STATUS_BUS_ERROR        0x00
#define TWI_STATUS_START            0x08
#define TWI_STATUS_REP_START        0x10
#define TWI_STATUS_MT_SLA_ACK       0x18
#define TWI_STATUS_MT_SLA_NACK      0x20
#define TWI_STATUS_MT_DATA_ACK      0x28
#define TWI_STATUS_MT_DATA_NACK     0x30
#define TWI_STATUS_ARB_LOST         0x38
#define TWI_STATUS_MR_SLA_ACK       0x40
#define TWI_STATUS_MR_SLA_NACK      0x48
#define TWI_STATUS_MR_DATA_ACK      0x50
#define TWI_STATUS_MR_DATA_NACK     0x58

/****************************************************/
// GLOBAL ENUMS
/****************************************************/

enum TwiState
{
    TWI_STATE_DISABLED,
    TWI_STATE_READY,
    TWI_STATE_BUSY,
    TWI_STATE_ERROR
};

enum TwiError
{
    TWI_ERROR_NONE,
    TWI_ERROR_PARAM,
    TWI_ERROR_DISABLED,
    TWI_ERROR_BUSY,
    TWI_ERROR_BUS
};

/****************************************************/
// GLOBAL FUNCTIONS
/****************************************************/

/**
 * @brief Initializes the AVR TWI hardware.
 * @param sclFrequency Target SCL clock frequency in Hz.
 * @return 1 on success, 0 on invalid parameters.
 *
 * The implementation uses prescaler 1 and calculates TWBR from F_CPU.
 */
uint8_t twiInit(uint32_t sclFrequency);

/**
 * @brief Disables the TWI peripheral and clears the cooperative ownership flag.
 */
void twiDisable(void);

/**
 * @brief Returns the internal TWI hardware-operation state.
 */
enum TwiState twiGetState(void);

/**
 * @brief Returns the last TWI driver error.
 */
enum TwiError twiGetError(void);

/**
 * @brief Clears the last TWI driver error.
 */
void twiClearError(void);

/**
 * @brief Returns 1 if no TWI hardware operation is currently pending.
 */
uint8_t twiIsReady(void);

/**
 * @brief Acquires the TWI bus for a sensor state machine.
 * @return 1 if the caller may start a transaction, 0 if the bus is owned or disabled.
 *
 * This is a cooperative software lock. It does not start a hardware operation.
 */
uint8_t twiAcquire(void);

/**
 * @brief Releases the cooperative TWI ownership flag.
 */
void twiRelease(void);

/**
 * @brief Returns 1 if a sensor state machine currently owns the TWI bus.
 */
uint8_t twiIsOwned(void);

/**
 * @brief Returns 1 if the current TWI hardware operation has finished.
 */
uint8_t twiIsFinished(void);

/**
 * @brief Returns 1 while a STOP condition is still pending in hardware.
 */
uint8_t twiIsStopPending(void);

/**
 * @brief Returns the masked TWI status code from TWSR.
 */
uint8_t twiGetStatus(void);

/**
 * @brief Initiates a START condition.
 * @return 1 if the request was accepted, not if the START has already completed.
 */
uint8_t twiInitiateStart(void);

/**
 * @brief Initiates a repeated START condition.
 * @return 1 if the request was accepted, not if the repeated START has completed.
 */
uint8_t twiInitiateRepeatedStart(void);

/**
 * @brief Initiates a STOP condition.
 * @return 1 if the request was accepted, not if STOP has already left the bus.
 */
uint8_t twiInitiateStop(void);

/**
 * @brief Initiates sending SLA+W.
 * @param address 7-bit I2C/TWI target address.
 */
uint8_t twiInitiateAddressWrite(uint8_t address);

/**
 * @brief Initiates sending SLA+R.
 * @param address 7-bit I2C/TWI target address.
 */
uint8_t twiInitiateAddressRead(uint8_t address);

/**
 * @brief Initiates writing one data byte.
 */
uint8_t twiInitiateWrite(uint8_t data);

/**
 * @brief Initiates reading one byte and sends ACK afterwards.
 */
uint8_t twiInitiateReadAck(void);

/**
 * @brief Initiates reading one byte and sends NACK afterwards.
 */
uint8_t twiInitiateReadNack(void);

/**
 * @brief Stores TWDR as the last received byte after a finished read operation.
 */
uint8_t twiSaveLastByte(void);

/**
 * @brief Returns the last byte saved by twiSaveLastByte().
 */
uint8_t twiGetLastByte(void);

#if defined(__cplusplus)
} // extern "C"
#endif
