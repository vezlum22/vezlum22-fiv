/**
 * @file statusbar.c
 * @author author
 * @date date
 * @brief Status bar functions
 */

/****************************************************/
// INCLUDES
/****************************************************/

#include "statusbar.h"
#include "timer2.h"
#include "adc.h"

/****************************************************/
// LOCAL DEFINES
/****************************************************/

/****************************************************/
// LOCAL ENUMS
/****************************************************/

/****************************************************/
// LOCAL STRUCT TYPE DEFINITION
/****************************************************/

/****************************************************/
// LOCAL STATIC STRUCTS and VARIABLES
/****************************************************/

/****************************************************/
// LOCAL FUNCTIONS
/****************************************************/

/****************************************************/
// LOCAL MACROS
/****************************************************/

/****************************************************/
// GLOBAL FUNCTIONS
/****************************************************/

void statusBar0(CliComPort *uart0)
{
    uint32_t seconds = timer2GetSeconds();
    uint8_t lastSampledChannel = adcGetLastChannel();
    // Apply status bar formatting
    cliPrintf_P(uart0, PSTR(TXT_COLOR_REVERSE));
    // Print status bar
    cliPrintf_P(uart0, PSTR("Runtime: %02d:%02d:%02d | ISR Load: %2d %% | CLI RX Load: %2d %% | T2 Overrun: %d\n"),
        (int)seconds / 3600, (int)seconds / 60 % 60, (int)seconds % 60,
        timer2GetRelDuration(TIMER2_ISR_EXIT),
        timer2GetRelDuration(TIMER2_ISR_CLI_RX_EXIT) - timer2GetRelDuration(TIMER2_ISR_CLI_RX_ENTRY),
        timer2GetOverrunFlag());

    cliPrintf_P(uart0, PSTR("ADC_CH_%d: %.2f V\n"), lastSampledChannel, adcGetLastConversion() * 5.0 / 255);
    // Clear ISR captures
    timer2ClearIsrCaptures();
    // Reset text formatting
    cliPrintf_P(uart0, PSTR(TXT_RESET_FORMAT));
}
