/**
 * @file onboardled.c
 * @author DEU
 * @date 13.05.2026
 * @brief Onboard LED functions
 */

/****************************************************/
// INCLUDES
/****************************************************/

#include "onboardled.h"
#include "timer2.h"

/****************************************************/
// LOCAL DEFINES
/****************************************************/

/****************************************************/
// LOCAL ENUMS
/****************************************************/

/****************************************************/
// LOCAL STRUCT TYPE DEFINITION
/****************************************************/

struct Onboardled 
{
    uint8_t state;
    uint32_t timespan;
    uint32_t timestamp;
};

/****************************************************/
// LOCAL STATIC STRUCTS and VARIABLES
/****************************************************/

static struct Onboardled onboardled = 
{
    .state = ONBOARDLED_STATE_IDLE,
    .timespan = 0,
    .timestamp = 0
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

void onboardledInit(enum OnboardledState state)
{
    DDRB|=(1<<PB5);
    onboardled.state = state;
}

void onboardledRunStateMachine()
{
    uint32_t timestampNow = timer2GetMillis();
    switch(onboardled.state)
    {
        case ONBOARDLED_STATE_IDLE:

            break;
        case ONBOARDLED_STATE_START_1HZ_BLINK:
            onboardled.timespan = 500;
            onboardled.timestamp = timestampNow;
            PORTB|=(1<<PB5);
            onboardled.state = ONBOARDLED_STATE_BLINK_LED_ON;
            break;
        case ONBOARDLED_STATE_BLINK_LED_OFF:
            if(timestampNow - onboardled.timestamp >= onboardled.timespan){
                onboardled.timestamp = timestampNow;
                PORTB |= (1<<PB5);
                onboardled.state = ONBOARDLED_STATE_BLINK_LED_ON;
            }
            break;
        case ONBOARDLED_STATE_BLINK_LED_ON:
            if(timestampNow - onboardled.timestamp >= onboardled.timespan){
                onboardled.timestamp = timestampNow;
                PORTB &= ~(1<<PB5);
                onboardled.state = ONBOARDLED_STATE_BLINK_LED_OFF;
            }
            break;
        case ONBOARDLED_STATE_DEACTIVE:
            PORTB &= ~(1<<PB5);
            onboardled.state = ONBOARDLED_STATE_IDLE;
            break;

        default:
            break;
    }
}