
OUTPUT_FORMAT("elf32-tradlittlemips", "elf32-tradlittlemips", "elf32-tradlittlemips")

OUTPUT_ARCH(mips)
ENTRY(_start)
SECTIONS
{
        . = 0x0040000;

        . = ALIGN(4);
        .text       :
        {
          *(.text)
        }

        . = 0x10000000;
        . = ALIGN(4);
        .rodata  : { *(.rodata) }

        . = ALIGN(4);
        .data  : { *(.data) }

        . = ALIGN(4);
        .sdata  : { *(.sdata) }
       _gp = ALIGN(16);

        __got_start = .;
        .got  : { *(.got) }
        __got_end = .;

        .sdata  : { *(.sdata) }

        num_got_entries = (__got_end - __got_start) >> 2;

        . = ALIGN(4);
	__bss_start__ = .;
        .sbss  : { *(.sbss) }
        .bss  : { *(.bss) }
	__bss_end__ = .;
}

