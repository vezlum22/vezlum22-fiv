/**
 * @file onboardled.h
 * @author DEU
 * @date 13.05.2026
 * @version 1.0
 * @brief h-template
 *
 * Features:
 *  
 * @copyright
 * Released under the Apache License, Version 2.0, January 2004.
 * http://www.apache.org/licenses/
 */

#pragma once

#if defined(__cplusplus)
extern "C" {
#endif

/****************************************************/
// INCLUDES
/****************************************************/

/****************************************************/
// GLOBAL DEFINES
/****************************************************/

/****************************************************/
// GLOBAL ENUMS
/****************************************************/

enum OnboardledState
{
    ONBOARDLED_STATE_IDLE = 0,
    ONBOARDLED_STATE_START_1HZ_BLINK,
    ONBOARDLED_STATE_BLINK_LED_ON,
    ONBOARDLED_STATE_BLINK_LED_OFF,
    ONBOARDLED_STATE_DEACTIVE
};

/****************************************************/
// GLOBAL STRUCT TYPE DEFINITION
/****************************************************/

/****************************************************/
// GLOBAL STATIC STRUCTS and VARIABLES
/****************************************************/

/****************************************************/
// GLOBAL MACROS
/****************************************************/

/****************************************************/
// GLOBAL FUNCTIONS
/****************************************************/

void onboardledInit(enum OnboardledState state);

void onboardledRunStateMachine();

#if defined(__cplusplus)
} // extern "C"
#endif
