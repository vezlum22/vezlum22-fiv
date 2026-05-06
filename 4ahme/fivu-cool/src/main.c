/**
 * @file main.c
 * @author JR
 * @date 20.04.2026
 * @brief FIVU code
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include <string.h>

#include "cli.h"
#include "cmd.h"
#include "avrcomport.h"
#include "timer0.h"
#include "timer1.h"
#include "timer2.h"
#include "statusbar.h"
#include "adc.h"

CliComPort *uart0 = NULL; 

int main()
{
    uint32_t seconds = 0;
    // Create ComPort for UART0 with 76800
    cliCreateComPort(&uart0, avrcomportCreateUartTx(0, 76800));
    cliAddComPort(uart0);
    cliSetStatusBar(uart0, statusBar0);
    cliSetStatusBarFlag(uart0, 1);

    // Initialise the ADC unit
    adcInit(ADC_VREF_AVCC, (1 << ADC0D));

    // Initialise Timer2 for ISR calling
    timer2CTCInit(125);

    // Global Interrupt enable
    sei();

    //TXT_GREEN = ANSI Escape Code
    cliPrintPrompt(uart0, TXT_GREEN);

    while (1)
    {
        //If cliProcessRxData() returns 1, a received command can be exectuted
        if(cliProcessRxData(uart0)) //somebody closed their entry with ENTER (\n, \r \n\r)
        {
            //Execute all CLI-commands
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

    // Count milli seconds
    if((++isrCallCounter & 0x07) == 0)
        milliSecCounter++;

    adcSaveLastConversion();
    adcStartNewConversion();

    // Count seconds
    if(milliSecCounter == 1000)
    {
        timer2IncreaseSeconds();
        milliSecCounter = 0;
    }

    timer2Capture(TIMER2_ISR_CLI_RX_ENTRY);
    // Check if CLI has input
    if(cliHasInput(uart0))
        cliReceiveByte(uart0);
    timer2Capture(TIMER2_ISR_CLI_RX_EXIT);

    // Check if ISR took longer than allowed
    timer2CheckOverrun();

    // Capture ISR execution time
    timer2Capture(TIMER2_ISR_EXIT);
}