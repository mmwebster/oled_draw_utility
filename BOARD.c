/*
 * File:   BOARD.h
 * Author: Max Dunne
 *
 * Created on December 19, 2012, 2:08 PM
 */

#include "BOARD.h"

// Microchip Libraries
#include <xc.h>
#include <plib.h>
#include <peripheral/osc.h>
#include <peripheral/system.h>
#include <stdint.h>
#include <stdio.h>

/*******************************************************************************
 * PRAGMAS                                                                     *
 ******************************************************************************/
// Configuration Bits
// SYSCLK = 80MHz
// PBCLK  = 20MHz
// using POSC w/ PLL, XT mode
#pragma config FPLLIDIV   = DIV_2     // Set the PLL input divider to 2, seems to
#pragma config IESO       = OFF       // Internal/External Switch
#pragma config POSCMOD    = XT        // Primary Oscillator Configuration for XT osc mode
#pragma config OSCIOFNC   = OFF       // Disable clock signal output
#pragma config FCKSM      = CSECMD    // Clock Switching and Monitor Selection
#pragma config WDTPS      = PS1       // Specify the watchdog timer interval (unused)
#pragma config FWDTEN     = OFF       // Disable the watchdog timer
#pragma config ICESEL     = ICS_PGx2  // Allow for debugging with the Uno32
#pragma config PWP        = OFF       // Keep the program flash writeable
#pragma config BWP        = OFF       // Keep the boot flash writeable
#pragma config CP         = OFF       // Disable code protect

/*******************************************************************************
 * PRIVATE #DEFINES                                                            *
 ******************************************************************************/
#define SYSTEM_CLOCK 80000000L
#define PB_CLOCK (SYSTEM_CLOCK / 4)
#define QUEUESIZE 512
#define TurnOffAndClearInterrupt(Name) INTEnable(Name,INT_DISABLED); INTClearFlag(Name)


#define LAB10_READ_OVERWRITE
/*******************************************************************************
 * PRIVATE DATATYPES                                                           *
 ******************************************************************************/



/*******************************************************************************
 * PRIVATE FUNCTION PROTOTYPES                                                 *
 ******************************************************************************/
void SERIAL_Init(void);

/*******************************************************************************
 * PRIVATE VARIABLES                                                           *
 ******************************************************************************/


/*******************************************************************************
 * PUBLIC FUNCTIONS                                                            *
 ******************************************************************************/

/**
 * Function: BOARD_Init(void)
 * @param None
 * @return None
 * @brief Initializes the board for 80MHz SYSCLK and 20MHz PBCLK.
 */
void BOARD_Init()
{
    // Set the PB divider to its maximum before altering SYSCLK, ensure that PB limits are observed
    OSCSetPBDIV(OSC_PB_DIV_8);

    // Configure the system for the specified 80MHz clock
    // Changse the flash wait state, RAM wait state, and enable prefetch cache
    SYSTEMConfig(SYSTEM_CLOCK, SYS_CFG_WAIT_STATES | SYS_CFG_PCACHE);

    // Configure for performance at 80MHz
    SYSTEMConfigPerformance(SYSTEM_CLOCK);

    // Use POSC, XT with PLL, XT specified above in config
    // PLL multiplier of 20
    // PLL postdiv of 1
    // FRC postdiv of 1, not using FRC so value is inconsequential
    OSCConfig(OSC_POSC_PLL, OSC_PLL_MULT_20, OSC_PLL_POST_1, OSC_FRC_POST_1);

    // Set PBCLK to 20MHz
    OSCSetPBDIV(OSC_PB_DIV_4);

    // Disable the secondary oscillator
    mOSCDisableSOSC();

    //seeds the random number generator with the time
    char seed1[] = __TIME__;
    unsigned int seed2 = (((unsigned int) (seed1[7] ^ seed1[2])) << 8) | ((unsigned int) (seed1[4] ^ seed1[6]));
    srand(seed2);

    //enables the interrupt system in the new style
    INTConfigureSystem(INT_SYSTEM_CONFIG_MULT_VECTOR);
    INTEnableInterrupts();
    // Initialize for serial
    SERIAL_Init();
}

/**
 * Function: BOARD_End(void)
 * @param None
 * @return None
 * @brief shuts down all peripherals except for serial and A/D. Turns all pins
 * into input
 * @author Max Dunne, 2013.09.20  */
