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
            timer1PWMInit((uint16_t)atoi(param));
        }else{
            cliPrintf_P(cliComPort,PSTR("Parameter Missing!\n"));
        }
    }else{
        cliPrintf_P(cliComPort,PSTR("Unknown command!!!!!!!!!\n"));
    }
    return 0; 
}