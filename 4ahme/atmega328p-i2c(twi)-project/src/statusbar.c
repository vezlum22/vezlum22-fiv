/**
 * @file statusbar.c
 * @author author
 * @date date
 * @brief Status bar output for sensor and runtime diagnostics.
 */

/****************************************************/
// INCLUDES
/****************************************************/

#include "statusbar.h"
#include "timer2.h"
#include "adc.h"
#include "mpu6050.h"

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

/**
 * @brief Padding buffer for status bar line endings.
 *
 * Contains STATUS_BAR_WIDTH spaces followed by a '\0' terminator.
 * Padding is printed by starting at the current character position.
 */
static const char padLine[STATUS_BAR_WIDTH + 1] =
{
    [0 ... STATUS_BAR_WIDTH - 1] = ' ',
};

/****************************************************/
// LOCAL FUNCTIONS
/****************************************************/

/**
 * @brief Pad the current line to the status bar width and print a newline.
 *
 * @param uart0 CLI communication port used for output.
 * @param charsPrinted Number of characters already printed on the current line.
 */
static void printPadLine(CliComPort *uart0, int charsPrinted)
{
    if(charsPrinted < 0)
        charsPrinted = 0;
    if(charsPrinted > STATUS_BAR_WIDTH)
        charsPrinted = STATUS_BAR_WIDTH;

    cliPrintf_P(uart0, PSTR("%s\n"), &padLine[charsPrinted]);
}

/**
 * @brief Scale and round a floating-point value to a signed 16-bit integer.
 *
 * @param value Floating-point value to scale.
 * @param scale Scale factor applied before rounding.
 * @return Scaled and rounded value.
 */
static int16_t scaleFloat(float value, float scale)
{
    if(value >= 0.0f)
        return (int16_t)(value * scale + 0.5f);

    return (int16_t)(value * scale - 0.5f);
}

/**
 * @brief Return the absolute value of a signed 16-bit integer.
 *
 * @param value Input value.
 * @return Absolute value of @p value.
 */
static int16_t abs16(int16_t value)
{
    if(value < 0)
        return -value;

    return value;
}

/**
 * @brief Return the sign character for a signed 16-bit integer.
 *
 * @param value Input value.
 * @return '-' for negative values, '+' for zero and positive values.
 */
static char sign16(int16_t value)
{
    if(value < 0)
        return '-';

    return '+';
}

/****************************************************/
// LOCAL MACROS
/****************************************************/

/****************************************************/
// GLOBAL FUNCTIONS
/****************************************************/

/**
 * @brief Print the complete status bar to a CLI port.
 *
 * The status bar contains runtime data, timer load values, ADC readings,
 * SHT3x measurements, MPU6050 measurements, and TCS34725 measurements.
 *
 * @param uart0 CLI communication port used for output.
 */
