/*
 * sci2_uart.h - SCI2 UART 割り込み駆動リングバッファ
 *
 * P50 = TXD2, P52 = RXD2 (115200bps 8N1)
 */

#ifndef SCI2_UART_H
#define SCI2_UART_H

void sci2_init(void);
void sci2_putc(char c);
void sci2_puts(const char *s);
int  sci2_getc_timeout(unsigned long timeout_ms);
int  sci2_readline(char *buf, int maxlen, unsigned long timeout_ms);

/* 割り込みハンドラ (inthandler.c から呼ばれる) */
void sci2_rxi_isr(void);

#endif /* SCI2_UART_H */
