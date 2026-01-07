#include <avr/io.h>
#include <avr/delay.h>

int main(){
    //Uart config: 76800/8/N/1
    //Onboard LED toggles on each character received
    uint8_t c;
    DDRB |= (1 << PB5);

    //UART Config
    UBRR0 = 0x0C; //76800 at 16MHz
    UCSR0B |= (1 << RXEN0) | (1 << TXEN0); //Enable RX and TX
    UCSR0C |= (1<< UCSZ01) | (1<<UCSZ00);
    
    //Timer1 config for fast PWM
    DDRB |= (1 << PB1); //Set OC1A (PB1) as output
    TCCR1A |= (1 << COM1A1) | (1 << WGM11); //Clear OC1A on compare match, set at BOTTOM (non-inverting mode)
    TCCR1B |= (1 << WGM13) | (1 << WGM12) | (1 << CS11); //Fast PWM mode 14, prescaler 256
    ICR1 = 39999;  //40000 - 1 -> 20ms
    OCR1A = 3000;   //4000 -> 2ms
                    //3000 -> 1,5ms 
                    //2000 -> 1ms
                    //1000 -> 0,5ms
                    //5000 -> 2,5ms 

// Timer0 -> ctc Modus 
// Period: 125 us
// Enable Interrupt: TIMER0_COMPB  

    /*DDRB |= (1 << PB1); //Set OC1A (PB1) as output
    TCCR1A |= (1 << COM1A0) | (1 << WGM12); //Toggle OC1A on compare match
    TCCR1A |= (1 << WGM12) | (1 << CS12) | (1 << CS10); //CTC mode

    OCR1A = 15623; //1s at 1024 prescaler
    */

    TCCR0A = (1<<WGM01); 
    TCCR0B = (1<<CS01);
    OCR0B = 
    TIMSK0 = 

    while(1){
        if(UCSR0A & (1 << RXC0)){ //Check if data received
            c = UDR0; //Read received data
            UDR0 = c; //Echo back the received data
            PORTB &= ~(1 << PB5); //Toggle onboard LED
            if (c=='l'){
                OCR1A = 1000;
            }
            if (c=='r'){
                OCR1A = 4000;       //2 clk cycles = 125ns 
            }
        }
    }
   

    return 0;
}

ISR(TIMER0_COMPB_vect){

}