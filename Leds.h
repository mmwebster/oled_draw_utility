/*
 * File:   Leds.h
 * Author: milowebster
 * Collaborators: Roger Berman
 *
 * Created on May 11, 2015, 11:56 AM
 */

#ifndef LEDS_H
#define	LEDS_H

// User libraries
#include "BOARD.h"

// System libraries
#include <xc.h>

// Macros
#define LEDS_INIT() do { \
    TRISE = 0x00; \
    LATE = 0x00; \
} while (0)

#define LEDS_GET() LATE

#define LEDS_SET(x) do { \
    LATE = (x); \
} while (0)

// Data types


#endif	/* LEDS_H */


