#ifndef UART1_H
#define UART1_H

void Uart1_Init(void);
bit Uart1_TakeTime(unsigned char data *hour,
                   unsigned char data *minute,
                   unsigned char data *second);
bit Uart1_TakeAlarm(unsigned char data *index,
                    unsigned char data *hour,
                    unsigned char data *minute);

#endif
