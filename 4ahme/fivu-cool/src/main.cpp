#include <avr/io.h>
#include <string.h>

#define MAX_CHARS 80

int main (){

        char recString[MAX_CHARS] = "";
        uint8_t charCount = 0;
        //Uart config: 76000/8/N/1
        //Onboard LED toggles on each character received 
        DDRB |= (1<<PB5);
        //UART-Config
        UBRR0 = 0x0C; //12 for 76800 bit/s with U2X0 = 0
        UCSR0B |= (1<<RXEN0) | (1<<TXEN0);

        while(1){
                if(UCSR0A & (1<<RXC0)){
                        recString[charCount]=UDR0; //RCX0 is set to zero (0) when 09r0 is read
                        UDR0 = recString[charCount];  //charCount ++ praefix



                        if (recString[charCount] == '\r' ){
                                recString[charCount] = 0;
                                if(strcmp(recString, "TPB5")==0)
                                        PORTB ^= (1<<PB5); //toggle mit xor    
                       
                              
                        } 
                        if(++charCount == 80)
                                charCount = 0; 
                        
                        if(recString[charCount-1] == 0x7F){
                                while (!(UCSR0A & (1<<UDRE0)));
                                UDR0 = 'A';
                                charCount -= 2;
                                recString[charCount] = 0; 
                                recString[charCount+1] = 0; 
                        }

                                
                }
        }
        return 0;
}