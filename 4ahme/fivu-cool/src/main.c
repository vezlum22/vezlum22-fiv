/**
 * @file main.c
 * @author Luca Vezonik 
 * @date 25.01.2026
 * @brief FIVU code
 */

#include <stdio.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <string.h>

#include "avrcomport.h"
#include "timer0.h"
#include "timer1.h"
#include "timer2.h"
#include "cli.h"

CliComPort *uart0 = NULL;

int main()
{
    // Set PB5 as output to control the Onboard LED
    DDRB |= (1 << PB5);

    cliCreateComPort(&uart0,avrcomportCreateUartTx(0,76800));
    cliAddComPort(uart0);

    timer1PWMInit();

    timer2CTCInit(125);

    sei(); // Global Interrupt enable

    cliPrintf(uart0,"Hello, World!");

    while (1)
    {
        if(cliProcessRxData(uart0)){ //someone closed their entry with ENTER (\n or \r)
            
        }
     }
    return 0;
}

// Interrupt Service Routine
ISR(TIMER2_COMPA_vect)
{
    static char c = 0;
    if(cliHasInput(uart0)){
        cliReceiveByte(uart0);
    }
}