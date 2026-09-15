#ifndef KEYS_H
#define KEYS_H

#include "types.h"

#define KEY_EVT_MODE_SHORT   0x01U
#define KEY_EVT_MODE_LONG    0x02U
#define KEY_EVT_PLUS_SHORT   0x04U
#define KEY_EVT_PLUS_LONG    0x08U
#define KEY_EVT_PLUS_REPEAT  0x10U
#define KEY_EVT_FUNC_SHORT   0x20U

void keys_init(void);
void keys_task_10ms(void);
u8 keys_take_events(void);

#endif
