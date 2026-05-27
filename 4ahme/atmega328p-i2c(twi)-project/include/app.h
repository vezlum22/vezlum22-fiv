/**
 * @file onboardled.h
 * @author vezonik
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

enum MPU6050State
{
    ONBOARDLED_STATE_IDLE = 0,
    ONBOARDLED_STATE_START_1HZ_BLINK,
    ONBOARDLED_STATE_BLINK_LED_ON,
    ONBOARDLED_STATE_BLINK_LED_OFF,
    ONBOARDLED_STATE_DEACTIVE
};

enum MPU6050bV
{
    MPU6050_
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

#if defined(__cplusplus)
} // extern "C"
#endif
