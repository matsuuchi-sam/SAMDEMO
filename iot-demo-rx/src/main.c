/*
 * main.c - GR-SAKURA (RX63N) LED Lチカ テスト
 *
 * 確認済み: LED は Active High（PODR=1 で点灯）
 * クロック: LOCO 125kHz（HardwareSetup なし）
 *   delay(10000) ≈ 1秒 @ 125kHz
 */

#include "iodefine.h"

static void delay(volatile unsigned long n)
{
    while (n--) {
        /* NOP */
    }
}

int main(void)
{
    /* 全ポートを出力に設定 */
    PORT0.PDR.BYTE = 0xFF;
    PORT1.PDR.BYTE = 0xFF;
    PORT2.PDR.BYTE = 0xFF;
    PORT3.PDR.BYTE = 0xFF;
    PORT4.PDR.BYTE = 0xFF;
    PORT5.PDR.BYTE = 0xFF;
    PORT6.PDR.BYTE = 0xFF;
    PORT7.PDR.BYTE = 0xFF;
    PORT8.PDR.BYTE = 0xFF;
    PORT9.PDR.BYTE = 0xFF;
    PORTA.PDR.BYTE = 0xFF;
    PORTB.PDR.BYTE = 0xFF;
    PORTC.PDR.BYTE = 0xFF;
    PORTD.PDR.BYTE = 0xFF;
    PORTE.PDR.BYTE = 0xFF;

    while (1) {
        /* 全ポート HIGH → LED ON */
        PORT0.PODR.BYTE = 0xFF;
        PORT1.PODR.BYTE = 0xFF;
        PORT2.PODR.BYTE = 0xFF;
        PORT3.PODR.BYTE = 0xFF;
        PORT4.PODR.BYTE = 0xFF;
        PORT5.PODR.BYTE = 0xFF;
        PORT6.PODR.BYTE = 0xFF;
        PORT7.PODR.BYTE = 0xFF;
        PORT8.PODR.BYTE = 0xFF;
        PORT9.PODR.BYTE = 0xFF;
        PORTA.PODR.BYTE = 0xFF;
        PORTB.PODR.BYTE = 0xFF;
        PORTC.PODR.BYTE = 0xFF;
        PORTD.PODR.BYTE = 0xFF;
        PORTE.PODR.BYTE = 0xFF;

        delay(10000);

        /* 全ポート LOW → LED OFF */
        PORT0.PODR.BYTE = 0x00;
        PORT1.PODR.BYTE = 0x00;
        PORT2.PODR.BYTE = 0x00;
        PORT3.PODR.BYTE = 0x00;
        PORT4.PODR.BYTE = 0x00;
        PORT5.PODR.BYTE = 0x00;
        PORT6.PODR.BYTE = 0x00;
        PORT7.PODR.BYTE = 0x00;
        PORT8.PODR.BYTE = 0x00;
        PORT9.PODR.BYTE = 0x00;
        PORTA.PODR.BYTE = 0x00;
        PORTB.PODR.BYTE = 0x00;
        PORTC.PODR.BYTE = 0x00;
        PORTD.PODR.BYTE = 0x00;
        PORTE.PODR.BYTE = 0x00;

        delay(10000);
    }

    return 0;
}
