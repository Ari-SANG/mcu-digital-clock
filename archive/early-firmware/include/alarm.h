#ifndef ALARM_H
#define ALARM_H

#include "types.h"

typedef struct
{
    u8 hour;
    u8 minute;
    u8 enabled;
} AlarmConfig;

void alarm_init(void);
void alarm_tick_1ms_isr(void);
void alarm_task_second(void);
void alarm_get(AlarmConfig *value);
bit alarm_set(u8 hour, u8 minute, bit enabled);
bit alarm_is_ringing(void);
void alarm_stop(void);

#endif
