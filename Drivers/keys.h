#ifndef KEYS_H
#define KEYS_H

#define KEY_EVENT_NONE   0x00U
#define KEY_EVENT_MODE   0x01U
#define KEY_EVENT_MINUS  0x02U
#define KEY_EVENT_PLUS   0x04U
#define KEY_EVENT_PLUS_REPEAT 0x08U

void Keys_Init(void);
unsigned char Keys_Scan10ms(void);

#endif
