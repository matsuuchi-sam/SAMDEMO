/*
* Copyright (c) 2024(2025) Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
*/
/* e2studio 生成コード (demo/generate/linker_script.ld) より */

MEMORY
{
	RAM : ORIGIN = 0x0, LENGTH = 262144
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
	.ustack 0x200: AT(0x200)
	{
		_ustack = .;
	} > RAM
	.istack 0x100: AT(0x100)
	{
		_istack = .;
	} > RAM
	.data 0x204: AT(_mdata)
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
}
