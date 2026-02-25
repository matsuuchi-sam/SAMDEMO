/**
 * @file main.c
 * @brief SAMDEMO GR-SAKURA (RX63N) メインファームウェア
 *
 * Phase 1: LED Lチカ + UART printf による動作確認スケルトン
 *
 * ハードウェア構成:
 *   - GR-SAKURA (RX63N @ 96MHz 想定)
 *   - LED0: PORT0 bit0 (緑 LED)
 *   - UART0: TXD=P20, RXD=P21, 115200bps 8N1
 *
 * 将来の拡張予定（コメントで場所を明示）:
 *   - [EXTEND: BME280] I2C 初期化 + センサ読み取り
 *   - [EXTEND: FREERTOS] FreeRTOS タスクへの分割
 *   - [EXTEND: JSON_TX] JSON フォーマット送信
 */

#include <stdint.h>

/* ===========================================================================
 * RX63N レジスタ定義（ベアメタル）
 * ===========================================================================
 * 注: 本来は Renesas 提供のヘッダ（iodefine.h）を使うが、
 *     ここでは依存関係ゼロのポータブル定義を使用する。
 *     iodefine.h が入手できた場合はそちらを include することを推奨。
 * =========================================================================*/

/* --- ポート I/O ---
 * PORT0 PDR (Port Direction Register): 0x0008C000
 * PORT0 PODR (Port Output Data Register): 0x0008C020
 */
#define PORT0_PDR   (*(volatile uint8_t *)0x0008C000)
#define PORT0_PODR  (*(volatile uint8_t *)0x0008C020)

/* LED0 は PORT0 の bit0 */
#define LED0_BIT    (1u << 0)

/* --- UART0 (SCI0) レジスタ ---
 * SCI0 ベースアドレス: 0x0008A000
 */
#define SCI0_BASE   0x0008A000u
#define SCI0_SMR    (*(volatile uint8_t *)(SCI0_BASE + 0x00))  /* シリアルモードレジスタ */
#define SCI0_BRR    (*(volatile uint8_t *)(SCI0_BASE + 0x01))  /* ビットレートレジスタ */
#define SCI0_SCR    (*(volatile uint8_t *)(SCI0_BASE + 0x02))  /* シリアルコントロールレジスタ */
#define SCI0_TDR    (*(volatile uint8_t *)(SCI0_BASE + 0x03))  /* 送信データレジスタ */
#define SCI0_SSR    (*(volatile uint8_t *)(SCI0_BASE + 0x04))  /* シリアルステータスレジスタ */

/* SSR ビット定義 */
#define SSR_TEND    (1u << 2)  /* 送信完了フラグ */
#define SSR_TDRE    (1u << 7)  /* 送信データレジスタ空フラグ */

/* SCR ビット定義 */
#define SCR_TE      (1u << 5)  /* 送信イネーブル */

/* ===========================================================================
 * プロトタイプ宣言
 * =========================================================================*/
static void clock_init(void);
static void led_init(void);
static void uart0_init(void);
static void uart0_putchar(char c);
static void uart0_puts(const char *str);
static void delay_ms(uint32_t ms);

/* ===========================================================================
 * main 関数
 * =========================================================================*/
int main(void)
{
    /* --- 初期化 --- */
    clock_init();
    led_init();
    uart0_init();

    uart0_puts("\r\n");
    uart0_puts("=== SAMDEMO RX63N Firmware ===\r\n");
    uart0_puts("Phase 1: LED Lchika + UART printf\r\n");
    uart0_puts("\r\n");

    /* --- メインループ --- */
    uint32_t count = 0;
    char buf[64];

    while (1)
    {
        /* LED0 トグル */
        PORT0_PODR ^= LED0_BIT;

        /* シリアル出力（簡易フォーマット）*/
        uart0_puts("[SAMDEMO] count=");
        /* 数値を文字列変換（printf 非使用の最小実装）*/
        uint32_t n = count;
        char num_buf[12];
        int idx = 0;
        if (n == 0) {
            num_buf[idx++] = '0';
        } else {
            while (n > 0) {
                num_buf[idx++] = '0' + (char)(n % 10);
                n /= 10;
            }
            /* 逆順にする */
            for (int i = 0, j = idx - 1; i < j; i++, j--) {
                char tmp = num_buf[i];
                num_buf[i] = num_buf[j];
                num_buf[j] = tmp;
            }
        }
        num_buf[idx] = '\0';
        uart0_puts(num_buf);
        uart0_puts(" LED=");
        uart0_puts((PORT0_PODR & LED0_BIT) ? "ON" : "OFF");
        uart0_puts("\r\n");

        /*
         * [EXTEND: BME280]
         * ここに BME280 読み取り処理を追加する:
         *   float temp, humidity, pressure;
         *   bme280_read(&temp, &humidity, &pressure);
         *   // JSON 送信へ...
         */

        /*
         * [EXTEND: JSON_TX]
         * ここに JSON 送信処理を追加する:
         *   uart0_puts("{\"temp\":xx.x,\"humidity\":xx.x,...}\r\n");
         */

        count++;
        delay_ms(500);  /* 500ms 待機（LED を 1Hz でトグル）*/
    }

    return 0;  /* 到達しない */
}

