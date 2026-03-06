/*
 * sci0_uart.c - SCI2 UART ドライバ (GR-SAKURA RX63N)
 *
 * P50 = TXD2, P52 = RXD2  (Arduino ピン24=TX, ピン26=RX)
 * 115200bps, 8N1
 * PCLKB = 50MHz (HOCO 50MHz / 1)
 *
 * BRR 計算 (SEMR.ABCS=1, CKS=0):
 *   BRR = PCLKB / (16 * baud) - 1
 *       = 50000000 / (16 * 115200) - 1 = 26.1 → BRR = 26
 *   実ボーレート = 50000000 / (16 * 27) = 115741 (誤差 +0.47%)
 */

#include "iodefine.h"
#include "sci0_uart.h"

void sci0_init(void)
{
    /* ---- モジュールストップ解除 ---- */
    SYSTEM.PRCR.WORD = 0xA502;
    SYSTEM.MSTPCRB.BIT.MSTPB29 = 0;  /* SCI2 モジュール起動 */
    SYSTEM.PRCR.WORD = 0xA500;

    /* ---- SCI2 初期化 ---- */
    SCI2.SCR.BYTE = 0x00;

    /* SMR: 非同期モード, 8bit, パリティなし, 1 stop, PCLK/1 */
    SCI2.SMR.BYTE = 0x00;

    /* SEMR: ABCS=1 (倍速モード) */
    SCI2.SEMR.BIT.ABCS = 1;

    /* BRR: 115200bps @ PCLKB=50MHz, ABCS=1 */
    SCI2.BRR = 26;

    /* ボーレート安定待ち (1ビット期間以上) */
    {
        volatile int i;
        for (i = 0; i < 1000; i++) {
            __asm("nop");
        }
    }

    /* ---- ピン機能設定 (MPC) ---- */
    MPC.PWPR.BIT.B0WI = 0;
    MPC.PWPR.BIT.PFSWE = 1;

    /* P50 → TXD2 (PSEL = 0x0A) */
    MPC.P50PFS.BIT.PSEL = 0x0A;
    /* P52 → RXD2 (PSEL = 0x0A) */
    MPC.P52PFS.BIT.PSEL = 0x0A;

    MPC.PWPR.BIT.PFSWE = 0;
    MPC.PWPR.BIT.B0WI = 1;

    /* ポートモードレジスタ: P50, P52 を周辺機能に切替 */
    PORT5.PMR.BIT.B0 = 1;      /* P50 = TXD2 (周辺機能) */
    PORT5.PMR.BIT.B2 = 1;      /* P52 = RXD2 (周辺機能) */

    /* ---- 送受信有効化 ---- */
    SCI2.SCR.BIT.TE = 1;
    SCI2.SCR.BIT.RE = 1;
}

void sci0_putc(char c)
{
    while (SCI2.SSR.BIT.TDRE == 0)
        ;
    SCI2.TDR = (unsigned char)c;
}

void sci0_puts(const char *s)
{
    while (*s) {
        sci0_putc(*s++);
    }
}

int sci0_getc(void)
{
    unsigned char data;

    if (SCI2.SSR.BIT.ORER || SCI2.SSR.BIT.FER || SCI2.SSR.BIT.PER) {
        SCI2.SSR.BIT.ORER = 0;
        SCI2.SSR.BIT.FER = 0;
        SCI2.SSR.BIT.PER = 0;
    }

    while (SCI2.SSR.BIT.RDRF == 0)
        ;
    data = SCI2.RDR;
    return (int)data;
}

int sci0_trygetc(void)
{
    if (SCI2.SSR.BIT.ORER || SCI2.SSR.BIT.FER || SCI2.SSR.BIT.PER) {
        SCI2.SSR.BIT.ORER = 0;
        SCI2.SSR.BIT.FER = 0;
        SCI2.SSR.BIT.PER = 0;
    }

    if (SCI2.SSR.BIT.RDRF == 0)
        return -1;
    return (int)SCI2.RDR;
}

void sci0_put_int(long val)
{
    char buf[12];
    int i = 0;
    unsigned long uval;

    if (val < 0) {
        sci0_putc('-');
        uval = (unsigned long)(-(val + 1)) + 1;
    } else {
        uval = (unsigned long)val;
    }

    if (uval == 0) {
        sci0_putc('0');
        return;
    }

    while (uval > 0) {
        buf[i++] = '0' + (char)(uval % 10);
        uval /= 10;
    }

    while (i > 0) {
        sci0_putc(buf[--i]);
    }
}

void sci0_put_hex(unsigned long val)
{
    static const char hex[] = "0123456789ABCDEF";
    int i;

    sci0_puts("0x");
    for (i = 28; i >= 0; i -= 4) {
        sci0_putc(hex[(val >> i) & 0x0F]);
    }
}
