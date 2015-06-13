/* 
 * @file:   main.c
 * @author: Milo Webster
 * @description: A utility (for PIC32 & UNO32 w/ std. I/O shield) to draw to the oled display and
 * retrieve the structured pixel coordinates for that drawing. Useful for quickly creating
 * graphics. Short press buttons 4-1 to change direction NORTH-WEST. Long press button 4 to toggle
 * current selected pixel. Hold buttons 3 & 2 to set or reset (respectively) a series of pixels
 * using the POT. Long press button 1 to toggle the axis of this setting/resetting. Turn switch 4
 * to on to output coordinates through stdin.
 *
 * Created on June 12, 2015, 1:59 PM
 */
// Standard libraries
#include <stdio.h>
#include <stdlib.h>

// 3rd party support libraries
#include "BOARD.h"

// Microchip libraries
#include <xc.h>
#include <plib.h>

// Project libraries
#include "Leds.h"
#include "Buttons.h"
#include "Adc.h"
#include "Oled.h"

// data types and #defines
#define ADC_CHANGED_BUFFER 3
#define LONG_PRESS 2
#define P_OBJ_WIDTH 20
#define P_OBJ_HEIGHT 20

typedef enum {
    NORTH,
    EAST,
    SOUTH,
    WEST
} Direction;

// Mode for setting current pixel
typedef enum {
    P_SET,
    P_RESET,
    P_TOGGLE
} Mode;

enum Axis {
    X,
    Y
};

enum Adcs {
    PREVIOUS,
    NEW
};

// Module level and global variables
ButtonEventFlags buttonEvent = BUTTON_EVENT_NONE;
uint8_t oled[OLED_DRIVER_PIXEL_COLUMNS][OLED_DRIVER_PIXEL_ROWS] = {}; // x, y
uint8_t cursor[2] = {}; // x, y
uint8_t buttonPressTimer = 0;
uint8_t slideAxis = X;
uint8_t buttonDown[5] = {}; // NOT base zero index. Is base 1 for readability.
uint16_t adc[2] = {};

// Function prototypes
static void MoveCursor(Direction dir);
static void SetCurrentPixel(Mode mode);
static uint8_t AdcDidChange(uint16_t *newAdcReturn);

int main() {

    // Peripheral and general system initializations
    BOARD_Init();
    LEDS_INIT();
    ButtonsInit();
    OledInit();
    AdcInit();

    // Configure Timer 2 using PBCLK as input. We configure it using a 1:16 prescalar, so each timer
    // tick is actually at F_PB / 16 Hz, so setting PR2 to F_PB / 16 / 100 yields a .01s timer.
    OpenTimer2(T2_ON | T2_SOURCE_INT | T2_PS_1_16, BOARD_GetPBClock() / 16 / 100);

    // Set up the timer interrupt with a medium priority of 4.
    INTClearFlag(INT_T2);
    INTSetVectorPriority(INT_TIMER_2_VECTOR, INT_PRIORITY_LEVEL_4);
    INTSetVectorSubPriority(INT_TIMER_2_VECTOR, INT_SUB_PRIORITY_LEVEL_0);
    INTEnable(INT_T2, INT_ENABLED);

    // Configure Timer 3 using PBCLK as input. We configure it using a 1:256 prescalar, so each
    // timer tick is actually at F_PB / 256 Hz, so setting PR3 to F_PB / 256 / 5 yields a .2s
    // timer.
    OpenTimer3(T3_ON | T3_SOURCE_INT | T3_PS_1_256, BOARD_GetPBClock() / 256 / 5);

    // Set up the timer interrupt with a medium priority of 4.
    INTClearFlag(INT_T3);
    INTSetVectorPriority(INT_TIMER_3_VECTOR, INT_PRIORITY_LEVEL_4);
    INTSetVectorSubPriority(INT_TIMER_3_VECTOR, INT_SUB_PRIORITY_LEVEL_0);
    INTEnable(INT_T3, INT_ENABLED);

    while(1) {
        // To slide set or unset pixels with the POT
        // Enter if change in ADC from POT AND BTN3||4 are down
        if (AdcDidChange(&adc[NEW]) && (buttonDown[3] || buttonDown[2])) {
            // Toggle pixel
            if (buttonDown[3]) {
                SetCurrentPixel(P_SET);
            }
            else if (buttonDown[2]) {
                SetCurrentPixel(P_RESET);
            }
            // move cursor
            if (adc[NEW] - adc[PREVIOUS] > 0) {
                // Change is positive, move positively along current axis
                if (slideAxis == X) {
                    MoveCursor(EAST);
                } else {
                    // axis is Y
                    MoveCursor(SOUTH);
                }
            } else {
                // Change is negative, move negatively along current axis
                if (slideAxis == X) {
                    MoveCursor(WEST);
                } else {
                    // axis is Y
                    MoveCursor(NORTH);
                }
            }
            // Setup for next cycle
            adc[PREVIOUS] = adc[NEW];
        }
        // Enter if button event hasn't been handled AND event is not BUTTON_EVENT_NONE
        if (buttonEvent) {
            switch (buttonEvent) {
                case BUTTON_EVENT_4DOWN:
                    // reset timer
                    buttonPressTimer = 0;
                    break;
                case BUTTON_EVENT_4UP:
                    if (buttonPressTimer < LONG_PRESS) {
                        // short press, move
                        MoveCursor(NORTH);
                    } else {
                        // long press, toggle pixel
                        SetCurrentPixel(P_TOGGLE);
                    }
                    break;
                case BUTTON_EVENT_3DOWN:
                    // set btn down
                    buttonPressTimer = 0;
                    buttonDown[3] = TRUE;
                    break;
                case BUTTON_EVENT_3UP:
                    // move cursor east
                    if (buttonPressTimer < LONG_PRESS) MoveCursor(EAST);
                    // reset btn down
                    buttonDown[3] = FALSE;
                    break;
                case BUTTON_EVENT_2DOWN:
                    // set btn down
                    buttonPressTimer = 0;
                    buttonDown[2] = TRUE;
                    break;
                case BUTTON_EVENT_2UP:
                    // move cursor south
                    if (buttonPressTimer < LONG_PRESS) MoveCursor(SOUTH);
                    // reset btn down
                    buttonDown[2] = FALSE;
                    break;
                case BUTTON_EVENT_1DOWN:
                    buttonPressTimer = 0;
                    break;
                case BUTTON_EVENT_1UP:
                    // move cursor west
                    if (buttonPressTimer < LONG_PRESS) {
                        // short press, move
                        MoveCursor(WEST);
                    } else {
                        // long press, toggle slideAxis
                        slideAxis = (slideAxis == X) ? Y : X;
                    }
                    break;
            }
            buttonEvent = BUTTON_EVENT_NONE;
        }
    }
    
    // prevent PIC reset
    while(1);
}

