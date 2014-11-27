
AS:=mipsel-linux-as
CC:=mipsel-linux-gcc
LD:=mipsel-linux-ld
OBJCOPY:=mipsel-linux-objcopy

CFLAGS:=-I../. -I../../../../incl -I../../../mips -I../../../mips/jz4740 -g -Os -fno-builtin -ffreestanding -nostdinc -mtune=r4600 -mips32 -O2 -mabicalls
LDFLAGS:=-g -T./pavo_ipl.ld 

OBJECTS=ipl_start.o nand_boot.o jz_serial.o jz4740.o

all: boot.bin uCore

boot.bin: boot
	$(OBJCOPY) -O binary $< $@

boot: $(OBJECTS)
	$(LD) $(LDFLAGS) -o $@ $(OBJECTS)

ipl_start.o: ../../../mips/jz4740/ipl_start.S
	$(CC) $(CFLAGS) -c -o $@ $<

jz4740.o: ../../../mips/jz4740/jz4740.c
	$(CC) $(CFLAGS) -c -o $@ $<

jz_serial.o: ../../../mips/jz4740/jz_serial.c
	$(CC) -DIPL $(CFLAGS) -c -o $@ $<


uCore: 


clean:
	rm -rf $(OBJECTS) boot boot.bin
