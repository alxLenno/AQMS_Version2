#ifndef DISPLAY_H
#define DISPLAY_H

#include <U8g2lib.h>
#include "sensors.h"
#include "config.h"

// External display object
extern U8G2_ST7920_128X64_F_SW_SPI u8g2;

// Display variables
extern int page;
extern unsigned long lastSwitchTime;
extern const unsigned long pageInterval;

// Function declarations
void updateDisplay();

#endif // DISPLAY_H
