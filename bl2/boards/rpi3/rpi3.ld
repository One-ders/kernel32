
SECTIONS
{
	. = 0xffff000000080000;
	__kernel_start__ = .;
	.text.boot : { *(.text.boot) }
	.text :  { *(.text) }
	.rodata : { *(.rodata) }
	.data : { *(.data) }
	. = ALIGN(0x8);
	__bss_start__ = .;
	.bss : { *(.bss*) }
	__bss_end__ = .;
	. = ALIGN(0x00001000);
	pg_dir_121 = .;
	.data.pgd_121 : { . += (3 * (1 << 12)); }
	. = ALIGN(0x00001000);
	pg_dir = .;
	.data.pgd : { . += (3 * (1 << 12)); }
	__static_mem_end__ = .;
}
