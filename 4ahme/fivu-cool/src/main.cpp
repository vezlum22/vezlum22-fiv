#include <avr/io.h>
#include <string.h>

#define MAX_CHARS 80

int main (){

        int8_t c = 0;

        DDRB |= (1<<PB5);
        //UART Config:
        UBRR0 = 0x0C;                           //Baud Rate eingestellt
        UCSR0B |= (1<<RXEN0) | (1<<TXEN0);      //Empfänger und Sender aktiviert

        while (1){
                if(UCSR0A & (1<<RXC0)){
                        c = UDR0;
                        UDR0 = c;
                        PORTB ^= (1<<PB5);
                }
        }
        return 0;
}