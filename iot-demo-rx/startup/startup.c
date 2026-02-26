/**
 * @file startup.c
 * @brief RX63N スタートアップコード
 *
 * リセット後に CPU が最初に実行するコードです。
 * 以下の初期化を行ってから main() を呼び出します:
 *   1. スタックポインタの設定
 *   2. .data セクションの RAM へのコピー（ROM → RAM）
 *   3. .bss セクションのゼロクリア
 *   4. main() の呼び出し
 *
 * ベクタテーブル（固定ベクタ）もここで定義します。
 *
 * 参考: RX63N Group User's Manual: Hardware
 *   23. Exception Handling
 */

#include <stdint.h>

/* ===========================================================================
 * リンカスクリプトで定義されたシンボル
 * =========================================================================*/
/* RX GCC は C シンボルに "_" を付加する: _data_start → ELF "__data_start" */
extern uint32_t _data_start;    /* RAM 上の .data 開始アドレス */
extern uint32_t _data_end;      /* RAM 上の .data 終了アドレス */
extern uint32_t _data_rom_start;/* ROM 上の .data ロードアドレス */
extern uint32_t _bss_start;     /* .bss 開始アドレス */
extern uint32_t _bss_end;       /* .bss 終了アドレス */
extern uint32_t _stack_end;     /* スタックトップ（高アドレス側）*/

/* ===========================================================================
 * 関数プロトタイプ
 * =========================================================================*/
extern int main(void);

void _start(void) __attribute__((section(".text.startup")));
void default_handler(void) __attribute__((weak));

/* ===========================================================================
 * デフォルト割り込みハンドラ
 * 未定義の割り込みが発生した場合にここに飛んでくる（無限ループ）
 * デバッグ時はここでブレークポイントを張ると捕捉できる
 * =========================================================================*/
void default_handler(void)
{
    /* 未処理割り込みのキャッチ用 */
    while (1)
    {
        /* [DEBUG] ここに到達した場合は未定義の割り込みが発生している */
        /* デバッガでスタックトレースを確認すること */
    }
}

/* ===========================================================================
 * 各割り込みハンドラの weak シンボル定義
 * ユーザーが自前のハンドラを定義した場合はそちらが優先される
 * =========================================================================*/
void INT_Excep_BusFault(void)      __attribute__((weak, alias("default_handler")));
void INT_Excep_AddressFault(void)  __attribute__((weak, alias("default_handler")));
void INT_Excep_IllegalInst(void)   __attribute__((weak, alias("default_handler")));
void INT_Excep_PrivilegedInst(void) __attribute__((weak, alias("default_handler")));

/* UART (SCI0) 割り込み - Phase 3 以降で実装予定 */
void INT_Excep_SCI0_RXI0(void) __attribute__((weak, alias("default_handler")));
void INT_Excep_SCI0_TXI0(void) __attribute__((weak, alias("default_handler")));
void INT_Excep_SCI0_TEI0(void) __attribute__((weak, alias("default_handler")));

/* ===========================================================================
 * 固定ベクタテーブル
 * RX63N の固定ベクタは 0xFFFFFFD4 から始まる
 * (リンカスクリプトの .fvectors セクションに配置)
 * =========================================================================*/
typedef void (*VectorFunc)(void);

/* 固定ベクタテーブル（サイズ: 11エントリ × 4バイト = 44バイト）*/
const VectorFunc fixed_vectors[] __attribute__((section(".fvectors"))) = {
    default_handler,         /* 0xFFFFFFD4: 予約 */
    default_handler,         /* 0xFFFFFFD8: 予約 */
    default_handler,         /* 0xFFFFFFDC: 予約 */
    default_handler,         /* 0xFFFFFFE0: 予約 */
    default_handler,         /* 0xFFFFFFE4: 予約 */
    default_handler,         /* 0xFFFFFFE8: 予約 */
    default_handler,         /* 0xFFFFFFEC: 予約 */
    default_handler,         /* 0xFFFFFFF0: 予約 */
    INT_Excep_BusFault,      /* 0xFFFFFFF4: バスエラー例外 */
    INT_Excep_AddressFault,  /* 0xFFFFFFF8: アドレスエラー例外 */
    _start,                  /* 0xFFFFFFFC: リセットベクタ */
};

/* ===========================================================================
 * スタートアップ処理（リセット後の最初の処理）
 * =========================================================================*/
void _start(void)
{
    /* ----------------------------------------------------------------
     * 1. スタックポインタを設定
     * RX アーキテクチャは通常 C ランタイムで SP が設定されるが、
     * 念のため明示的に設定する
     * ---------------------------------------------------------------- */
    /* RX63N の SP は ISP (Interrupt Stack Pointer) と USP (User SP) がある */
    /* ここではシンプルに1つのスタックを使用 */

    /* ----------------------------------------------------------------
     * 2. .data セクションを ROM から RAM へコピー
     * 初期値を持つグローバル変数の初期化
     * ---------------------------------------------------------------- */
    uint32_t *src = &_data_rom_start;
    uint32_t *dst = &_data_start;

    while (dst < &_data_end)
    {
        *dst++ = *src++;
    }

    /* ----------------------------------------------------------------
     * 3. .bss セクションをゼロクリア
     * 初期値なしのグローバル変数を 0 で初期化
     * ---------------------------------------------------------------- */
    dst = &_bss_start;
    while (dst < &_bss_end)
    {
        *dst++ = 0u;
    }

    /*
     * [EXTEND: FREERTOS]
     * FreeRTOS 導入時はここで FreeRTOS の初期化を行う:
     *   xTaskCreate(main_task, "main", 512, NULL, 1, NULL);
     *   vTaskStartScheduler();
     * この場合、main() の呼び出しは不要になる
     */

    /* ----------------------------------------------------------------
     * 4. main() を呼び出す
     * ---------------------------------------------------------------- */
    main();

    /* ----------------------------------------------------------------
     * 5. main() が返ってきた場合（通常は到達しない）
     * ---------------------------------------------------------------- */
    while (1)
    {
        /* 何もしないで待機 */
    }
}
