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

enum MPUState
{
    MPU_STATE_IDLE = 0,
    MPU_STATE_START,
    MPU_STATE_LED_EVEN,
    MPU_STATE_LED_LEFT,
    MPU_STATE_LED_RIGHT,
    MPU_STATE_LED_DOWN,
    MPU_STATE_LED_UP,
    MPU_STATE_DEACTIVE
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

//Initialisierung der Anwendung 
void appInit(enum MPUState state);

//Betrieb der Anwendung
void appUpdateStateMachine();

#if defined(__cplusplus)
} // extern "C"
#endif
