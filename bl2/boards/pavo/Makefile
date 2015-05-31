
BOARD=pavo
TOP:=$(shell pwd)/../..
OBJ:=$(TOP)/boards/$(BOARD)/obj

GIT_VERSION:=$(shell git describe --abbrev=4 --dirty --always --tags)

export AS:=mipsel-linux-as
export CC:=mipsel-linux-gcc
export LD:=mipsel-linux-ld
export OBJCOPY:=mipsel-linux-objcopy
 
#include $(TOP)/os/Makefile

#CFLAGS:=-I. -I$(TOP)/incl -I$(TOP)/arch/mips -I$(TOP)/arch/mips/incl -I$(TOP)/arch/mips/jz4740 -I/opt/cross/lib/gcc/mipsel-linux/4.9.1/include -g -Os -fno-builtin -ffreestanding -nostdinc -mtune=r4600 -mips32 -O2 -mabicalls
CFLAGS:=-I. -I$(TOP)/incl -I$(TOP)/boards/$(BOARD) -I$(TOP)/arch/mips -I$(TOP)/arch/mips/incl -I$(TOP)/arch/mips/jz4740 -I/opt/cross/lib/gcc/mipsel-linux/4.9.1/include -g -Os -fno-builtin -ffreestanding -nostdinc -mtune=r4600 -mips32 -O2 -mabicalls -DDEBUG
CFLAGS_USR:=-I$(TOP)/usr/incl -I/opt/cross/lib/gcc/mipsel-linux/4.9.1/include -g -Os -fno-builtin -ffreestanding -nostdinc -mtune=r4600 -mips32 -O2 -mabicalls
LDFLAGS:=-g -T./pavo.ld 
LDFLAGS_USR:=-g $(OBJ)/usr/lib/start.o $(OBJ)/usr/lib/usrlib.o -T$(OBJ)/../pavo_usr.ld


all: uCore.img

fsize=$(shell stat -c "%s" tmp/fot4)
nblock=$(shell expr $(fsize) / 4096)

uCore.img: tmp/fot5 usr.bin
	cat tmp/fot5 obj/usr/sys_cmd/sys_cmd.srec tmp/fot1 > uCore.img

tmp/fot5: tmp/fot4
	dd if=tmp/fot4 of=tmp/fot5 ibs=4096 count=$(nblock)

tmp/fot4: tmp/fot3 uCore.bin
	cat tmp/fot3 uCore.bin tmp/fot1 > tmp/fot4
	
tmp/fot3: tmp/fot2
	dd if=tmp/fot2 of=tmp/fot3 ibs=4096 count=1

tmp/fot2: ipl/boot.bin tmp/fot1
	cat ipl/boot.bin tmp/fot1 > tmp/fot2
	
	

tmp/fot1:
	mkdir -p tmp
	dd if=/dev/zero of=tmp/fot1 ibs=4096 count=1
	

uCore.bin: uCore
	$(OBJCOPY) -O binary $< $@

ipl/boot.bin:
	$(MAKE) -C ipl

uCore: os version mips jz c_start.o stddrv
	$(LD) $(LDFLAGS) -o $@ $(OBJ)/lib/jz.o c_start.o $(OBJ)/version.o $(OBJ)/lib/init_sect.o $(OBJ)/lib/mips.o $(OBJ)/lib/os.o $(OBJ)/stddrv/stddrv.o $(OBJ)/lib/end_sect.o

#traps.o: ../../mips/jz4740/traps.S
#	$(CC) $(CFLAGS) -c -o $@ $<

c_start.o: ./c_start.c
	$(CC) $(CFLAGS) -c -o $@ $<



#jz4740.o: ../../../mips/jz4740/jz4740.c
#	$(CC) $(CFLAGS) -c -o $@ $<

#jz_serial.o: ../../mips/jz4740/jz_serial.c
#	$(CC) $(CFLAGS) -c -o $@ $<

version: 
	echo "const char *ver=\"$(BOARD)-$(GIT_VERSION)\";" > $(OBJ)/version.c
	$(CC) -c -o $(OBJ)/version.o $(OBJ)/version.c

os: $(OBJ)/os
	CFLAGS="$(CFLAGS)" TOP=$(TOP) OBJ=$(OBJ) $(MAKE) -C $<

$(OBJ)/os: $(TOP)/os
	mkdir -p $@
	lndir $(TOP)/os $(OBJ)/os

mips: $(OBJ)/arch/mips
	CFLAGS="$(CFLAGS)" TOP=$(TOP) OBJ=$(OBJ) $(MAKE) -C $<

$(OBJ)/arch/mips: $(TOP)/arch/mips
	mkdir -p $@
	lndir $< $@

stddrv: $(OBJ)/stddrv
	CFLAGS="$(CFLAGS)" TOP=$(TOP) OBJ=$(OBJ) $(MAKE) -C $<

$(OBJ)/stddrv: $(TOP)/stddrv
	mkdir -p $@
	lndir $< $@

jz: $(OBJ)/arch/mips/jz4740
	CFLAGS="$(CFLAGS)" TOP=$(TOP) OBJ=$(OBJ) $(MAKE) -C $<

$(OBJ)/arch/mips/jz4740: $(TOP)/arch/mips/jz4740
	mkdir -p $@
	lndir $< $@

usr.bin: $(OBJ)/usr/lib $(OBJ)/usr/sys_cmd


$(OBJ)/usr/lib: $(TOP)/usr/lib $(OBJ)/usr/lib/mips
	mkdir -p $@
	lndir $< $@
	CFLAGS="$(CFLAGS_USR)" TOP=$(TOP) OBJ=$(OBJ) $(MAKE) -C $<

$(OBJ)/usr/lib/mips: $(TOP)/usr/lib/mips
	mkdir -p $@
	lndir $< $@
	CFLAGS="$(CFLAGS_USR)" TOP=$(TOP) OBJ=$(OBJ) $(MAKE) -C $<

$(OBJ)/usr/sys_cmd: $(TOP)/usr/sys_cmd
	mkdir -p $@
	lndir $< $@
	CFLAGS="$(CFLAGS_USR)" LDFLAGS="$(LDFLAGS_USR)" TOP=$(TOP) OBJ=$(OBJ) $(MAKE) -C $<


clean:
	rm -rf $(OBJECTS) uCore uCore.bin uCore.img
	rm -rf tmp obj *.o
	$(MAKE) -C ipl clean
