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
#include <avr/interrupt.h>
#include "timer2.h"

/**
 * @brief Verify that the ISR exit capture is the last valid ISR capture index.
 */
_Static_assert(TIMER2_CAPTURE_ISR_MICROS_EXIT == TIMER2_CAPTURE_ISR_MICROS_ENUM_COUNT - 1,
    "TIMER2_CAPTURE_ISR_MICROS_EXIT must be the last valid index before TIMER2_CAPTURE_ISR_MICROS_ENUM_COUNT");

/****************************************************/
// LOCAL DEFINES
/****************************************************/

/****************************************************/
// LOCAL ENUMS
/****************************************************/

/****************************************************/
// LOCAL STRUCT TYPE DEFINITION
/****************************************************/

/**
 * @brief Internal Timer2 timebase and capture state.
 */
struct Timer2
{
    /** Millisecond counter advanced by the Timer2 ISR. */
    volatile uint32_t millis;

    /** Microsecond counter advanced by the Timer2 ISR. */
    volatile uint32_t micros;

    /** Counter used to detect concurrent ISR updates during reads. */
    volatile uint8_t changeFlag;

    /** Second counter advanced by the Timer2 ISR. */
    volatile uint32_t seconds;

    /** Latched flag set when the Timer2 ISR overruns its compare period. */
    volatile uint8_t isrOverrunFlag;

    /** Maximum TCNT2 values captured at configured ISR capture points. */
    volatile uint8_t isrCapture[TIMER2_CAPTURE_ISR_MICROS_ENUM_COUNT];

    /** Millisecond timestamps captured at configured application points. */
    volatile uint32_t captureMillis[TIMER2_CAPTURE_MILLIS_ENUM_COUNT];

    /** Configured Timer2 compare period in microseconds. */
    uint8_t periodMicros;
};

/****************************************************/
// LOCAL STATIC STRUCTS and VARIABLES
/****************************************************/

static struct Timer2 timer2 =
{
    .millis = 0,
    .micros = 0,
    .changeFlag = 0,
    .seconds = 0,
    .isrOverrunFlag = 0,
    .isrCapture = {0},
    .captureMillis = {0},
    .periodMicros = 0,
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

/**
 * @brief Initializes Timer2 to toggle OC2A at 10 kHz.
 */
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

/**
 * @brief Initializes Timer2 to toggle OC2A at the requested frequency.
 *
 * @param fsignal Output signal frequency in Hz.
 */
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

/**
 * @brief Initializes Timer2 CTC mode and enables compare-match interrupts.
 *
 * @param periodMicros Interrupt period in microseconds, clamped to 1..128.
 */
void timer2CTCInit(uint32_t periodMicros)
{
    // Check parameter validity
    if(periodMicros < 1)
        periodMicros = 1;
    if(periodMicros > 128)
        periodMicros = 128;

    // Mode of Operation: CTC
    TCCR2A = (1 << WGM21);

    // Clock Select Bit set to: clk/8
    // fclk/8 = 2 Mhz -> 500 ns per clock
    TCCR2B = (1 << CS21);
    timer2.periodMicros = periodMicros;
    
    // Timer0 Period depends on periodMicros
    OCR2A = (timer2.periodMicros << 1) - 1;

    // Enable Interrupt: TIMER0_COMPA
    TIMSK2 = (1 << OCIE2A);
}

/**
 * @brief Increments the Timer2 microsecond counter by the configured period.
 */
void timer2IncreaseMicros()
{
    timer2.micros += timer2.periodMicros;
    timer2.changeFlag++;
}

/**
 * @brief Gets the current Timer2 microsecond counter including TCNT2 offset.
 *
 * @return Microseconds counted by Timer2.
 */
uint32_t timer2GetMicros()
{
    uint8_t flag;
    uint8_t ticks;
    uint32_t micros;

    do
    {
        flag = timer2.changeFlag;
        micros = timer2.micros;
        ticks = TCNT2;

        if(TIFR2 & (1 << OCF2A))
            micros += timer2.periodMicros;
    }
    while(flag != timer2.changeFlag);

    return micros + ticks / 2;
}

/**
 * @brief Increments the Timer2 millisecond counter.
 */
void timer2IncreaseMillis()
{
    timer2.millis++;
    timer2.changeFlag++;
}

/**
 * @brief Gets the current Timer2 millisecond counter.
 *
 * @return Milliseconds counted by Timer2.
 */
uint32_t timer2GetMillis()
{
    uint8_t flag;
    uint32_t millis;

    do
    {
        flag = timer2.changeFlag;
        millis = timer2.millis;
    }
    while(flag != timer2.changeFlag);

    return millis;    
}

/**
 * @brief Increments the Timer2 second counter.
 */
void timer2IncreaseSeconds()
{
    timer2.seconds++;
    timer2.changeFlag++;
}

/**
 * @brief Gets the current Timer2 second counter.
 *
 * @return Seconds counted by Timer2.
 */
uint32_t timer2GetSeconds()
{
    uint8_t flag;
    uint32_t seconds;

    do
    {
        flag = timer2.changeFlag;
        seconds = timer2.seconds;
    }
    while(flag != timer2.changeFlag);

    return seconds;
}

/**
 * @brief Saves the current TCNT2 value for an ISR capture point.
 */
void timer2SaveIsrCapture(enum Timer2CaptureIsrMicros index)
{
    if(index < 0 || index >= TIMER2_CAPTURE_ISR_MICROS_ENUM_COUNT)
        return;

    // Captures the current counter value of TCNT2
    uint8_t capture = TCNT2;
    // Update only the largest captured
    // value for worst-case timing evaluation.
    if(capture > timer2.isrCapture[index])
        timer2.isrCapture[index] = capture;
}

/**
 * @brief Gets the Timer2 ISR load at an indexed capture point.
 */
uint8_t timer2GetIsrLoad(enum Timer2CaptureIsrMicros index)
{
    if(index >= 0 && index < TIMER2_CAPTURE_ISR_MICROS_ENUM_COUNT)
        return (uint8_t)((100U * timer2.isrCapture[index]) / OCR2A);

    return 0;
}

/**
 * @brief Clears all stored Timer2 ISR captures.
 */
void timer2ClearIsrCaptures()
{
    for(int i = 0; i < TIMER2_CAPTURE_ISR_MICROS_ENUM_COUNT; i++)
        timer2.isrCapture[i] = 0;
}

/**
 * @brief Checks whether the Timer2 ISR overran one compare-match period.
 */
void timer2CheckIsrOverrun()
{
    if(TIFR2 & (1 << OCF2A))
        timer2.isrOverrunFlag = 1;
}

/**
 * @brief Gets the latched Timer2 ISR overrun flag.
 */
uint8_t timer2GetIsrOverrunFlag()
{
    return timer2.isrOverrunFlag;
}

/**
 * @brief Saves the current millisecond counter for a capture point.
 */
void timer2SaveMillisCapture(enum Timer2CaptureMillis index)
{
    if(index < 0 || index >= TIMER2_CAPTURE_MILLIS_ENUM_COUNT)
        return;

    timer2.captureMillis[index] = timer2GetMillis();
}

/**
 * @brief Gets a saved millisecond counter for a capture point.
 */
uint32_t timer2GetMillisCapture(enum Timer2CaptureMillis index)
{
    if(index >= 0 && index < TIMER2_CAPTURE_MILLIS_ENUM_COUNT)
        return timer2.captureMillis[index];
    
    return 0;
}
