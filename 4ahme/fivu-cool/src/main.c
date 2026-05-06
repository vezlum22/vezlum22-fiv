/**
 * @file main.c
 * @author Vezonik
 * @date 25.01.2026
 * @brief FIVU code
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include <string.h>

#include "cli.h"
#include "avrcomport.h"
#include "timer0.h"
#include "timer1.h"
#include "timer2.h"
#include "cmd.h"
#include "statusbar.h"


CliComPort *uart0 = NULL;


int main()
{
    uint32_t seconds = 0;

    //Create Comport for UART-0 with 76800 baud
    //& --> Adressoperator
    cliCreateComPort(&uart0, avrcomportCreateUartTx(0, 76800));
    cliAddComPort(uart0);
    cliSetStatusBar(uart0, statusBar0);
    cliSetStatusBarFlag(uart0, 1);

    //Clears Screen
    cliClearScreen(uart0);

    //initialises timer 2 for ISR calling
    timer2CTCInit(125);

    sei(); // Global Interrupt enable

    //cliPrintf
    //PSTR legt den String zu Compile-zeit im Programmspeicher an
    //cliPrintf_P(uart0, PSTR("Hello World!\nHow are you today?"));

    cliPrintPrompt(uart0, TXT_GREEN);     //TXT_RED --> ANSI Escape Code

    while (1)
    {
        if(cliProcessRxData(uart0))
        {
            //Executes all CLI-Commands
            cmdExecuteCommand(uart0);
            //Print UART prompt for next command to be entered
            cliPrintPrompt(uart0, TXT_GREEN);
        }
        if(seconds != timer2GetSeconds())
        {
            seconds = timer2GetSeconds();
            cliPrintStatusBar(uart0);
        }    
    }
    return 0;
}

// Interrupt Service Routine
ISR(TIMER2_COMPA_vect)
{
    static uint8_t isrCallCounter = 0;
    static uint16_t milliSecCounter = 0;
    
            //0000 0111
    if((++isrCallCounter & 0x07) == 0)
        milliSecCounter++;

    if(milliSecCounter == 1000)
    {
        timer2IncreaseSeconds();
        milliSecCounter = 0;
    }

    if (cliHasInput(uart0)) // Mask RXC0 Bit in UCSR0A
        cliReceiveByte(uart0);
    


    

}