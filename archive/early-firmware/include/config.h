#ifndef CONFIG_H
#define CONFIG_H

/* This value must match the clock configured in the STC download tool. */
#define FOSC                 12000000UL
#define UART_BAUD            9600UL

/* Replace these placeholder digits with the real student number. */
#define STUDENT_ID_LENGTH    8U
#define STUDENT_ID_DIGITS    2U, 0U, 2U, 6U, 0U, 0U, 0U, 0U

#define CLOCK_START_HOUR     12U
#define CLOCK_START_MINUTE   0U
#define CLOCK_START_SECOND   0U

#define ALARM_START_HOUR     7U
#define ALARM_START_MINUTE   30U
#define ALARM_RING_MS        30000U

#endif
