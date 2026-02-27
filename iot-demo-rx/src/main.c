/**
 * @file main.c
 * @brief GR-SAKURA (RX63N) LED 点灯確認用 最小構成ファームウェア
 *
 * 目的: CPUが起動しているか確認するために、
 *       PORT0〜PORT4 の複数ポートを全て出力LOWにしてLEDを探す。
 *       clock_init/uart_init は一切なし。LOCOクロックのまま動作。
 */

#include <stdint.h>

/* --- ポート I/O レジスタ定義 ---
 * PDR  (Port Direction Register) : 出力=1, 入力=0
 * PODR (Port Output Data Register): High=1, Low=0
 * GR-SAKURA の LED は Active Low (Low で点灯)
 */
#define PORT0_PDR   (*(volatile uint8_t *)0x0008C000)
#define PORT1_PDR   (*(volatile uint8_t *)0x0008C001)
#define PORT2_PDR   (*(volatile uint8_t *)0x0008C002)
#define PORT3_PDR   (*(volatile uint8_t *)0x0008C003)
#define PORT4_PDR   (*(volatile uint8_t *)0x0008C004)

#define PORT0_PODR  (*(volatile uint8_t *)0x0008C020)
#define PORT1_PODR  (*(volatile uint8_t *)0x0008C021)
#define PORT2_PODR  (*(volatile uint8_t *)0x0008C022)
#define PORT3_PODR  (*(volatile uint8_t *)0x0008C023)
#define PORT4_PODR  (*(volatile uint8_t *)0x0008C024)

/* --- ソフトウェアディレイ (LOCO 125kHz 動作) ---
 * 1ループ ≈ 数サイクル @ 125kHz ≈ 数十μs
 * count=50000 で約 2〜3秒 (目視確認できる速度)
 */
static void delay_loop(volatile uint32_t count)
{
    while (count--);
}

int main(void)
{
    /* 全ポートを出力方向に設定 */
    PORT0_PDR = 0xFF;
    PORT1_PDR = 0xFF;
    PORT2_PDR = 0xFF;
    PORT3_PDR = 0xFF;
    PORT4_PDR = 0xFF;

    /* 全ポートを最初に High (LED OFF) にする */
    PORT0_PODR = 0xFF;
    PORT1_PODR = 0xFF;
    PORT2_PODR = 0xFF;
    PORT3_PODR = 0xFF;
    PORT4_PODR = 0xFF;

    while (1)
    {
        /* 全ポートを Low → LED が点灯するはず (Active Low) */
        PORT0_PODR = 0x00;
        PORT1_PODR = 0x00;
        PORT2_PODR = 0x00;
        PORT3_PODR = 0x00;
        PORT4_PODR = 0x00;

        delay_loop(50000);  /* 点灯 */

        /* 全ポートを High → LED 消灯 */
        PORT0_PODR = 0xFF;
        PORT1_PODR = 0xFF;
        PORT2_PODR = 0xFF;
        PORT3_PODR = 0xFF;
        PORT4_PODR = 0xFF;

        delay_loop(50000);  /* 消灯 */
    }

    return 0;
}
