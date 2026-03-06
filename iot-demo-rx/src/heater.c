/*
 * heater.c - ヒーター GPIO 制御
 *
 * ヒーターポート: PA0
 * Active High: PA0=1 でヒーターON
 */

#include "iodefine.h"
#include "heater.h"

static int g_state = 0;

void heater_init(void)
{
    PORTA.PDR.BIT.B0 = 1;   /* PA0 出力 */
    PORTA.PODR.BIT.B0 = 0;  /* OFF */
    g_state = 0;
}

void heater_on(void)
{
    PORTA.PODR.BIT.B0 = 1;
    g_state = 1;
}

void heater_off(void)
{
    PORTA.PODR.BIT.B0 = 0;
    g_state = 0;
}

int heater_state(void)
{
    return g_state;
}

void heater_set_from_pid(long pid_output)
{
    /* シンプルなオンオフ制御 (50% 閾値) */
    if (pid_output >= 5000)
        heater_on();
    else
        heater_off();
}
