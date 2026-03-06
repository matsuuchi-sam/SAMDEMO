/*
 * main.c - LED点滅テスト (最小)
 */

#include "iodefine.h"

static void delay(volatile unsigned long count)
{
    while (count--) __asm("nop");
}

int main(void)
{
    PORTE.PDR.BIT.B0 = 1;

    for (;;) {
        PORTE.PODR.BIT.B0 = 1;
        delay(2500000);
        PORTE.PODR.BIT.B0 = 0;
        delay(2500000);
    }

    return 0;
}
