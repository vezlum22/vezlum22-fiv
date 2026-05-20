/**
 * @file adc.c
 * @author JR
 * @date 15.04.2026
 * @brief ADC functions
 */

/****************************************************/
// INCLUDES
/****************************************************/

#include <avr/io.h>
#include "adc.h"

/****************************************************/
// LOCAL DEFINES
/****************************************************/

#define NUM_OF_CHANNELS 16

/****************************************************/
// LOCAL ENUMS
/****************************************************/

enum AdcState
{
    ADC_STATE_DISABLED,
    ADC_STATE_ENABLED
};

/****************************************************/
// LOCAL STRUCT TYPE DEFINITION
/****************************************************/

struct Adc
{
    uint8_t state;
    uint8_t setChannel;
    uint8_t sampledChannel;
    uint8_t sample[NUM_OF_CHANNELS];
};

/****************************************************/
// LOCAL STATIC STRUCTS and VARIABLES
/****************************************************/

static struct Adc adc =
{
    .state = ADC_STATE_DISABLED,
    .setChannel = 0,
    .sampledChannel = 0,
    .sample = {0}
};

/****************************************************/
// LOCAL FUNCTIONS
/****************************************************/

/****************************************************/
// LOCAL MACROS
/****************************************************/

/****************************************************/
// GLOBAL FUNCTIONS
/****************************************************/

// Initializes the ADC hardware and internal ADC state
// for conversions with 8-bit resolution
uint8_t adcInit(enum AdcVRef adcVRef, uint8_t didr0)
{
    // Check if RSVD_ADC_VREF (REFS1=1 and REFS0=0) has been handed over
    if(adcVRef == RSVD_ADC_VREF)
        return 0;

    // Set the voltage reference source and select left adjusted results
    ADMUX = adcVRef << 6 | (1 << ADLAR);

    // Disable the digital input buffer while using it as analog input.
    DIDR0 = didr0 & 0x3F;

    // Enable ADC and set prescaler to 128
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
    //0011 1111
    //   3    F
    
    // Starting first conversion on ADC_CH_0 as a dummy conversion
    // to initialise the analog circuitry (takes 25 clocks)
    ADCSRA |= (1 << ADSC);

    // Waiting for first conversion to be finished
    while (ADCSRA & (1 << ADSC));

    adc.state = ADC_STATE_ENABLED;
    return 1;
}

// Disables the ADC.
void adcDisable()
{
    ADCSRA &= ~(1 << ADEN);
    adc.state = ADC_STATE_DISABLED;
}

// Sets the channel to be sampled.
uint8_t adcSetNewChannel(enum AdcChannel adcChannel)
{
    // Check if a reserved channel was handed over
    if(adcChannel >= RSVD_ADC_CH9 && adcChannel <= RSVD_ADC_CH13)
        return 0;
    
    // Check if lower und upper boundaries are violated
    if(adcChannel < ADC_CH_0 || adcChannel > ADC_CH_GND)
        return 0;
    
    ADMUX = (ADMUX & 0xF0) | adcChannel;
    adc.setChannel = adcChannel;
    return 1;
}

// Starts a new analog conversion on a channel
// set by adcSetNewChannel(enum AdcChannel adcChannel).
uint8_t adcStartNewConversion()
{
    // Leave function if ADC is not enabled
    if(adc.state == ADC_STATE_DISABLED)
        return 0;

    // Leave function if conversion is ongoing
    if(ADCSRA & (1 << ADSC))
        return 0;

    // Start Conversion
    ADCSRA |= (1 << ADSC);

    // Save the channel which gets sampled
    // 1.5 ADC clocks after the start of conversion
    adc.sampledChannel = adc.setChannel;
    return 1;
}

// Stores the last conversion result of a channel
// set by adcSetNewChannel(enum AdcChannel adcChannel).
uint8_t adcSaveLastConversion()
{  
    // Leave function if conversion is ongoing
    if(ADCSRA & (1 << ADSC))
        return 0;

    // Only read the hight byte of the conversuion result
    adc.sample[adc.sampledChannel] = ADCH;
    return 1;
}

// Returns the analog conversion result of a channel
// set by adcSetNewChannel(enum AdcChannel adcChannel).
uint8_t adcGetLastConversion()
{
    return adc.sample[adc.sampledChannel];
}

// Returns the last sampled AdcChannel enumerator.
enum AdcChannel adcGetLastChannel()
{
    return adc.sampledChannel;
}
