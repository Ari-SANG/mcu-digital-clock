#ifndef BOARD_H
#define BOARD_H

#include "types.h"
#include "stc15w4k32s4.h"

/* V1.3 board pin mapping from the supplied schematic. */
sbit DIGIT_1 = P4^2;
sbit DIGIT_2 = P4^1;
sbit DIGIT_3 = P3^7;
sbit DIGIT_4 = P3^6;
sbit COLON_LED = P4^4;

sbit KEY_MODE_PIN = P1^3;
sbit KEY_PLUS_PIN = P1^4;
sbit KEY_FUNC_PIN = P1^5;

sbit BUZZER_PIN = P3^5;

void board_io_init(void);

#endif
