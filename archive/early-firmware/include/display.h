#ifndef DISPLAY_H
#define DISPLAY_H

#include "types.h"

#define DISP_0       0U
#define DISP_1       1U
#define DISP_2       2U
#define DISP_3       3U
#define DISP_4       4U
#define DISP_5       5U
#define DISP_6       6U
#define DISP_7       7U
#define DISP_8       8U
#define DISP_9       9U
#define DISP_BLANK   10U

#define COLON_OFF    0U
#define COLON_ON     1U
#define COLON_BLINK  2U

void display_init(void);
void display_tick_1ms_isr(void);
void display_set_codes(u8 d0, u8 d1, u8 d2, u8 d3,
                       u8 decimal_mask, u8 blink_mask);
void display_set_colon(u8 mode);

#endif
