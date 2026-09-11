#ifndef BOARD_H
#define BOARD_H

#define COLON_OFF       0U
#define COLON_ON        1U
#define COLON_BLINK     2U

#define FIELD_NONE      0U
#define FIELD_HOUR      1U
#define FIELD_MINUTE    2U
#define FIELD_SECOND    3U

extern volatile unsigned char data ClockHour;
extern volatile unsigned char data ClockMinute;
extern volatile unsigned char data ClockSecond;
extern unsigned char data AlarmHour;
extern unsigned char data AlarmMinute;
extern volatile bit AlarmRinging;

void Board_Init(void);
bit Board_Take10msTick(void);
bit Board_TakeSecondEvent(void);
void Board_SetDisplay(unsigned char d0, unsigned char d1,
                      unsigned char d2, unsigned char d3,
                      unsigned char blink_mask, unsigned char colon_mode);
void Board_SetTime(unsigned char hour, unsigned char minute,
                   unsigned char second);
void Board_AdjustClock(unsigned char field, unsigned char increase);
void Board_AdjustAlarm(unsigned char field, unsigned char increase);
void Board_StartClockTick(void);
void Board_StartAlarm(void);
void Board_StopAlarm(void);

#endif
