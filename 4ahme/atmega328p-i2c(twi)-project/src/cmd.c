/**
 * @file cmd.c
 * @author JR
 * @date 10.03.2026
 * @brief Execution of cli commands
 */

/****************************************************/
// INCLUDES
/****************************************************/

#include <string.h>
#include <stdlib.h> //for e.g.: atoi
#include <avr/wdt.h>
#include "cli.h"
#include "timer1.h"
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

uint8_t cmdExecuteCommand(CliComPort *cliComPort)
{
    const char *cmd = NULL;
    char ctrlKey = cliGetCtrlKey(cliComPort);

    if(ctrlKey != 0)
    {
        switch(ctrlKey)
        {
            case CTRL_L:
                cliClearScreen(cliComPort);
                return 0;
            default:
                cliPrintf_P(cliComPort, PSTR("CTRL_%c not assigned\n"), ctrlKey - 1 + 'A');
                return 0;
        }
    }

    if((cmd = cliGetFirstToken(cliComPort)) == NULL)
        return 0;

    if (strcmp(cmd, "cls") == 0 || strcmp(cmd, "clear") == 0)
        cliClearScreen(cliComPort);

    else if (strcmp(cmd, "rst") == 0)
    {
        wdt_enable(WDTO_15MS);
        while (1);
    }

    //pwm1a 2500 => timer1PWMInit(2500)
    else if (strcmp(cmd, "pwm1a") == 0)
    {
        const char *param = cliGetNextToken(cliComPort);
        if(param != NULL)
        {
            switch(timer1SetPWMPulse((uint16_t)atoi(param)))
            {
                case TIMER1_PWM_INITIALISED:
                    cliPrintf_P(cliComPort, PSTR("PWM initialised\n"));
                    break;
                case TIMER1_PWM_RUNNING:
                    cliPrintf_P(cliComPort, PSTR("PWM running with pulse width %d\n"), (uint16_t)atoi(param));
                    break;
                case TIMER1_PWM_STOPPED:
                    cliPrintf_P(cliComPort, PSTR("PWM stopped\n"));
                    break;
                case TIMER1_PWM_PARAM_ERROR:
                    cliPrintf_P(cliComPort, PSTR("PWM param error: %d, range[%d;%d]\n"), (uint16_t)atoi(param),
                    TIMER1_PWM_MIN_PULSE, TIMER1_PWM_MAX_PULSE);
                    break;
                default:
                    cliPrintf_P(cliComPort, PSTR("Unknown PWM state\n"));
            }
        }
        else
            cliPrintf_P(cliComPort, PSTR("Parameter missing!\n"));
    }

    else if(strcmp(cmd, "adcch") == 0)
    {
        const char *param = cliGetNextToken(cliComPort);
        if(param != NULL)
        {
            int channel = atoi(param);
            if(!adcSetNewChannel(channel))
                cliPrintf_P(cliComPort, PSTR("ADC channel %d not valid!\n"), channel);
            else
                cliPrintf_P(cliComPort, PSTR("ADC channel %d set successfully!\n"), channel);
        }
        else
            cliPrintf_P(cliComPort, PSTR("Parameter missing!\n"));
    }
    
    else if(strcmp(cmd,"mpu6050")){
        
    }

    else
        cliPrintf_P(cliComPort, PSTR("Unknown command!\n"));
    return 0;
}