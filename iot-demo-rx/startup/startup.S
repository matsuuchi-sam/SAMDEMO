/* ===========================================================================
 * startup.S - RX63N スタートアップコード（純粋アセンブリ版）
 *
 * RX GCC シンボル規則: C名 foo → ELF _foo
 *   startup_c_init (C) → ELF _startup_c_init
 *   _start (C)         → ELF __start
 * ===========================================================================*/

    .section .text.startup
    .global  __start
    .type    __start, @function

__start:
    /* 1. スタックポインタ(R15)を__stack_endに設定 */
    mov.l   #__stack_end, r15

    /* 2. C初期化関数を呼び出す (ELFシンボル: _startup_c_init) */
    bsr.a   _startup_c_init

    /* 3. 到達しない安全ガード */
9:
    bra.b   9b

    .size __start, . - __start


/* ===========================================================================
 * デフォルト割り込みハンドラ
 * ===========================================================================*/

    .section .text
    .global  _default_handler
    .type    _default_handler, @function

_default_handler:
8:
    bra.b   8b

    .size _default_handler, . - _default_handler


/* ===========================================================================
 * 固定ベクタテーブル
 * 0xFFFFFFD4 - 0xFFFFFFFF (11エントリ × 4バイト = 44バイト)
 * ===========================================================================*/

    .section .fvectors, "a"
    .balign 4

    .long   _default_handler    /* 0xFFFFFFD4: 予約 */
    .long   _default_handler    /* 0xFFFFFFD8: 予約 */
    .long   _default_handler    /* 0xFFFFFFDC: 予約 */
    .long   _default_handler    /* 0xFFFFFFE0: 予約 */
    .long   _default_handler    /* 0xFFFFFFE4: 予約 */
    .long   _default_handler    /* 0xFFFFFFE8: 予約 */
    .long   _default_handler    /* 0xFFFFFFEC: 予約 */
    .long   _default_handler    /* 0xFFFFFFF0: 予約 */
    .long   _default_handler    /* 0xFFFFFFF4: バスエラー */
    .long   _default_handler    /* 0xFFFFFFF8: アドレスエラー */
    .long   __start             /* 0xFFFFFFFC: リセットベクタ */
