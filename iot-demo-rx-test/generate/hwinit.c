/*
* Copyright (c) 2013(2025) Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
*/

#include "iodefine.h"
#ifdef __cplusplus
extern "C" {
#endif
extern void HardwareSetup(void);
#ifdef __cplusplus
}
#endif

void HardwareSetup(void)
{
    /*
     * HOCO 50MHz クロック設定
     *
     * 外部水晶 + PLL ではなく、内蔵 HOCO を使用。
     * 外部水晶の起動問題を回避しつつ、十分な速度を確保。
     *
     * 最終クロック:
     *   ICLK  = 50MHz (CPU)
     *   PCLKA = 50MHz (周辺A)  ※ PCLKx 上限 50MHz
     *   PCLKB = 25MHz (周辺B)
     *   BCLK  = 25MHz
     *   FCLK  = 25MHz
     */
    volatile unsigned long i;

    /* 1. レジスタ書き込み保護解除 */
    SYSTEM.PRCR.WORD = 0xA503;   /* PRC0=1(クロック), PRC1=1(MSTP) */

    /* 2. HOCO 周波数選択: 50MHz */
    SYSTEM.HOCOPCR.BYTE = 0x00;  /* 00: 50MHz (デフォルト) */

    /* 3. HOCO 起動 */
    SYSTEM.HOCOCR.BYTE = 0x00;   /* HCSTP=0: HOCO 動作 */

    /* HOCO 安定待ち */
    for (i = 0; i < 10000; i++) {
        __asm("nop");
    }

    /* 4. SCKCR: 分周比設定 */
    /*    ICK  = 0000 (/1 = 50MHz) */
    /*    FCK  = 0001 (/2 = 25MHz) */
    /*    PCKA = 0000 (/1 = 50MHz) */
    /*    PCKB = 0000 (/1 = 50MHz) */
    /*    BCK  = 0001 (/2 = 25MHz) */
    /*    PSTOP1 = 1, PSTOP0 = 1 */
    SYSTEM.SCKCR.LONG = 0x10C10000;

    /* 5. クロックソースを HOCO に切替え */
    /*    CKSEL = 001 (HOCO 選択) */
    SYSTEM.SCKCR3.WORD = 0x0100;

    /* 6. 保護復帰 */
    SYSTEM.PRCR.WORD = 0xA500;
}
