OUTPUT_ARCH(aarch64)
ENTRY(_start)
SECTIONS
{
        /* ========================================================
           SEKTION 1: KOD & READ-ONLY DATA (ALLTID PÅ 4MB)
           Mappas i kernel som: USER_READONLY | USER_EXECUTE
           ======================================================== */
        . = 0x00400000;

        /* Tvinga fram 4KB-justering för starten av text (kod) */
        .text : ALIGN(4096)
        {
          *(.text .text.*)
        }

        /* Lägg rodata direkt efter .text på en egen jämn sidgräns */
        .rodata : ALIGN(4096)
        {
          *(.rodata .rodata.*)
        }
        .rodata1 : { *(.rodata1) }

        /* ========================================================
           SEKTION 2: DATA, BSS & HEAP (ALLTID PÅ 16MB)
           Mappas i kernel som: USER_READWRITEEXECUTE
           ======================================================== */
        . = 0x01000000;

        /* Starten på datasegmentet justeras hårt till en ny 4KB-sida */
        .data : ALIGN(4096)
        {
          *(.data .data.*)
        }
        .data1  : { *(.data1) }

        /* Global Offset Table (används av dynamiska/musl-strukturer) */
        .got    : { *(.got.plt) *(.got) }

        _edata = .; PROVIDE (edata = .);

        /* ========================================================
           SEKTION 3: BSS
           ======================================================== */
        . = ALIGN(4096);
        __bss_start__ = .;

        .bss : ALIGN(4096)
        {
                *(.dynbss)
                *(.bss .bss.*)
                *(COMMON)
        }

        /* Slutjustera BSS och sätt Heap-starten på en ren sidgräns */
        . = ALIGN(4096);
        __bss_end__ = .;
        _end = .; PROVIDE (end = .);
}

