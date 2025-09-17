#include <Arduino.h>

void setup() {
            //76543210
    PORTB = 0b00100000; //PB5 ein
    DDRB = 0b00100000; //Data direction register 
}

void loop() {

}