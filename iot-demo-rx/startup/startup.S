/* ===========================================================================
 * startup.S  -  RX63N スタートアップコード
 *
 * Renesas e2studio 生成コード (generate/start.S + generate/vects.c) をベースに
 * SAMDEMO プロジェクト用へ移植。
 *
 * 主な変更点（旧コードからの差分）:
 *   - mov.l #__stack_end, r15  →  mvtc 命令で ISP/USP を正しく設定
 *   - mvtc で FPSW / INTB / PSW を初期化
 *   - .data ROM→RAM コピー / .bss ゼロクリアをインラインアセンブリで実施
 *   - _main を直接呼び出す（startup.c 経由を廃止）
 *   - 固定ベクタ (.fvectors) を 0xFFFFFF80 に拡張（OFS/MDES 含む）
 * ===========================================================================*/

    .section .text.startup
    .extern _main
    .extern _istack
    .extern _ustack
    .extern _rvectors_start
    .extern __data_start
    .extern __data_rom_start
    .extern __data_end
    .extern __bss_start
    .extern __bss_end

    .global __start
    .type   __start, @function

__start:
    /* --- 1. スタックポインタ初期化 --- */
    /* ISP (Interrupt Stack Pointer): 割り込み/例外処理用 */
    mvtc    #_istack, ISP

    /* USP (User Stack Pointer): 通常実行用 */
    mvtc    #_ustack, USP

    /* --- 2. 割り込みベクタベースアドレス設定 --- */
    mvtc    #_rvectors_start, intb

    /* --- 3. 浮動小数点ステータスワード初期化 --- */
    mvtc    #0x100, fpsw

    /* --- 4. .data セクション: ROM 初期値 → RAM コピー --- */
    /* r2 = 初期値の ROM アドレス (LMA)  */
    /* r1 = RAM 転送先の先頭アドレス     */
    /* r3 = コピーサイズ (バイト)        */
    mov     #__data_rom_start, r2
    mov     #__data_start,     r1
    mov     #__data_end,       r3
    sub     r1, r3              /* r3 = end - start (サイズ計算) */
    cmp     #0, r3
    beq     .Ldata_done
.Ldata_copy:
    mov.b   [r2+], r5
    mov.b   r5, [r1+]
    sub     #1, r3
    bne     .Ldata_copy
.Ldata_done:

    /* --- 5. .bss セクション: ゼロクリア --- */
    mov     #0x00,        r2
    mov     #__bss_start, r1
    mov     #__bss_end,   r3
.Lbss_zero:
    cmp     r1, r3
    beq     .Lbss_done
    mov.b   r2, [r1+]
    bra     .Lbss_zero
.Lbss_done:

    /* --- 6. PSW 設定: U ビット(USP 選択) + IPL=0(割り込み許可) --- */
    mvtc    #0x10000, psw

    /* --- 7. main 呼び出し --- */
    mov     #_main, r7
    jsr     r7

    /* --- 8. main が戻った場合は無限ループ --- */
.Lfin:
    bra     .Lfin

    .size __start, . - __start


/* ===========================================================================
 * デフォルト割り込みハンドラ（無限ループ）
 * ===========================================================================*/
    .section .text
    .global  _default_handler
    .type    _default_handler, @function

_default_handler:
.Ldefault_loop:
    bra     .Ldefault_loop

    .size _default_handler, . - _default_handler


/* ===========================================================================
 * 再配置可能割り込みベクタテーブル（最小構成）
 * mvtc #_rvectors_start, intb でハードウェアが参照するテーブル
 * ===========================================================================*/
    .section .rvectors, "a"
    .balign 4
    .long   _default_handler    /* 0x0000: BRK 命令ハンドラ */


/* ===========================================================================
 * 固定ベクタテーブル (0xFFFFFF80 - 0xFFFFFFFF, 128 バイト)
 *
 * e2studio generate/vects.c ベース:
 *   0xFFFFFF80: MDES  エンディアン選択  (0xFFFFFFFF = リトルエンディアン)
 *   0xFFFFFF88: OFS1  オプション設定1   (0xFFFFFFFF = 保護なし)
 *   0xFFFFFF8C: OFS0  オプション設定0   (0xFFFFFFFF = 保護なし)
 *   0xFFFFFFD0: スーパーバイザ命令例外
 *   0xFFFFFFD4: アクセス命令例外
 *   0xFFFFFFDC: 未定義命令例外
 *   0xFFFFFFE4: 浮動小数点例外
 *   0xFFFFFFF8: NMI
 *   0xFFFFFFFC: リセットベクタ (Power-On Reset → __start)
 * ===========================================================================*/
    .section .fvectors, "a"
    .balign 4

    /* 0xFFFFFF80 */ .long   0xFFFFFFFF  /* MDES (リトルエンディアン) */
    /* 0xFFFFFF84 */ .long   0x00000000  /* 予約 */
    /* 0xFFFFFF88 */ .long   0xFFFFFFFF  /* OFS1 */
    /* 0xFFFFFF8C */ .long   0xFFFFFFFF  /* OFS0 */
    /* 0xFFFFFF90 */ .long   0x00000000  /* 予約 */
    /* 0xFFFFFF94 */ .long   0x00000000  /* 予約 */
    /* 0xFFFFFF98 */ .long   0x00000000  /* 予約 */
    /* 0xFFFFFF9C */ .long   0x00000000  /* 予約 */
    /* 0xFFFFFFA0 */ .long   0xFFFFFFFF  /* 予約 */
    /* 0xFFFFFFA4 */ .long   0xFFFFFFFF  /* 予約 */
    /* 0xFFFFFFA8 */ .long   0xFFFFFFFF  /* 予約 */
    /* 0xFFFFFFAC */ .long   0xFFFFFFFF  /* 予約 */
    /* 0xFFFFFFB0 */ .long   0x00000000  /* 予約 */
    /* 0xFFFFFFB4 */ .long   0x00000000  /* 予約 */
    /* 0xFFFFFFB8 */ .long   0x00000000  /* 予約 */
    /* 0xFFFFFFBC */ .long   0x00000000  /* 予約 */
    /* 0xFFFFFFC0 */ .long   0x00000000  /* 予約 */
    /* 0xFFFFFFC4 */ .long   0x00000000  /* 予約 */
    /* 0xFFFFFFC8 */ .long   0x00000000  /* 予約 */
    /* 0xFFFFFFCC */ .long   0x00000000  /* 予約 */
    /* 0xFFFFFFD0 */ .long   _default_handler  /* スーパーバイザ命令例外 */
    /* 0xFFFFFFD4 */ .long   _default_handler  /* アクセス命令例外 */
    /* 0xFFFFFFD8 */ .long   0x00000000        /* 予約 */
    /* 0xFFFFFFDC */ .long   _default_handler  /* 未定義命令例外 */
    /* 0xFFFFFFE0 */ .long   0x00000000        /* 予約 */
    /* 0xFFFFFFE4 */ .long   _default_handler  /* 浮動小数点例外 */
    /* 0xFFFFFFE8 */ .long   0x00000000        /* 予約 */
    /* 0xFFFFFFEC */ .long   0x00000000        /* 予約 */
    /* 0xFFFFFFF0 */ .long   0x00000000        /* 予約 */
    /* 0xFFFFFFF4 */ .long   0x00000000        /* 予約 */
    /* 0xFFFFFFF8 */ .long   _default_handler  /* NMI */
    /* 0xFFFFFFFC */ .long   __start           /* リセットベクタ */
