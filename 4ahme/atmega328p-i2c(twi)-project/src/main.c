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
#include "twi.h"
#include "mpu6050.h"

CliComPort *uart0 = NULL;

/**
 * @brief Firmware entry point.
 *
 * Initializes UART/CLI, ADC, TWI, all sensor state machines, and Timer2. The
 * foreground loop cooperatively advances each sensor state machine and services
 * CLI/status output.
 *
 * @dot
 * digraph MainLoop {
 *   rankdir=LR;
 *   node [shape=box, style="rounded"];
 *
 *   Start [label="init peripherals"];
 *   TCS [label="tcs34725UpdateStateMachine()"];
 *   SHT [label="sht3xUpdateStateMachine()"];
 *   MPU [label="mpu6050UpdateStateMachine()"];
 *   CLI [label="cliProcessRxData()"];
 *   Status [label="cliPrintStatusBar()"];
 *
 *   Start -> TCS;
 *   TCS -> SHT;
 *   SHT -> MPU;
 *   MPU -> CLI;
 *   CLI -> Status;
 *   Status -> TCS [label="while(1)"];
 * }
 * @enddot
 */
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

    // Initialise TWI
    twiInit(100000UL);

    // Initialise the MPU6050
    mpu6050Init(MPU6050_DEFAULT_ADDRESS, 100000UL, timer2GetMicros);
    mpu6050StartCalibration(MPU6050_DEFAULT_CALIBRATION_SAMPLES);
    appInit();
    // Initialise Timer2 for ISR calling
    timer2CTCInit(125);

    // Global Interrupt enable
    sei();

    // TXT_GREEN = ANSI Escape Code
    cliPrintPrompt(uart0, TXT_GREEN);

    while (1)
    {
        timer2SaveMillisCapture(TIMER2_CAPTURE_MILLIS_MAIN_LOOP_START);
        mpu6050UpdateStateMachine();
        appUpdateStateMachine();

        // If cliProcessRxData() returns 1, a received command can be exectuted
        if (cliProcessRxData(uart0)) // somebody closed their entry with ENTER (\n, \r \n\r)
        {
            // Execute all CLI-commands
            cmdExecuteCommand(uart0);
            // Print UART prompt for next command to be entered
            cliPrintPrompt(uart0, TXT_GREEN);
        }

        if (seconds != timer2GetSeconds())
        {
            seconds = timer2GetSeconds();
            cliPrintStatusBar(uart0);
        }
    }
    return 0;
}

/**
 * @brief Timer2 compare-match interrupt service routine.
 *
 * Performs periodic ADC sampling/channel switching, CLI RX polling, millisecond
 * timebase maintenance, and ISR timing capture.
 */
ISR(TIMER2_COMPA_vect)
{
    static uint8_t isrCallCounter = 0;
    static uint16_t milliSecCounter = 0;

    adcSaveLastConversion();
    adcStartNewConversion();

    timer2IncreaseMicros();

    timer2SaveIsrCapture(TIMER2_CAPTURE_ISR_MICROS_CLI_RX_ENTRY);
    // Check if CLI has input
    if (cliHasInput(uart0))
        cliReceiveByte(uart0);
    timer2SaveIsrCapture(TIMER2_CAPTURE_ISR_MICROS_CLI_RX_EXIT);

    switch (++isrCallCounter & 0x07)
    {
    // Count milli seconds
    case 0:
        milliSecCounter++;
        timer2IncreaseMillis();
        // Count seconds
        if (milliSecCounter == 1000)
        {
            timer2IncreaseSeconds();
            milliSecCounter = 0;
        }
        timer2SaveIsrCapture(TIMER2_CAPTURE_ISR_MICROS_SET_ADC_CH_1);
        adcSetNewChannel(ADC_CH_1);
        break;

    case 1:
        timer2SaveIsrCapture(TIMER2_CAPTURE_ISR_MICROS_SET_ADC_CH_0);
        adcSetNewChannel(ADC_CH_0);
        break;

    case 2:
        timer2SaveIsrCapture(TIMER2_CAPTURE_ISR_MICROS_SET_ADC_CH_1);
        adcSetNewChannel(ADC_CH_1);
        break;

    case 3:
        timer2SaveIsrCapture(TIMER2_CAPTURE_ISR_MICROS_SET_ADC_CH_0);
        adcSetNewChannel(ADC_CH_0);
        break;

    case 4:
        timer2SaveIsrCapture(TIMER2_CAPTURE_ISR_MICROS_SET_ADC_CH_1);
        adcSetNewChannel(ADC_CH_1);
        break;
    case 5:
        timer2SaveIsrCapture(TIMER2_CAPTURE_ISR_MICROS_SET_ADC_CH_0);
        adcSetNewChannel(ADC_CH_0);
        break;

    case 6:
        timer2SaveIsrCapture(TIMER2_CAPTURE_ISR_MICROS_SET_ADC_CH_1);
        adcSetNewChannel(ADC_CH_1);
        break;

    case 7:
        timer2SaveIsrCapture(TIMER2_CAPTURE_ISR_MICROS_SET_ADC_CH_0);
        adcSetNewChannel(ADC_CH_0);
        break;
    }   

    // Check if ISR took longer than allowed
    timer2CheckIsrOverrun();

    // Capture ISR execution time
    timer2SaveIsrCapture(TIMER2_CAPTURE_ISR_MICROS_EXIT);
}
