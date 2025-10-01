#include <Arduino.h>

void setup() {
            //76543210
    PORTB = 0b00100000; //PB5 ein
    DDRB =  0b00100000; //Data direction register 
            //76543210 
    PORTB = 0b00110000; //to additionally activate PB4   
    DDRB =  0b00110000; //Data direction register: output 

    //Alternative Approach for setting PB4:
                    //76543210
    PORTB = PORTB | 0b00010000; //Settings bits always with OR | 
    //         76543210
    //PORTB: 0b00110000
    //       0b00010000 | -> bitwhise or (bitweises ODER)
    //       0b00110000

    PORTB = PORTB | (1 << PB4);
    //            76543210
    //Aim:      0b00010000
    //(1 << PB4)0b00000001 shifting the "1" 4 times to the back
    // 1.       0b00000010
    // 2.       0b00000100
    // 3.       0b00001000
    // 4.       0b00010000      
    PORTB |= (1 << PB4);

    //clearing bits always with AND (&)
    //             76543210
    // Current Val.     0b00110000
    // Bit Mask:        0b11011111
    // 1.               0b00100000 (1 << PB5)
    // 2.               0b11011111 ~(1 << PB5)
    // Aim:             0b00010000 PB5 should be cleared 

    PORTB &= ~(1 << PB5);

    //Button input on PB3
    //Guarantee PB3 being an input 
    DDRB &= ~(1 << PB3);
    //Read an input pin 
    if(PINB & (1 << PB3)){
    
    }
}

void loop() {

}