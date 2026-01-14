#include <avr/io.h>
#include <avr/interrupt.h>
#include "timer0.h"

int main(){  
//------------------------------------------------------------------UART Konfiguration-----------------------------
    // Uart-Config : 76800 bps, 8N1
    // Onboard-LED toggles on each char received

    DDRB |= (1 << PB5); // Set pin 13 as output (onboard LED)

    // UART-Config
    UBRR0 = 0x0C;                           // 76800 baud at 16 MHz
    UCSR0B |= (1 << RXEN0) | (1 << TXEN0);  // Enable RX and TX
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00); // 8 data bits, no parity, 1 stop bit

    
//------------------------------------------------------------------Timer1 Konfiguration für servo---------------------
    DDRB |= (1 << PB1);                                 // Pin 9 (PB1)(Daten Pin vom Servo) = OC1A output
    TCCR1A = (1 << COM1A1) | (1 << WGM11); // Set OC1A on compare match and WGM11 for Fast PWM
    TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS11); // Fast PWM mode with ICR1 as TOP, prescaler = 8
    ICR1 = 39999; // Set TOP for 50Hz frequency
    OCR1A = 3000; //4000 -> 2ms
                    //3000-> 1.5ms
                    //2000 -> 1.0ms
                    //1000 -> 0.5ms
                    //5000 -> 2.5 ms

//------------------------------------------------------------------Timer0 Konfiguration für UART Empfang-------------
    //Timer0 -> CTC mode (Clear Timer on Compare Match)
    //Period: 125us (8kHz)
    //Enable Interrupt: Timer0_COMPA

    timer0CTCInit();

    sei(); // Befehl damit globale Interrupts aktiviert werden
    while (1){

    }

    return 0;
}

//----------------------------------------------------------------Timer0 COMPA Interrupt Service Routine--------------
// Das wird ausgeführt wenn der Timer0 den wert in OCR0A erreicht hat
ISR(TIMER0_COMPA_vect){

        uint16_t c = 0;
    // Wait for data to be received
        if (UCSR0A & (1 << RXC0)){

            c = UDR0; // Read received byte
            UDR0 = c; // Echo back
            PORTB ^= (1 << PB5); // Toggle onboard LED
            if(c == 'l')
                OCR1A = 1000;
            if(c == 'r')
                OCR1A = 4000;
            if(c == 'm')
                OCR1A = 2000;       //2 clock cycles = 125 ns 
        } 
}