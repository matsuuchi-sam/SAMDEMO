/*
 * sci0_uart.h - SCI0 UART ドライバ (GR-SAKURA RX63N)
 *
 * P20 = TXD0, P21 = RXD0
 * 115200bps, 8N1
 * PCLKB = 50MHz (HOCO)
 */

#ifndef SCI0_UART_H
#define SCI0_UART_H

void sci0_init(void);
void sci0_putc(char c);
void sci0_puts(const char *s);
int  sci0_getc(void);          /* ブロッキング受信 */
int  sci0_trygetc(void);       /* ノンブロッキング受信 (-1 = データなし) */

/* 簡易数値出力 */
void sci0_put_int(long val);
void sci0_put_hex(unsigned long val);

#endif /* SCI0_UART_H */
