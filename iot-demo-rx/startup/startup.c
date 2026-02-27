/**
 * @file startup.c
 * @brief RX63N C 言語初期化処理
 *
 * __start（アセンブリ: startup.S）から呼び出される。
 * .data コピー、.bss ゼロクリア後に main() を呼ぶ。
 *
 * ELF シンボル規則: C名 startup_c_init → ELF _startup_c_init
 * startup.S の bsr.a _startup_c_init に対応。
 */

#include <stdint.h>

/* リンカスクリプトシンボル */
extern uint32_t _data_start;      /* ELF: __data_start     */
extern uint32_t _data_end;        /* ELF: __data_end       */
extern uint32_t _data_rom_start;  /* ELF: __data_rom_start */
extern uint32_t _bss_start;       /* ELF: __bss_start      */
extern uint32_t _bss_end;         /* ELF: __bss_end        */

extern int main(void);

/* startup.S から BSR で呼ばれる（non-static で ELF シンボル _startup_c_init を公開）*/
void startup_c_init(void) __attribute__((used, noinline));
void startup_c_init(void)
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
