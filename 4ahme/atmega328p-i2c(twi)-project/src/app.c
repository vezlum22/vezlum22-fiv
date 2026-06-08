/**
 * @file onboardled.c
 * @author vezonik
 * @date 13.05.2026
 * @brief Onboard LED functions
 */

/****************************************************/
// INCLUDES
/****************************************************/

#include "app.h"
#include "timer2.h"
#include <avr/io.h>
#include <math.h>
#include "mpu6050.h"

/****************************************************/
// LOCAL DEFINES
/****************************************************/
#define toleranz 0.5
/****************************************************/
// LOCAL ENUMS
/****************************************************/
enum LED
{
    LED_LEFT =1,
    LED_RIGHT,
    LED_DOWN,
    LED_UP
};
/****************************************************/
// LOCAL STRUCT TYPE DEFINITION
/****************************************************/

struct MPU6050
{
    uint8_t state;
    uint32_t timespan;
    uint32_t timestamp;
};

/****************************************************/
// LOCAL STATIC STRUCTS and VARIABLES
/****************************************************/

static struct MPU6050 mpu6050 = 
{
    .state = MPU_STATE_IDLE,
    .timespan = 0,
    .timestamp = 0
};

/****************************************************/
// LOCAL FUNCTIONS
/****************************************************/
void getMpuBVal(){
    if(mpu6050GetAccelX()<toleranz&&mpu6050GetAccelY()<toleranz){
        mpu6050.state = MPU_STATE_LED_EVEN;
    } else if(fabs(mpu6050GetAccelX())>fabs(mpu6050GetAccelY())&&mpu6050GetAccelX>0){
        mpu6050.state = MPU_STATE_LED_LEFT;
    } else if(fabs(mpu6050GetAccelX())>fabs(mpu6050GetAccelY())&&mpu6050GetAccelX<0){
        mpu6050.state = MPU_STATE_LED_RIGHT;
    } else if(fabs(mpu6050GetAccelY())>fabs(mpu6050GetAccelX())&&mpu6050GetAccelY>0){
        mpu6050.state = MPU_STATE_LED_UP;
    } else if(fabs(mpu6050GetAccelY())>fabs(mpu6050GetAccelX())&&mpu6050GetAccelY<0){
        mpu6050.state = MPU_STATE_LED_DOWN;
    }
}

void ledOff(){
    PORTD &= ~(1<<LED_LEFT) & ~(1<<LED_RIGHT) & ~(1<<LED_DOWN) & ~(1<<LED_UP);
}
/****************************************************/
// LOCAL MACROS
/****************************************************/

/****************************************************/
// GLOBAL FUNCTIONS
/****************************************************/

void appInit(enum MPUState state)
{
    ledOff();
    mpu6050.state = state; 
}

void appUpdateStateMachine()
{
    uint32_t timestampNow = timer2GetMillis();
    switch(mpu6050.state)
    {
        case MPU_STATE_IDLE: 
            break;
        case MPU_STATE_START:
            mpu6050.timespan = 500;
            mpu6050.timestamp = timestampNow; 
            getMpuBVal();
            break;
        case MPU_STATE_LED_EVEN: 
            if(timestampNow - mpu6050.timestamp>=mpu6050.timespan){
                ledOff();
                getMpuBVal();
            }
            break;
        case MPU_STATE_LED_LEFT: 
            if(timestampNow - mpu6050.timestamp>=mpu6050.timespan){
                ledOff();
                PORTD |= (1<<LED_LEFT);
                getMpuBVal();
            }
            break;
        case MPU_STATE_LED_RIGHT: 
            if(timestampNow - mpu6050.timestamp>=mpu6050.timespan){
                ledOff();
                PORTD |= (1<<LED_RIGHT);
                getMpuBVal();
            }
            break;
        case MPU_STATE_LED_DOWN: 
            if(timestampNow - mpu6050.timestamp>=mpu6050.timespan){
                ledOff();
                PORTD |= (1<<LED_DOWN);
                getMpuBVal();                
            }
            break;
        case MPU_STATE_LED_UP: 
            if(timestampNow - mpu6050.timestamp>=mpu6050.timespan){
                ledOff();
                PORTD |= (1<<LED_UP);
                getMpuBVal();
            }            
            break;
        case MPU_STATE_DEACTIVE:
            //TODO
            break;
        default: break;
    }
}   