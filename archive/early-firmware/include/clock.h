#ifndef CLOCK_H
#define CLOCK_H

#include "types.h"

typedef struct
{
    u8 hour;
    u8 minute;
    u8 second;
} ClockTime;

void clock_init(void);
void clock_tick_1ms_isr(void);
void clock_get(ClockTime *value);
bit clock_set(u8 hour, u8 minute, u8 second);
bit clock_take_second_event(void);

#endif
