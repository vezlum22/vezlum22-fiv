/**
 * @file timer1.c
 * @author JR
 * @date 21.01.2026
 * @brief Timer1 functions
 */

/****************************************************/
// INCLUDES
/****************************************************/

#include <avr/io.h>
#include "timer1.h"

/****************************************************/
// LOCAL DEFINES
/****************************************************/

/****************************************************/
// LOCAL ENUMS
/****************************************************/

/****************************************************/
// LOCAL STRUCT TYPE DEFINITION
/****************************************************/

/****************************************************/
// LOCAL STATIC STRUCTS and VARIABLES
/****************************************************/

/****************************************************/
// LOCAL FUNCTIONS
/****************************************************/

/****************************************************/
// LOCAL MACROS
/****************************************************/

/****************************************************/
// GLOBAL FUNCTIONS
/****************************************************/

// Initialises Timer1 to output a PWM servo signal
// with a frequency of 50 Hz (T = 20 ms)
void timer1PWMInit(uint16_t ocr1a)
{
    // Set Pin of PB1 as output
    DDRB |= (1 << PB1); 

    // Clear OC1A on Compare Match, set 
    // OC1A at BOTTOM (non-inverting mode)
    TCCR1A = (1 << COM1A1);
    
    // Mode of Operation:
    // Fast PWM, TOP value in ICR1
    // WGM13, WGM12, WGM11 to be set
    TCCR1A |= (1 << WGM11);
    TCCR1B = (1 << WGM13) | (1 << WGM12);

    // Clock Select Bit set to: clk/8
    // fz/8 = 2 Mhz -> 500 ns per clock
    TCCR1B |= (1 << CS11);

    // PWM-Period T = 20 ms / 500 ns = 40000
    ICR1 = 39999; //40000 - 1

    // PWM-Pulse-Width
    OCR1A = ocr1a; // 5000 * 500 ns = 2.5 ms
                  // 4000 * 500 ns = 2ms
                  // 3000 * 500 ns = 1.5 ms
                  // 2000 * 500 ns = 1.0 ms
                  // 1000 * 500 ns = 0.5 ms
}

//Initalizes Timer 1 to toggle PB1 every second 
void timer1_OC1A_Toggle1s(){
    // Timer1-Config
    DDRB |= (1 << PB1);
    TCCR1A |= (1 << COM1A0);
    TCCR1B |= (1 << WGM12) | (1 << CS12) | (1 << CS10);
    OCR1A = 15624;
}