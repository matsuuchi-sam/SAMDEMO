/*
 * cmt_timer.c - CMT0 タイマードライバ (GR-SAKURA RX63N)
 *
 * CMT0 で 1ms 周期割り込み
 * PCLKB = 50MHz (HOCO 50MHz / 1)
 * CKS = 00 (PCLK/8 = 6.25MHz)
 * CMCOR = 6250 - 1 = 6249
 * 割り込み周期 = 6250 / 6250000 = 1ms
 */

#include "iodefine.h"
#include "cmt_timer.h"

static volatile unsigned long g_millis = 0;

void cmt0_isr(void)
{
    g_millis++;
}

void cmt0_init(void)
{
    /* モジュールストップ解除 */
    SYSTEM.PRCR.WORD = 0xA502;       /* PRC1=1: MSTP 書き込み許可 */
    SYSTEM.MSTPCRA.BIT.MSTPA15 = 0;  /* CMT0/CMT1 起動 */
    SYSTEM.PRCR.WORD = 0xA500;

    /* CMT0 停止 */
    CMT.CMSTR0.BIT.STR0 = 0;

    /* CMCR: PCLK/8, コンペアマッチ割り込み有効 */
    CMT0.CMCR.BIT.CKS = 0;   /* 00: PCLK/8 */
    CMT0.CMCR.BIT.CMIE = 1;  /* コンペアマッチ割り込み有効 */

    /* CMCOR: 1ms 周期 */
    CMT0.CMCOR = 6249;       /* 50MHz/8/1000 - 1 */

    /* カウンタ初期化 */
    CMT0.CMCNT = 0;

    /* 割り込み優先度設定 */
    /* CMT0_CMI0: ベクタ 28, IER=0x03, IEN4 */
    ICU.IPR[4].BIT.IPR = 5;          /* 優先度 5 */
    ICU.IR[28].BIT.IR = 0;           /* 割り込みフラグクリア */
    ICU.IER[0x03].BIT.IEN4 = 1;      /* CMT0_CMI0 割り込み許可 */

    /* CMT0 カウント開始 */
    CMT.CMSTR0.BIT.STR0 = 1;
}

unsigned long millis(void)
{
    return g_millis;
}

void delay_ms(unsigned long ms)
{
    unsigned long start = g_millis;
    while ((g_millis - start) < ms)
        ;
}