/**
 * Timer2 interrupt. Checks for button events.
 */
void __ISR(_TIMER_2_VECTOR, IPL4AUTO) TimerInterrupt100Hz(void)
{
    // Clear the interrupt flag.
    IFS0CLR = 1 << 8;

    // check for button events
    buttonEvent = ButtonsCheckEvents();
}

void __ISR(_TIMER_3_VECTOR, IPL4AUTO) TimerInterrupt5Hz(void) {
    // Clear the interrupt flag.
    IFS0CLR = 1 << 12;

    // Distinguish LONG_PRESS button event
    buttonPressTimer += 1;
}

/**
 * Moves cursor in desired direction if possible, else silently fails
 * @param dir
 */
static void MoveCursor(Direction dir)
{
    switch (dir) {
        case NORTH:
            if (cursor[Y] > 0) cursor[Y]--;
            break;
        case EAST:
            if (cursor[X] < (OLED_DRIVER_PIXEL_COLUMNS-1)) cursor[X]++;
            break;
        case SOUTH:
            if (cursor[Y] < (OLED_DRIVER_PIXEL_ROWS-1)) cursor[Y]++;
            break;
        case WEST:
            if (cursor[X] > 0) cursor[X]--;
            break;
        default:
            FATAL_ERROR();
    }
}

/**
 * Toggles the pixel for the current cursor position and updates the oled
 */
static void SetCurrentPixel(Mode mode)
{
    static OledColor color;
    if (mode == P_SET) {
        // turn on
        color = OLED_COLOR_WHITE;
        oled[cursor[X]][cursor[Y]] = TRUE;
    } else if (mode == P_RESET) {
        // turn off
        color = OLED_COLOR_BLACK;
        oled[cursor[X]][cursor[Y]] = TRUE;
    } else {
        // toggle on/off
        if (!oled[cursor[X]][cursor[Y]]) {
            // pixel off, turn on
            color = OLED_COLOR_WHITE;
            oled[cursor[X]][cursor[Y]] = TRUE;
        } else {
            // pixel on, turn off
            color = OLED_COLOR_BLACK;
            oled[cursor[X]][cursor[Y]] = FALSE;
        }
    }
    OledSetPixel(cursor[X], cursor[Y], color);
    OledUpdate();
    if (SW4) {
        int i, j;
        printf("\n\n\"{ ");
        for (i = 0; i < P_OBJ_WIDTH; i++) {
            printf("{ ");
            for (j = 0; j < P_OBJ_HEIGHT; j++) {
                if (oled[i][j]) printf("1");
                else printf("0");
                if (j != P_OBJ_HEIGHT-1) printf(", ");
            }
            printf("}");
            if (i != P_OBJ_WIDTH-1) printf(", ");
        }
        printf(" }\"");
    };
}

/**
 * Creates a buffer to take into account hardware limitations in sensitivity
 * @param newAdc reference value to store new adc if has changed
 * @return TRUE or FALSE
 */
static uint8_t AdcDidChange(uint16_t *newAdcReturn)
{
    static uint16_t previousAdc = 0;
    if (AdcChanged()) {
        uint16_t newAdc = AdcRead();
        // if change abs. value has exceeded buffer region
        if ((previousAdc - newAdc) > ADC_CHANGED_BUFFER || (previousAdc - newAdc) < -ADC_CHANGED_BUFFER) {
            *newAdcReturn = previousAdc = newAdc;
            return TRUE;
        }
    }
    return FALSE;
}