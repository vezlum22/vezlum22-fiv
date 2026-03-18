/**
 * @file template.c
 * @author author
 * @date date
 * @brief c-template
 */

/****************************************************/
// INCLUDES
/****************************************************/
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include <string.h>
#include <stdlib.h>

#include "cli.h"
#include "timer1.h"
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
uint8_t cmdExecuteCommand(CliComPort *cliComPort){

    const char *cmd = NULL;
    char ctrlKey = cliGetCtrlKey(cliComPort);

    if(ctrlKey != 0){
        switch (ctrlKey){
        case CTRL_L:
            cliClearScreen(cliComPort);
            return 0;
        default:
            cliPrintf_P(cliComPort,PSTR("CRTL_%c not assigned\n"),ctrlKey - 1 + 'A');
            break;
        }
    }

    if((cmd = cliGetFirstToken(cliComPort))==NULL){
        return 0;
    }

    if(strcmp(cmd,"cls")==0||strcmp(cmd,"clear")==0){
        cliClearScreen(cliComPort);
    } else if(strcmp(cmd,"rst")==0){
        wdt_enable(WDTO_30MS);
        while(1);
    }else if(strcmp(cmd,"pwm1a")==0){ //pwm1a 2500 => timer1PWMinit(2500)
        const char *param = cliGetNextToken(cliComPort);
        if(param!=NULL){
            switch(timer1SetPWMPulse((uint16_t)atoi(param))){
                case TIMER1_PWM_INITIALISED: cliPrintf(cliComPort,PSTR("PWM initialised\n"));break;
                case TIMER1_PWM_RUNNING: cliPrintf(cliComPort,PSTR("PWM running with pulse width %d\n",(uint16_t)atoi(param))); break;
                case TIMER1_PWM_STOPPED: cliPrintf(cliComPort,PSTR("PWM stopped\n"));break;
                case TIMER1_PWM_PARAM_ERROR: cliPrintf(cliComPort,PSTR("PWM param error: %d , range[%d;%d]",(uint16_t)atoi(param),TIMER1_PWM_MIN_PULSE,TIMER1_PWM_MIN_PULSE));break;
                default: cliPrintf(cliComPort,PSTR("Unknown PWM state\n"));
            }
        }else{
            cliPrintf_P(cliComPort,PSTR("Parameter Missing!\n"));
        }
    }else{
        cliPrintf_P(cliComPort,PSTR("Unknown command!!!!!!!!!\n"));
    }
    return 0; 
}