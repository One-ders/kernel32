
/* Binary composition and layout of sections  */

MEMORY {
	FLASH (rx)	: ORIGIN = 0x08000000, LENGTH = 0x100000 /* 1 Meg */
	RAM (rwx)	: ORIGIN = 0x20000000, LENGTH = 0x020000 /* 128 K */
}

SECTIONS {
	.text : { 
		* (.vectors);
		* (.text);
		* (.text*);
		* (.init);
		* (.fini);
		* (.eh_frame);
		* (.rodata); 
		* (.rodata*); 
	} >FLASH

	.ARM.extab   : { 
		*(.ARM.extab* .gnu.linkonce.armextab.*) 
	} >FLASH

	.ARM : {
		__exidx_start = .;
		*(.ARM.exidx*)
		__exidx_end = .;
	} >FLASH
 
	_sidata = .;
	.data : AT ( _sidata ) {
		__data_start__ = .;
		 *(.data); 
		 *(.data*); 
		__data_end__ = .;
	} >RAM

	.bss : { 
		__bss_start__ = .;
		__usr___bss_start__ = .;
		* (.bss); 
		* (.bss*); 
		* (COMMON); 
		__usr___bss_end__ = .;
		__bss_end__ = .;
	} >RAM
	bss_size = __bss_end__ - __bss_start__;
	. = 0x20020000;
	__stack_end__ =.;

	/DISCARD/ : {
		libc.a ( * )
		libm.a ( * )
		libgcc.a ( * )
	}
	
}
