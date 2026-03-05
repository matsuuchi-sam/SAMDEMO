/* rx63n_freertos.ld - FreeRTOS対応リンカスクリプト
 *
 * RAM: 0x00000000 - 0x0001FFFF (128KB)
 *   0x00000000 - 0x000000FF  予約
 *   0x00000100 - 0x0000FFFF  .data + .bss + FreeRTOS heap (~64KB)
 *   0x0001E000 - 0x0001FBFF  ISP (割り込みスタック 1KB)
 *   0x0001FC00 - 0x0001FFFF  USP (ユーザースタック 1KB, startup用)
 *
 * ROM: 0xFFF00000 - 0xFFFFFFFF (1MB)
 */

MEMORY
{
	RAM : ORIGIN = 0x0, LENGTH = 131072
	ROM : ORIGIN = 0xFFF00000, LENGTH = 1048576
}
SECTIONS
{
	.fvectors 0xFFFFFF80: AT(0xFFFFFF80)
	{
		KEEP(*(.fvectors))
	} > ROM
	.text 0xFFF00000: AT(0xFFF00000)
	{
		*(.text)
		*(.text.*)
		*(P)
		etext = .;
	} > ROM
	.rvectors ALIGN(4):
	{
		_rvectors_start = .;
		KEEP(*(.rvectors))
		_rvectors_end = .;
	} > ROM
	.init :
	{
		KEEP(*(.init))
		__preinit_array_start = .;
		KEEP(*(.preinit_array))
		__preinit_array_end = .;
		__init_array_start = (. + 3) & ~ 3;
		KEEP(*(.init_array))
		KEEP(*(SORT(.init_array.*)))
		__init_array_end = .;
		__fini_array_start = .;
		KEEP(*(.fini_array))
		KEEP(*(SORT(.fini_array.*)))
		__fini_array_end = .;
	} > ROM
	.fini :
	{
		KEEP(*(.fini))
	} > ROM
	.got :
	{
		*(.got)
		*(.got.plt)
	} > ROM
	.rodata :
	{
		*(.rodata)
		*(.rodata.*)
		*(C_1)
		*(C_2)
		*(C)
		_erodata = .;
	} > ROM
	gcc_exceptions_table :
	{
	    KEEP (*(.gcc_except_table))
	    *(.gcc_except_table.*)
	} > ROM
	.eh_frame_hdr :
	{
		*(.eh_frame_hdr)
	} > ROM
	.eh_frame :
	{
		*(.eh_frame)
	} > ROM
	.jcr :
	{
		*(.jcr)
	} > ROM
	.tors :
	{
		__CTOR_LIST__ = .;
		. = ALIGN(2);
		___ctors = .;
		*(.ctors)
		___ctors_end = .;
		__CTOR_END__ = .;
		__DTOR_LIST__ = .;
		___dtors = .;
		*(.dtors)
		___dtors_end = .;
		__DTOR_END__ = .;
		. = ALIGN(2);
		_mdata = .;
	} > ROM
	.data 0x100: AT(_mdata)
	{
		_data = .;
		*(.data)
		*(.data.*)
		*(D)
		*(D_1)
		*(D_2)
		*(.sdata.*)
		_edata = .;
	} > RAM
	.bss :
	{
		_bss = .;
		*(.bss)
		*(.bss.**)
		*(COMMON)
		*(B)
		*(B_1)
		*(B_2)
		_ebss = .;
		. = ALIGN(128);
		_end = .;
	} > RAM AT>RAM
	/* ISP (割り込みスタック): 1KB */
	.istack 0x1E400: AT(0x1E400)
	{
		_istack = .;
	} > RAM
	/* USP (ユーザースタック): 1KB - startup時のみ使用 */
	.ustack 0x1E800: AT(0x1E800)
	{
		_ustack = .;
	} > RAM
}
