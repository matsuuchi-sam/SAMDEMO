/**
 * @file startup.c
 * @brief RX63N スタートアップコード
 *
 * 【RX GCC シンボル変換規則】
 *   C シンボル foo → ELF シンボル _foo
 *   例: C の _data_start → ELF の __data_start
 *   よって、リンカスクリプトで __data_start = . と定義したものを
 *   C から参照するには extern uint32_t _data_start と宣言する。
 *
 * 【SP 設定の理由】
 *   RX63N はリセット後 SP(R0) が不定値。
 *   GCC が生成する C 関数プロローグは push 命令でスタックを使うため、
 *   SP 設定前に C コードが実行されるとアドレスエラーが発生する。
 *   → __start をグローバルアセンブリで実装し、
 *     最初の命令で SP を設定してから C 関数を呼び出す。
 */

#include <stdint.h>

/* ===========================================================================
 * リンカスクリプトシンボル参照
 * C名(_xxx) → ELFシンボル(__xxx) に対応するため、アンダースコア1つで宣言
 * =========================================================================*/
extern uint32_t _data_start;      /* ELF: __data_start   */
extern uint32_t _data_end;        /* ELF: __data_end     */
extern uint32_t _data_rom_start;  /* ELF: __data_rom_start */
extern uint32_t _bss_start;       /* ELF: __bss_start    */
extern uint32_t _bss_end;         /* ELF: __bss_end      */
extern uint32_t _stack_end;       /* ELF: __stack_end    */

extern int main(void);

/* ===========================================================================
 * デフォルト割り込みハンドラ
 * =========================================================================*/
void default_handler(void) __attribute__((weak));
void default_handler(void)
{
    while (1);
}

void INT_Excep_BusFault(void)       __attribute__((weak, alias("default_handler")));
void INT_Excep_AddressFault(void)   __attribute__((weak, alias("default_handler")));
void INT_Excep_IllegalInst(void)    __attribute__((weak, alias("default_handler")));
void INT_Excep_PrivilegedInst(void) __attribute__((weak, alias("default_handler")));
void INT_Excep_SCI0_RXI0(void)     __attribute__((weak, alias("default_handler")));
void INT_Excep_SCI0_TXI0(void)     __attribute__((weak, alias("default_handler")));
void INT_Excep_SCI0_TEI0(void)     __attribute__((weak, alias("default_handler")));

/* ===========================================================================
 * C 言語の初期化処理（SP 設定後に呼ばれる）
 *
 * アセンブリ __start から BSR で呼び出される。
 * ELF シンボル: _startup_c_init
 * =========================================================================*/
static void startup_c_init(void) __attribute__((used, noinline));
static void startup_c_init(void)
{
    /* .data: ROM → RAM コピー */
    uint32_t *src = &_data_rom_start;
    uint32_t *dst = &_data_start;
    while (dst < &_data_end)
    {
        *dst++ = *src++;
    }

    /* .bss: ゼロクリア */
    dst = &_bss_start;
    while (dst < &_bss_end)
    {
        *dst++ = 0u;
    }

    /* main 呼び出し */
    main();

    /* main が返ってきた場合（通常は到達しない） */
    while (1);
}

/* ===========================================================================
 * アセンブリレベルのスタートアップ
 *
 * ELF シンボル __start として定義（リンカスクリプトの ENTRY(__start) に対応）
 * C から参照する場合は extern void _start(void) と宣言する。
 *
 * 処理順:
 *   1. mov.l #__stack_end, r0  → SP(R0) を設定
 *   2. bsr.a _startup_c_init   → C初期化関数（ELFシンボル: _startup_c_init）
 *   3. bra.b 9b                → 安全ガード（到達しない）
 * =========================================================================*/
__asm__(
    "   .section .text.startup              \n"
    "   .global  __start                    \n"
    "   .type    __start, @function         \n"
    "__start:                               \n"
    "   mov.l   #__stack_end, r0            \n"  /* SP = スタックトップ        */
    "   bsr.a   _startup_c_init             \n"  /* C初期化（ELF: _startup_c_init）*/
    "9: bra.b   9b                          \n"  /* 到達しない                 */
);

/* ===========================================================================
 * 固定ベクタテーブル
 * RX63N 固定ベクタ: 0xFFFFFFD4 - 0xFFFFFFFF (11エントリ, 44バイト)
 * =========================================================================*/
typedef void (*VectorFunc)(void);

/* アセンブリで定義した __start を C から参照 */
extern void _start(void);  /* C名 _start → ELF __start */

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
