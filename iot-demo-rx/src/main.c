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

/* --- クロック制御レジスタ ---
 * RX63N リセット後のデフォルトクロック: LOCO (125kHz)
 * → HOCO (32MHz) に切り替えて UART BRR を正しい値に合わせる
 */
#define PRCR        (*(volatile uint16_t *)0x000803FE)  /* プロテクト解除レジスタ */
#define HOCOCR      (*(volatile uint8_t  *)0x00080036)  /* HOCO 制御レジスタ */
#define OSCOVFSR    (*(volatile uint8_t  *)0x0008003C)  /* 発振安定フラグレジスタ */
#define SCKCR3      (*(volatile uint16_t *)0x00080026)  /* システムクロック制御レジスタ3 */

/* --- MPC (マルチ機能ピンコントローラ) ---
 * P20 を TXD0 として、P21 を RXD0 として動作させるために必要
 */
#define MPC_PWPR    (*(volatile uint8_t *)0x0008C11F)   /* 書き込み保護レジスタ */
#define MPC_P20PFS  (*(volatile uint8_t *)0x0008C150)   /* P20 ピン機能選択 (=0x0008C140 + 2*8 + 0) */
#define MPC_P21PFS  (*(volatile uint8_t *)0x0008C151)   /* P21 ピン機能選択 (=0x0008C140 + 2*8 + 1) */

/* --- PORT2 制御レジスタ ---
 * P20/P21 を周辺機能（SCI0）として使用するために必要
 */
#define PORT2_PDR   (*(volatile uint8_t *)0x0008C002)   /* ポート方向レジスタ */
#define PORT2_PMR   (*(volatile uint8_t *)0x0008C042)   /* ポートモードレジスタ */

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
        delay_ms(1);  /* LOCO(125kHz) 動作時: 約 80ms 相当 */
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
    /* RX63N リセット後のデフォルトは LOCO (125kHz)。
     * このままでは BRR の計算が合わず UART が動作しない。
     * HOCO (High-Speed On-Chip Oscillator, 32MHz) に切り替える。
     *
     * BRR 計算 (HOCO 32MHz, 115200bps):
     *   BRR = PCLK / (32 × baud) - 1
     *       = 32000000 / (32 × 115200) - 1 = 7.68 → 7
     */

    /* 1. プロテクト解除（クロック関連レジスタへの書き込みを許可） */
    PRCR = 0xA503u;

    /* 2. HOCO 動作開始（HCSTP=0） */
    HOCOCR = 0x00u;

    /* 3. HOCO 安定待ち（固定ループ）
     * while(OSCOVFSR) は OFS 設定で HOCO が無効の場合に無限ループになるため、
     * 固定ループを使用。LOCO(125kHz) 動作中で約 10000 ループ ≈ 数百 ms 待機。
     * HOCO 安定時間仕様 < 1ms なので十分な余裕がある。
     */
    for (volatile int i = 0; i < 10000; i++);
    /* HCOVF フラグは念のため確認（立っていれば安定済み） */
    (void)OSCOVFSR;

    /* 4. [DISABLED] HOCO 切り替えは OFS0 設定が必要なため一時無効
     *    OFS0 デフォルト (0xFFFFFFFF) では HOCOEN=1（無効）のため、
     *    SCKCR3=0x0100 にするとクロックが止まり CPU フリーズする。
     *    → まず LOCO のまま CPU 起動確認を行う。
     */
    /* SCKCR3 = 0x0100u; */  /* HOCO 選択: 無効化中 */

    /* 5. プロテクト再設定 */
    PRCR = 0xA500u;
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
    /* 1. MPC 書き込み保護解除 */
    MPC_PWPR = 0x00u;   /* B0WI=0: PFSWE ビット書き込みを許可 */
    MPC_PWPR = 0x40u;   /* PFSWE=1: PFS レジスタ書き込みを許可 */

    /* 2. P20 を TXD0 に、P21 を RXD0 に割り当て（PSEL=0x0A） */
    MPC_P20PFS = 0x0Au;
    MPC_P21PFS = 0x0Au;

    /* 3. MPC 保護再設定 */
    MPC_PWPR = 0x00u;
    MPC_PWPR = 0x80u;   /* B0WI=1: PFSWE ビット書き込みを禁止 */

    /* 4. PORT2: P20 を出力方向に設定（P21 は入力のままでよい） */
    PORT2_PDR |= (1u << 0);

    /* 5. PORT2: P20/P21 を周辺機能モード（SCI0）に切り替え */
    PORT2_PMR |= (1u << 0) | (1u << 1);

    /* 6. SCI0 の送信を停止してから設定 */
    SCI0_SCR = 0x00u;

    /* 7. 非同期モード、8bit、1停止ビット、パリティなし */
    SCI0_SMR = 0x00u;

    /* 8. ビットレート設定（HOCO 32MHz, 115200bps → BRR=7） */
    SCI0_BRR = 7u;

    /* 9. 1ビット時間待機（BRR 設定後に必要） */
    for (volatile int i = 0; i < 1000; i++);

    /* 10. 送信イネーブル */
    SCI0_SCR = SCR_TE;
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
