# oled_draw_utility

A utility (for PIC32 & UNO32 w/ chipKIT Basic I/O Shield) to draw to the oled display and retrieve the structured pixel coordinates for that drawing. Useful for quickly creating graphics by pixel. Short press buttons 4-1 to change direction NORTH-WEST. Long press button 4 to toggle current selected pixel. Hold buttons 3 & 2 to set or reset (respectively) a series of pixels using the POT. Long press button 1 to toggle the axis of this sliding set/reset. Set switch 4 to output coordinates through stdin.

Notes:
- Best compiled with Microchip's XC32
- Some support libraries written by faculty at UC Santa Cruz (credits in file headers)