void BOARD_End()
{

    // disable timer interrupts, clear flags and turn off module
    TurnOffAndClearInterrupt(INT_T1);
    TurnOffAndClearInterrupt(INT_T1);
    TurnOffAndClearInterrupt(INT_T2);
    TurnOffAndClearInterrupt(INT_T3);
    TurnOffAndClearInterrupt(INT_T4);
    TurnOffAndClearInterrupt(INT_T5);
    T1CON = 0;
    T2CON = 0;
    T3CON = 0;
    T4CON = 0;
    T5CON = 0;


    // disable input capture interrupts, clear flags and turn off module
    TurnOffAndClearInterrupt(INT_IC1);
    TurnOffAndClearInterrupt(INT_IC2);
    TurnOffAndClearInterrupt(INT_IC3);
    TurnOffAndClearInterrupt(INT_IC4);
    TurnOffAndClearInterrupt(INT_IC5);
    IC1CONCLR = _IC1CON_ICM_MASK;
    IC2CONCLR = _IC2CON_ICM_MASK;
    IC3CONCLR = _IC3CON_ICM_MASK;
    IC4CONCLR = _IC4CON_ICM_MASK;
    IC5CONCLR = _IC5CON_ICM_MASK;

    // disable output compare interrupts, clear flags and turn off module
    TurnOffAndClearInterrupt(INT_OC1);
    TurnOffAndClearInterrupt(INT_OC2);
    TurnOffAndClearInterrupt(INT_OC3);
    TurnOffAndClearInterrupt(INT_OC4);
    TurnOffAndClearInterrupt(INT_OC5);
    TurnOffAndClearInterrupt(INT_AD1);
    OC1CONCLR = _OC1CON_ON_MASK;
    OC2CONCLR = _OC2CON_ON_MASK;
    OC3CONCLR = _OC3CON_ON_MASK;
    OC4CONCLR = _OC4CON_ON_MASK;
    OC5CONCLR = _OC5CON_ON_MASK;

    // disable I2C interrupts, clear flags and turn off module
    TurnOffAndClearInterrupt(INT_I2C1);
    TurnOffAndClearInterrupt(INT_I2C1B);
    TurnOffAndClearInterrupt(INT_I2C1M);
    TurnOffAndClearInterrupt(INT_I2C1S);
    TurnOffAndClearInterrupt(INT_I2C2);
    TurnOffAndClearInterrupt(INT_I2C2B);
    TurnOffAndClearInterrupt(INT_I2C2M);
    TurnOffAndClearInterrupt(INT_I2C2S);
    I2C1CONCLR = _I2C1CON_ON_MASK;
    I2C2CONCLR = _I2C2CON_ON_MASK;

    //disable spi interrupts, clear flags and turn off module
    TurnOffAndClearInterrupt(INT_SPI1);
    TurnOffAndClearInterrupt(INT_SPI1E);
    TurnOffAndClearInterrupt(INT_SPI1RX);
    TurnOffAndClearInterrupt(INT_SPI1TX);
    TurnOffAndClearInterrupt(INT_SPI2);
    TurnOffAndClearInterrupt(INT_SPI2E);
    TurnOffAndClearInterrupt(INT_SPI2RX);
    TurnOffAndClearInterrupt(INT_SPI2TX);
    SPI1CONCLR = _SPI1CON_ON_MASK;
    SPI2CONCLR = _SPI2CON_ON_MASK;

    // disable external interrupts, clear flags and turn off module
    TurnOffAndClearInterrupt(INT_INT0);
    TurnOffAndClearInterrupt(INT_INT1);
    TurnOffAndClearInterrupt(INT_INT2);
    TurnOffAndClearInterrupt(INT_INT3);
    TurnOffAndClearInterrupt(INT_INT4);

    // set all ports to be digital inputs
    PORTSetPinsDigitalIn(IOPORT_B, 0xff);
    PORTSetPinsDigitalIn(IOPORT_C, 0xff);
    PORTSetPinsDigitalIn(IOPORT_D, 0xff);
    PORTSetPinsDigitalIn(IOPORT_E, 0xff);
    PORTSetPinsDigitalIn(IOPORT_F, 0xff);
    PORTSetPinsDigitalIn(IOPORT_G, 0xff);

    //Serial and A/D are left on for output and battery monitoring respectively
}

/**
 * Function: BOARD_GetPBClock(void)
 * @param None
 * @return
 */
unsigned int BOARD_GetPBClock()
{
    return PB_CLOCK;
}

/**
 * Function: BOARD_GetSysClock(void)
 * @param None
 * @return
 */
unsigned int BOARD_GetSysClock()
{
    return SYSTEM_CLOCK;
}

/*******************************************************************************
 * PRIVATE FUNCTIONS                                                           *
 ******************************************************************************/

/**
 * @Function SERIAL_Init(void)
 * @param none
 * @return none
 * @brief  Initializes the UART subsystem to 115200 and sets up the circular buffer
 * @author Max Dunne, 2011.11.10 */

void SERIAL_Init(void)
{

    UARTConfigure(UART1, 0x00);
    UARTSetDataRate(UART1, PB_CLOCK, 115200);
    UARTSetFifoMode(UART1, UART_INTERRUPT_ON_RX_NOT_EMPTY | UART_INTERRUPT_ON_RX_NOT_EMPTY);



    UARTEnable(UART1, UART_ENABLE_FLAGS(UART_PERIPHERAL | UART_TX | UART_RX));
    __XC_UART = 1;

}



/*******************************************************************************
 * OVERRIDE FUNCTIONS                                                          *
 ******************************************************************************/

/**
 * @Function read(int handle, void *buffer, unsigned int len)
 * @param handle
 * @param buffer
 * @param len
 * @return Returns the number of characters read into buffer
 * @brief Overrides the built-in function called for scanf() to ensure proper functionality
 */

#ifndef LAB10_READ_OVERWRITE
int read(int handle, void *buffer, unsigned int len)
{
    int i;
    if (handle == 0) {
        while (!U1STAbits.URXDA) {
            if (U1STAbits.OERR) {
                U1STAbits.OERR = 0;
            }
            continue;
        }
        i = 0;
        while (U1STAbits.URXDA) {
            char tmp = U1RXREG;
            if (tmp == '\r') {
                tmp = '\n';
            }
            *(char*) buffer++ = tmp;
            WriteUART1(tmp);
            i++;
        }
        return i;
    }
    return 0;
}
#endif