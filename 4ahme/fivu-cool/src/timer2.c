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

/****************************************************/
// LOCAL DEFINES
/****************************************************/

/****************************************************/
// LOCAL ENUMS
/****************************************************/

/****************************************************/
// LOCAL STRUCT TYPE DEFINITION
/****************************************************/
struct Timer2{
    uint32_t seconds;
};
/****************************************************/
// LOCAL STATIC STRUCTS and VARIABLES
/****************************************************/
static struct Timer2 timer2{
    .seconds=0;
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

void timer2CTCInit(uint32_t period_us)
{
    if(period_us <1){
        period_us = 1;
    }else if(period_us>128){
        period_us=128;
    }

    // Mode of Operation: CTC
    TCCR2A = (1 << WGM21);

    // Clock Select Bit set to: clk/8
    // fz/8 = 2 Mhz -> 500 ns per clock
    TCCR2B = (1 << CS21);
    
    // Timer0 Period: 125 us
    // 125 us / 500 ns = 250
    OCR2A = (period_us << 1)-1;    // 250 -1

    // Enable Interrupt: TIMER0_COMPA
    TIMSK2 = (1 << OCIE2A);
}

void timer2IncreaseSeconds(){
    timer2.seconds++;
}

uint32_t timer2GetSeconds(){
    return timer2.seconds;
}