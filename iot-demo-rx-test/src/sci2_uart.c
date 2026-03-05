/*
 * sci2_uart.c - SCI2 UART 割り込み駆動リングバッファ
 *
 * P50 = TXD2, P52 = RXD2
 * 115200bps 8N1, PCLKB = 50MHz
 * 受信: 割り込み + 256バイトリングバッファ
 * 送信: ポーリング
 */

#include "iodefine.h"
#include "sci2_uart.h"
#include "FreeRTOS.h"
#include "semphr.h"

#define RX_BUF_SIZE 256
#define RX_BUF_MASK (RX_BUF_SIZE - 1)

static volatile unsigned char rx_buf[RX_BUF_SIZE];
static volatile unsigned int  rx_head = 0;
static volatile unsigned int  rx_tail = 0;

static SemaphoreHandle_t rx_sem = NULL;

void sci2_rxi_isr(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    unsigned char data = SCI2.RDR;
    unsigned int next = (rx_head + 1) & RX_BUF_MASK;

    if (next != rx_tail) {
        rx_buf[rx_head] = data;
        rx_head = next;
    }

    if (rx_sem != NULL) {
        xSemaphoreGiveFromISR(rx_sem, &xHigherPriorityTaskWoken);
    }
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void sci2_init(void)
{
    rx_sem = xSemaphoreCreateBinary();

    /* モジュールストップ解除 */
    SYSTEM.PRCR.WORD = 0xA502;
    SYSTEM.MSTPCRB.BIT.MSTPB29 = 0;
    SYSTEM.PRCR.WORD = 0xA500;

    SCI2.SCR.BYTE = 0x00;
    SCI2.SMR.BYTE = 0x00;
    SCI2.SEMR.BIT.ABCS = 1;
    SCI2.BRR = 26;

    {
        volatile int i;
        for (i = 0; i < 1000; i++) {
            __asm("nop");
        }
    }

    /* MPC */
    MPC.PWPR.BIT.B0WI = 0;
    MPC.PWPR.BIT.PFSWE = 1;
    MPC.P50PFS.BIT.PSEL = 0x0A;
    MPC.P52PFS.BIT.PSEL = 0x0A;
    MPC.PWPR.BIT.PFSWE = 0;
    MPC.PWPR.BIT.B0WI = 1;

    PORT5.PMR.BIT.B0 = 1;
    PORT5.PMR.BIT.B2 = 1;

    /* RXI2 割り込み設定 (ベクタ220 = 0x370) */
    ICU.IPR[220].BIT.IPR = 3;   /* 優先度3 (configMAX_SYSCALL_INTERRUPT_PRIORITY以下) */
    ICU.IR[220].BIT.IR = 0;
    ICU.IER[0x1B].BIT.IEN4 = 1; /* IER[220/8].IEN[220%8] = IER[27].IEN4 */

    /* 送受信有効化 + RXI割り込み有効 */
    SCI2.SCR.BIT.RIE = 1;
    SCI2.SCR.BIT.TE = 1;
    SCI2.SCR.BIT.RE = 1;
}

void sci2_putc(char c)
{
    while (SCI2.SSR.BIT.TDRE == 0)
        ;
    SCI2.TDR = (unsigned char)c;
}

void sci2_puts(const char *s)
{
    while (*s)
        sci2_putc(*s++);
}

int sci2_getc_timeout(unsigned long timeout_ms)
{
    TickType_t start = xTaskGetTickCount();
    TickType_t ticks = pdMS_TO_TICKS(timeout_ms);

    while (rx_head == rx_tail) {
        TickType_t elapsed = xTaskGetTickCount() - start;
        if (elapsed >= ticks) return -1;
        xSemaphoreTake(rx_sem, ticks - elapsed);
    }

    unsigned char c = rx_buf[rx_tail];
    rx_tail = (rx_tail + 1) & RX_BUF_MASK;
    return (int)c;
}

int sci2_readline(char *buf, int maxlen, unsigned long timeout_ms)
{
    int pos = 0;
    while (pos < maxlen - 1) {
        int c = sci2_getc_timeout(timeout_ms);
        if (c < 0) return -1;
        if (c == '\n' || c == '\r') {
            if (pos > 0) {
                buf[pos] = '\0';
                return pos;
            }
            continue;
        }
        buf[pos++] = (char)c;
    }
    buf[pos] = '\0';
    return pos;
}
