/* Project entry point: initialize modules and run the cooperative scheduler. */

#include <STC15.H>
#include "app.h"
#include "board.h"
#include "keys.h"
#include "music.h"
#include "uart1.h"

void main(void)
{
    EA = 0;
    Board_Init();
    Music_Init();
    Keys_Init();
    Uart1_Init();
    EA = 1;

    App_Init();

    while (1)
    {
        App_Service();
        if (Board_Take10msTick())
            App_Tick10ms(Keys_Scan10ms());
    }
}
