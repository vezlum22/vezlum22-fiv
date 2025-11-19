#include <avr/io.h>

int main (){

        uint8_t c = 0;
        //Uart config: 76000/8/N/1
        //Onboard LED toggles on each character received 
        DDRB |= (1<<PB5);
        //UART-Config
        UBRR0 = 0x0C; //12 for 76800 bit/s with U2X0 = 0
        UCSR0B |= (1<<RXEN0) | (1<<TXEN0);

        while(1){
                if(UCSR0A & (1<<RXC0)){
                        c = UDR0;
                        UDR0 =c;
                        PORTB ^= (1<<PB5); //Toggling with XOR
                }
        }
        return 0;
}