/*
 * start.S - 最小限スタートアップ（診断用）
 *
 * .data コピー、BSS クリア、HardwareSetup、ユーザーモード遷移を
 * 全てスキップし、リセットから直接 main を呼ぶ。
 * これで LED が光れば、元の start.S のどこかに問題がある。
 */

    .list
    .section .text

    .extern _main
    .extern _ustack
    .extern _istack

    .global _PowerON_Reset
    .type   _PowerON_Reset, @function

_PowerON_Reset:
    /* スタックポインタだけ設定 */
    mvtc    #_ustack, USP
    mvtc    #_istack, ISP
    mvtc    #100h, fpsw

    /* 直接 main へ（スーパーバイザモードのまま） */
    bsr     _main

_loop:
    bra     _loop

    /* _exit シンボル（リンクエラー回避） */
    .global _exit
    .type   _exit, @function
_exit:
    bra     _exit

    .global ___call_exitprocs
    .type   ___call_exitprocs, @function
___call_exitprocs:
    rts

    .end