/* ===========================================================================
 * クロック初期化
 *
 * GR-SAKURA は 12MHz 外部クリスタルを搭載。
 * RX63N PLL を使って 96MHz で動作させる場合の設定。
 *
 * 注: この関数は簡略化されたスケルトンです。
 *     実際の設定は RX63N ハードウェアマニュアルの
 *     「クロック発生回路」章を参照してください。
 * =========================================================================*/
static void clock_init(void)
{
    /*
     * [SKELETON]
     * 実際の実装では以下の設定が必要:
     *   1. 外部クリスタル (EXTAL) を有効化
     *   2. PLL 逓倍率を設定（例: 12MHz × 8 = 96MHz）
     *   3. システムクロック分周比を設定
     *   4. クロック安定待機
     *
     * ここではデフォルトの内部クロックで動作させる（約 125kHz）。
     * 正確なタイミングが必要になった時点で実装を追加する。
     */
}

/* ===========================================================================
 * LED 初期化
 *
 * PORT0 bit0 を出力に設定し、初期状態を OFF（High）にする。
 * GR-SAKURA の LED は Active Low（Lレベルで点灯）。
 * =========================================================================*/
static void led_init(void)
{
    /* PORT0 bit0 を出力方向に設定 */
    PORT0_PDR |= LED0_BIT;

    /* 初期状態: LED OFF（High 出力）*/
    PORT0_PODR |= LED0_BIT;
}

/* ===========================================================================
 * UART0 初期化
 *
 * SCI0 を非同期モード 115200bps 8N1 で初期化する。
 *
 * BRR 計算式（SMR.CKS=00 の場合）:
 *   BRR = (PCLK / (64 × 2^(2×n-1) × baud)) - 1
 *   96MHz クロック、115200bps の場合: BRR ≈ 12
 * =========================================================================*/
static void uart0_init(void)
{
    /* SCI0 の送信を停止 */
    SCI0_SCR = 0x00;

    /* 非同期モード、8bit、1停止ビット、パリティなし */
    SCI0_SMR = 0x00;

    /* ビットレート設定（96MHz クロック想定、115200bps → BRR=12）*/
    /* クロック設定が未完了の場合は動作しない点に注意 */
    SCI0_BRR = 12;

    /* 1ビット時間待機（BRR 設定後に必要）*/
    for (volatile int i = 0; i < 1000; i++);

    /* 送信イネーブル */
    SCI0_SCR = SCR_TE;

    /*
     * [SKELETON]
     * 実際の実装では以下も必要:
     *   1. MPC (マルチ機能ピンコントローラ) で P20/P21 を TXD0/RXD0 に割り当て
     *   2. PORT2 PDR で P20 を出力に設定
     *   3. 受信が必要なら SCR_RE も設定
     */
}

/* ===========================================================================
 * UART0 1文字送信
 * =========================================================================*/
static void uart0_putchar(char c)
{
    /* 送信データレジスタ空き待ち */
    while (!(SCI0_SSR & SSR_TDRE));

    /* データ送信 */
    SCI0_TDR = (uint8_t)c;
}

/* ===========================================================================
 * UART0 文字列送信
 * =========================================================================*/
static void uart0_puts(const char *str)
{
    while (*str != '\0') {
        uart0_putchar(*str);
        str++;
    }
}

/* ===========================================================================
 * ソフトウェアディレイ（ms 単位）
 *
 * 注: クロック設定に依存する。clock_init() が完了するまで不正確。
 *     Phase 2 以降では SysTick または TMR を使った正確な実装に置き換える。
 *
 * [EXTEND: FREERTOS] FreeRTOS 導入後は vTaskDelay() に置き換える
 * =========================================================================*/
static void delay_ms(uint32_t ms)
{
    /* 96MHz 動作を仮定した概算ループカウント */
    /* 実際のクロック設定後に調整が必要 */
    volatile uint32_t count = ms * 10000u;
    while (count--);
}
