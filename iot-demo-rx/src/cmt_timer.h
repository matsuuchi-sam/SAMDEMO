/*
 * cmt_timer.h - CMT0 タイマードライバ (GR-SAKURA RX63N)
 *
 * CMT0 で 1ms 周期割り込みを生成
 * PCLKB = 50MHz, CMCR.CKS = 00 (PCLK/8)
 * CMCOR = 50000000 / 8 / 1000 - 1 = 6249
 */

#ifndef CMT_TIMER_H
#define CMT_TIMER_H

void          cmt0_init(void);
unsigned long millis(void);
void          delay_ms(unsigned long ms);

/* 割り込みハンドラから呼ばれる (inthandler.c から使用) */
void          cmt0_isr(void);

#endif /* CMT_TIMER_H */
