#include "board.h"

void board_io_init(void)
{
    /* Segment cathodes are active low, so 1 means all segments off. */
    P2 = 0xFF;

    /* PNP digit drivers, colon LEDs and buzzer are all active low. */
    DIGIT_1 = 1;
    DIGIT_2 = 1;
    DIGIT_3 = 1;
    DIGIT_4 = 1;
    COLON_LED = 1;
    BUZZER_PIN = 1;

    KEY_MODE_PIN = 1;
    KEY_PLUS_PIN = 1;
    KEY_FUNC_PIN = 1;

    /* P2.0-P2.7: push-pull segment outputs. */
    P2M1 = 0x00;
    P2M0 = 0xFF;

    /* P4.1, P4.2, P4.4: push-pull digit/colon outputs. */
    P4M1 &= (u8)~0x16;
    P4M0 |= 0x16;

    /* P3.5-P3.7: push-pull buzzer and digit outputs. */
    P3M1 &= (u8)~0xE0;
    P3M0 |= 0xE0;

    /* P1.3-P1.5: quasi-bidirectional inputs with internal pull-up. */
    P1M1 &= (u8)~0x38;
    P1M0 &= (u8)~0x38;

    /* P3.5 is used by the buzzer, never as the T0 clock output. */
    INT_CLKO &= (u8)~0x01;
}
