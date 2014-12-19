
define reset
	set *(unsigned int *)0xe000ed0c=0x05fa0004
end

target remote :4242
