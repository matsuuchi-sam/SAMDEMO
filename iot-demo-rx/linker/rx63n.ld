/**
 * rx63n.ld - RX63N リンカスクリプト
 *
 * RX の GCC は C シンボルに "_" を付加する（COFF スタイル）。
 * C コードの "_start" は ELF シンボル "__start" になる。
 * そのため、リンカスクリプトのシンボル名も "__xxx" 形式にする。
 *
 * GR-SAKURA (R5F563NBDxFP) メモリマップ:
 *   ROM (.text): 0xFFE00000 - 0xFFFFFFD3
 *   固定ベクタ:  0xFFFFFFD4 - 0xFFFFFFFF (44バイト)
 *   RAM:         0x00010000 - 0x0002FFFF (128KB)
 */

MEMORY
{
    ROM      (rx)  : ORIGIN = 0xFFE00000, LENGTH = 0x1FFFD4
    FVECTORS (rx)  : ORIGIN = 0xFFFFFFD4, LENGTH = 44
    RAM      (rwx) : ORIGIN = 0x00010000, LENGTH = 128K
}

/* エントリポイント: C の _start → ELF シンボル __start */
ENTRY(__start)

SECTIONS
{
    .fvectors 0xFFFFFFD4 :
    {
        KEEP(*(.fvectors))
    } > FVECTORS

    .text :
    {
        _text_start = .;
        *(.text.startup)
        *(.text)
        *(.text.*)
        *(.rodata)
        *(.rodata.*)
        _text_end = .;
    } > ROM

    .data :
    {
        __data_start = .;
        *(.data)
        *(.data.*)
        __data_end = .;
    } > RAM AT > ROM

    __data_rom_start = LOADADDR(.data);

    .bss :
    {
        __bss_start = .;
        *(.bss)
        *(.bss.*)
        *(COMMON)
        __bss_end = .;
    } > RAM

    .stack (NOLOAD) :
    {
        . = ALIGN(4);
        __stack_start = .;
        . += 0x1000;
        __stack_end = .;
    } > RAM

    /DISCARD/ :
    {
        *(.comment)
        *(.note.*)
    }
}
