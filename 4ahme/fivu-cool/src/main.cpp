#include <Arduino.h>

void setup() {
            //76543210
    PORTB = 0b00100000; //PB5 ein
    DDRB = 0b00100000; //Data direction register 
            //76543210 
    PORTB = 0b00110000; //to additionally activate PB4   
    DDRB = 0b00100000; //Data direction register: output 
}

void loop() {

}