/**
 * @file timer2.c
 * @author JR
 * @date 21.01.2026
 * @brief Timer2 functions
 */

/****************************************************/
// INCLUDES
/****************************************************/

#include <avr/io.h>
#include "timer2.h"

// Verify at compile time that TIMER2_ISR_EXIT is the
// last valid capture index before TIMER2_ISR_CAPTURE_COUNT
_Static_assert(TIMER2_ISR_EXIT == TIMER2_ISR_CAPTURE_COUNT - 1,
    "TIMER2_ISR_EXIT must be the last valid index before TIMER2_ISR_CAPTURE_COUNT");

/****************************************************/
// LOCAL DEFINES
/****************************************************/

/****************************************************/
// LOCAL ENUMS
/****************************************************/

/****************************************************/
// LOCAL STRUCT TYPE DEFINITION
/****************************************************/

struct Timer2
{
    volatile uint32_t milliSeconds;
    volatile uint32_t seconds;
    volatile uint8_t changeFlag;
    volatile uint8_t isrOverrunFlag;
    volatile uint8_t capture[TIMER2_ISR_CAPTURE_COUNT];
};

/****************************************************/
// LOCAL STATIC STRUCTS and VARIABLES
/****************************************************/

static struct Timer2 timer2 =
{
    .milliSeconds = 0,
    .seconds = 0,
    .changeFlag = 0,
    .isrOverrunFlag = 0,
    .capture = {0}
};

/****************************************************/
// LOCAL FUNCTIONS
/****************************************************/

/****************************************************/
// LOCAL MACROS
/****************************************************/

/****************************************************/
// GLOBAL FUNCTIONS
/****************************************************/

// Initialises Timer2 to toggle OC2A
// with a frequency of 10 kHz
void timer2_OC2A_Toggle10k()
{
    // Set Pin of PB3 as output
    DDRB |= (1 << PB3);

    // Toggle OC2A on Compare Match
    // Automatically switches OC2A to Pin of PB3
    TCCR2A = (1 << COM2A0);

    // Mode of Operation: CTC
    TCCR2A |= (1 << WGM21);

    // Tsignal/2 = 50 µs
    // fz   = 16 MHz -> 62,5 ns * 256 -> 16 us
    // fz/8 =  2 Mhz -> 500 ns * 256 = 128 us
    // 50 µs / 500 ns = 100; OCR2A = 100 - 1 = 99
    OCR2A = 99;

    //  Clock Select Bit set to: clk/8
    TCCR2B = (1 << CS21);
}

// Initialises Timer2 to toggle OC2A
// with a frequency set by fsignal
void timer2_OC2A_SetToggleFrequency(uint16_t fsignal)
{
    // Pin of PB3 set as output
    DDRB |= (1 << PB3);

    // Toggle OC2A on Compare Match
    // Automatically switches OC2A to Pin of PB3
    TCCR2A = (1 << COM2A0);

    // Mode of Operation: CTC
    TCCR2A |= (1 << WGM21);

    // Tsignal/2 = 50 µs
    // fz   = 16 MHz -> 62,5 ns * 256 -> 16 us
    // fz/8 =  2 Mhz -> 500 ns * 256 = 128 us
    if(fsignal)
        OCR2A = F_CPU / (2 * 8 * fsignal) - 1;

    //  Clock Select Bit set to: clk/8
    TCCR2B = (1 << CS21);
}

// Initialises Timer2 to generate IRQs depending on
// period_us
void timer2CTCInit(uint32_t period_us)
{
    // Check parameter validity
    if(period_us < 1)
        period_us = 1;
    if(period_us > 128)
        period_us = 128;

    // Mode of Operation: CTC
    TCCR2A = (1 << WGM21);

    // Clock Select Bit set to: clk/8
    // fclk/8 = 2 Mhz -> 500 ns per clock
    TCCR2B = (1 << CS21);
    
    // Timer0 Period depends on period_us
    OCR2A = (period_us << 1) - 1;    // 250 -1

    // Enable Interrupt: TIMER0_COMPA
    TIMSK2 = (1 << OCIE2A);
}

void timer2IncreaseMillis()
{
    timer2.milliSeconds++;
    timer2.changeFlag++;
}

// Gets the current value of timer2.milliSeconds
uint32_t timer2GetMillis()
{
    uint8_t changeFlag;
    uint32_t milliSeconds;

    do{
        changeFlag = timer2.changeFlag;
        milliSeconds = timer2.milliSeconds; //Could be torn by ISR
    }while(changeFlag != timer2.changeFlag);

    return timer2.milliSeconds;
}

// Sets value of timer2.seconds by increasing it
void timer2IncreaseSeconds()
{
    timer2.seconds++;
    timer2.changeFlag++;
}

// Gets the current value of timer2.seconds
uint32_t timer2GetSeconds()
{
    uint8_t changeFlag;
    uint32_t seconds;

    do{
        changeFlag = timer2.changeFlag;
        seconds = timer2.seconds; //Could be torn by ISR
    }while(changeFlag != timer2.changeFlag);

    return seconds;
}

// Captures the current value of TCNT2 and stores it in
// timer2.capture[index] if it is the largest value seen so far.
void timer2Capture(enum Timer2IsrCapture index)
{
    if(index >= TIMER2_ISR_CAPTURE_COUNT)
        return;

    // Captures the current counter value of TCNT2
    uint8_t capture = TCNT2;
    // Update only the largest captured
    // value for worst-case timing evaluation.
    if(capture > timer2.capture[index])
        timer2.capture[index] = capture;
}

// Returns the duration from Timer2 ISR entry to the indexed
// capture, relative to the maximum available ISR time
uint16_t timer2GetRelDuration(enum Timer2IsrCapture index)
{
    if (index < TIMER2_ISR_CAPTURE_COUNT)
        return (100U * timer2.capture[index] / OCR2A);
    return 0xFFFF;
}

// Clears all captures taken
void timer2ClearIsrCaptures()
{
    for(int i = 0; i < TIMER2_ISR_CAPTURE_COUNT; i++)
        timer2.capture[i] = 0;
}

// Indicates whether the ISR took longer to run than one
// Timer2 compare-match period. There is no reset function
// because if isrOverrunFlag is ever set to 1, the ISR
// should be shortened.
void timer2CheckOverrun()
{
    if(TIFR2 & (1 << OCF2A))
        timer2.isrOverrunFlag = 1;
}

// Returns the isrOverrunFlag
uint8_t timer2GetOverrunFlag()
{
    return timer2.isrOverrunFlag;
}