void statusBar0(CliComPort *uart0)
{
    uint32_t seconds = timer2GetSeconds();
    uint32_t start = timer2GetMicros();
    uint32_t charsPrinted = 0;
    // Apply status bar formatting
    cliPrintf_P(uart0, PSTR(TXT_COLOR_REVERSE));
    //cliPrintf_P(uart0, PSTR(TXT_DIM));
    // Print status bar
    charsPrinted = cliPrintf_P(uart0, PSTR("Runtime: %02d:%02d:%02d | ISR Load: %2d %% | CLI RX Load: %2d %% | T2 Overrun: %d"),
        (int)seconds / 3600, (int)seconds / 60 % 60, (int)seconds % 60,
        timer2GetIsrLoad(TIMER2_CAPTURE_ISR_MICROS_EXIT),
        timer2GetIsrLoad(TIMER2_CAPTURE_ISR_MICROS_CLI_RX_EXIT) - timer2GetIsrLoad(TIMER2_CAPTURE_ISR_MICROS_CLI_RX_ENTRY),
        timer2GetIsrOverrunFlag());
    //cliPrintf_P(uart0, PSTR(CLEAR_LINE_END "\n"));
    printPadLine(uart0, charsPrinted);

    uint16_t adc0Millivolt = (uint16_t)adcGetConversion(0) * 5000UL / 255;
    uint16_t adc1Millivolt = (uint16_t)adcGetConversion(1) * 5000UL / 255;
    uint16_t channelChangeMicros = (uint16_t)(((timer2GetIsrLoad(TIMER2_CAPTURE_ISR_MICROS_SET_ADC_CH_1) - timer2GetIsrLoad(TIMER2_CAPTURE_ISR_MICROS_CLI_RX_ENTRY)) * 125UL) / 100);
    int16_t mpuAccelX = scaleFloat(mpu6050GetAccelX(), 100.0f);
    int16_t mpuAccelY = scaleFloat(mpu6050GetAccelY(), 100.0f);
    int16_t mpuAccelZ = scaleFloat(mpu6050GetAccelZ(), 100.0f);
    int16_t mpuGyroX = scaleFloat(mpu6050GetGyroX(), 1000.0f);
    int16_t mpuGyroY = scaleFloat(mpu6050GetGyroY(), 1000.0f);
    int16_t mpuGyroZ = scaleFloat(mpu6050GetGyroZ(), 1000.0f);
    int16_t mpuTemp = scaleFloat(mpu6050GetTempC(), 10.0f);
    charsPrinted = cliPrintf_P(uart0, PSTR("ADMUX change after: %d us | ADC_CH_0: %d.%03d V | ADC_CH_1: %d.%03d V"),
        channelChangeMicros,
        adc0Millivolt / 1000, adc0Millivolt % 1000,
        adc1Millivolt / 1000, adc1Millivolt % 1000);
    //cliPrintf_P(uart0, PSTR(CLEAR_LINE_END "\n"));
    printPadLine(uart0, charsPrinted);

    charsPrinted = cliPrintf_P(uart0, PSTR("MPU6050 Update: %d us | Acc X:%c%d.%02d, Y:%c%d.%02d, Z:%c%d.%02d m/s2 | Cal:%2d"),
        (uint16_t) mpu6050GetUpdateDurationMicros(), sign16(mpuAccelX), abs16(mpuAccelX) / 100, abs16(mpuAccelX) % 100,
        sign16(mpuAccelY), abs16(mpuAccelY) / 100, abs16(mpuAccelY) % 100,
        sign16(mpuAccelZ), abs16(mpuAccelZ) / 100, abs16(mpuAccelZ) % 100,
        mpu6050GetCalibrationSamplesLeft());
    //cliPrintf_P(uart0, PSTR(CLEAR_LINE_END "\n"));
    printPadLine(uart0, charsPrinted);

    charsPrinted = cliPrintf_P(uart0, PSTR("MPU6050 Gyro X:%c%d.%03d, Y:%c%d.%03d, Z:%c%d.%03d rad/s | T: %c%d.%01d deg. C | Error: %d"),
        sign16(mpuGyroX), abs16(mpuGyroX) / 1000, abs16(mpuGyroX) % 1000,
        sign16(mpuGyroY), abs16(mpuGyroY) / 1000, abs16(mpuGyroY) % 1000,
        sign16(mpuGyroZ), abs16(mpuGyroZ) / 1000, abs16(mpuGyroZ) % 1000,
        sign16(mpuTemp), abs16(mpuTemp) / 10, abs16(mpuTemp) % 10, mpu6050GetError());
    //cliPrintf_P(uart0, PSTR(CLEAR_LINE_END "\n"));
    printPadLine(uart0, charsPrinted);

    timer2SaveMillisCapture(TIMER2_CAPTURE_MILLIS_MAIN_LOOP_END);
    cliPrintf_P(uart0, PSTR("Code execution times: Statusbar: %lu ms, Main loop: %lu ms"),
        (unsigned long)(timer2GetMicros() - start)/1000,
        (unsigned long)(timer2GetMillisCapture(TIMER2_CAPTURE_MILLIS_MAIN_LOOP_END)
        - timer2GetMillisCapture(TIMER2_CAPTURE_MILLIS_MAIN_LOOP_START)));
    cliPrintf_P(uart0, PSTR(CLEAR_LINE_END "\n"));
    
    // Clear ISR captures
    timer2ClearIsrCaptures();
    // Reset text formatting
    cliPrintf_P(uart0, PSTR(TXT_RESET_FORMAT));
}