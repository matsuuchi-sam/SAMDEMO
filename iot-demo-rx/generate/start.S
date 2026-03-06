/*
 * start.S - GR-SAKURA (RX63N) スタートアップコード
 *
 * リセット後の処理:
 *   1. スタックポインタ設定 (USP, ISP)
 *   2. .data セクションを ROM → RAM へコピー
 *   3. .bss セクションをゼロクリア
 *   4. HardwareSetup() 呼び出し (HOCO 50MHz)
 *   5. main() 呼び出し
 */

    .list
    .section .text

    .extern _main
    .extern _HardwareSetup
    .extern _ustack
    .extern _istack
    .extern _mdata
    .extern _data
    .extern _edata
    .extern _bss
    .extern _ebss
    .extern _rvectors_start

    .global _PowerON_Reset
    .type   _PowerON_Reset, @function

_PowerON_Reset:
    /* 1. スタックポインタ設定 */
    mvtc    #_ustack, USP
    mvtc    #_istack, ISP
    mvtc    #100h, fpsw

    /* 再配置ベクタテーブルのベースアドレスを設定 */
    mvtc    #_rvectors_start, INTB

    /* 2. .data コピー (ROM → RAM) */
    mov     #_mdata, r2
    mov     #_data, r1
    mov     #_edata, r3
    cmp     r1, r3
    beq     _bss_clear
_data_copy:
    mov.b   [r2+], r4
    mov.b   r4, [r1+]
    cmp     r1, r3
    bne     _data_copy

    /* 3. .bss クリア */
_bss_clear:
    mov     #_bss, r1
    mov     #_ebss, r3
    mov     #0, r2
    cmp     r1, r3
    beq     _call_hw_setup
_bss_loop:
    mov.b   r2, [r1+]
    cmp     r1, r3
    bne     _bss_loop

    /* 4. HardwareSetup (HOCO 50MHz) */
_call_hw_setup:
    bsr     _HardwareSetup

    /* 5. main */
    bsr     _main

_loop:
    bra     _loop

    .global _exit
    .type   _exit, @function
_exit:
    bra     _exit

    .global ___call_exitprocs
    .type   ___call_exitprocs, @function
___call_exitprocs:
    rts

    .end
