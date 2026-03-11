/**
 * @file main.c
 * @author Luca Vezonik 
 * @date 25.01.2026
 * @brief FIVU code
 */

#include <stdio.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include <string.h>


#include "avrcomport.h"
#include "timer0.h"
#include "timer1.h"
#include "timer2.h"
#include "cli.h"
#include "cmd.h"

CliComPort *uart0 = NULL;

int main(){
    //Create ComPort for uart0 with 76800 Baud
    cliCreateComPort(&uart0,avrcomportCreateUartTx(0,76800));
    cliAddComPort(uart0);

    timer2CTCInit(125);

    sei(); // Global Interrupt enable
    cliPrintPrompt(uart0, TXT_GREEN);
    while (1)
    {
        if(cliProcessRxData(uart0)){ //someone closed their entry with ENTER (\n or \r)

            cmdExecuteCommand(uart0);
            cliPrintPrompt(uart0, TXT_GREEN);
        }
     }
    return 0;
}

// Interrupt Service Routine
ISR(TIMER2_COMPA_vect)
{
    if(cliHasInput(uart0)){
        cliReceiveByte(uart0);
    }
}