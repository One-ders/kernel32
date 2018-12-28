
BOARD=pavo
TOP:=$(shell pwd)/../..
OBJ:=$(TOP)/boards/$(BOARD)/obj
ARCH:=mips

GIT_VERSION:=$(shell git describe --abbrev=4 --dirty --always --tags)

export COMPILER_PREFIX:=/opt/cross/bin/mipsel-linux-

export AS:=mipsel-linux-as
export CC:=mipsel-linux-gcc
export LD:=mipsel-linux-ld
export OBJCOPY:=mipsel-linux-objcopy
 
#include $(TOP)/os/Makefile

#CFLAGS:=-I. -I$(TOP)/incl -I$(TOP)/arch/mips -I$(TOP)/arch/mips/incl -I$(TOP)/arch/mips/jz4740 -I/opt/cross/lib/gcc/mipsel-linux/8.1.0/include -g -Os -fno-builtin -ffreestanding -nostdinc -mtune=r4600 -mips32 -O2 -mabicalls
CFLAGS:=-I. -I$(TOP)/incl -I$(TOP)/boards/$(BOARD) -I$(TOP)/arch/mips -I$(TOP)/arch/mips/incl/jz4740 -I$(TOP)/arch/mips/incl -I$(TOP)/arch/mips/jz4740 -I/opt/cross/lib/gcc/mipsel-linux/8.1.0/include -g -Os -fno-builtin -ffreestanding -nostdinc -mtune=r4600 -mips32 -O2 -mabicalls -D__KERNEL__ -DDEBUG -DDEFAULT_TRACE_LEV=0x0 -DDEFAULT_TRACE_SYS=0x0
CFLAGS_USR:=-I$(TOP)/usr/include -I$(OBJ)/usr/libc/obj/include -I/opt/cross/lib/gcc/mipsel-linux/8.1.0/include -g -Os -fno-builtin -ffreestanding -nostdinc -mtune=r4600 -mips32 -O2 -mabicalls -DMMU -DMIPS -D__mips_o32 -msoft-float
#CFLAGS_USR:=-I$(TOP)/usr/incl -I/opt/cross/lib/gcc/mipsel-linux/8.1.0/include -g -Os -nostdinc -mtune=r4600 -mips32 -O2 -mabicalls -DMMU -DMIPS -D__mips_o32
LDFLAGS:=-g -T./pavo.ld 
#LDFLAGS_USR:=-g $(OBJ)/usr/lib/start.o $(OBJ)/usr/lib/usrlib.o $(OBJ)/usr/lib/stdlib/stdlib.o $(OBJ)/usr/lib/stdio/libstdio.o $(OBJ)/usr/lib/sys/sys.o $(OBJ)/usr/lib/gdtoa/gdtoa.o $(OBJ)/usr/lib/termios/libtermios.o -T$(OBJ)/../pavo_usr.ld -L/opt/cross/lib/gcc/mipsel-linux/8.1.0 -lgcc
LDFLAGS_USR:=-g -T$(OBJ)/../pavo_usr.ld  $(OBJ)/usr/syslib/start.o $(OBJ)/usr/syslib/usrlib.o
LDLIBS_USR:=-L$(OBJ)/usr/libc/lib -lc -L/opt/cross/lib/gcc/mipsel-linux/8.1.0 -lgcc


all: uCore.img uroot.img

fsize=$(shell stat -c "%s" tmp/fot4)
nblock=$(shell expr $(fsize) / 4096)

##uCore.img: tmp/fot5 usr.bin
##	cat tmp/fot5 obj/usr/sys_cmd/sys_cmd.srec tmp/fot1 > uCore.img
##	rm -rf uroot
##	mkdir uroot
##	cp obj/usr/sys_cmd/sys_cmd uroot/init
##	mkyaffs2image 1 uroot rootf.img

##tmp/fot5: tmp/fot4
##	dd if=tmp/fot4 of=tmp/fot5 ibs=4096 count=$(nblock)
##
##tmp/fot4: tmp/fot3 uCore.bin
uCore.img: tmp/fot3 uCore.bin
	cat tmp/fot3 uCore.bin tmp/fot1 > uCore.img

uroot.img: usr.bin
	rm -rf uroot
	mkdir uroot
	cp obj/usr/sys_cmd/sys_cmd uroot/init
	cp obj/usr/ls/ls uroot/ls
	cp obj/usr/fb/fb uroot/fb
	cp -r $(TOP)/usr/files/* uroot/.
	mkyaffs2image 1 uroot rootf.img
	
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

uCore: os version mips jz mm stddrv stdlib fb fs bin_fmt
	$(LD) $(LDFLAGS) -o $@ $(OBJ)/lib/init_sect.o $(OBJ)/lib/jz.o $(OBJ)/version.o $(OBJ)/lib/mips.o $(OBJ)/lib/os.o $(OBJ)/lib/mm.o $(OBJ)/stddrv/stddrv.o $(OBJ)/stdlib/stdlib.o $(OBJ)/lib/fb.o $(OBJ)/lib/fs.o $(OBJ)/lib/bin_fmt.o $(OBJ)/lib/end_sect.o -L/opt/cross/lib/gcc/mipsel-linux/8.1.0 -lgcc

#traps.o: ../../mips/jz4740/traps.S
#	$(CC) $(CFLAGS) -c -o $@ $<

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

stdlib: $(OBJ)/stdlib
	CFLAGS="$(CFLAGS)" TOP=$(TOP) OBJ=$(OBJ) $(MAKE) -C $<

$(OBJ)/stdlib: $(TOP)/stdlib
	mkdir -p $@
	lndir $< $@


fb: $(OBJ)/drivers/fb
	CFLAGS="$(CFLAGS)" TOP=$(TOP) OBJ=$(OBJ) $(MAKE) -C $<

$(OBJ)/drivers/fb: $(TOP)/drivers/fb
	mkdir -p $@
	lndir $< $@

fs: $(OBJ)/fs/yaffs2
	CFLAGS="$(CFLAGS)" TOP=$(TOP) OBJ=$(OBJ) $(MAKE) -C $<

$(OBJ)/fs/yaffs2: $(TOP)/fs/yaffs2
	mkdir -p $@
	lndir $< $@

bin_fmt: $(OBJ)/bin_fmt/elf
	CFLAGS="$(CFLAGS)" TOP=$(TOP) OBJ=$(OBJ) $(MAKE) -C $<

$(OBJ)/bin_fmt/elf: $(TOP)/bin_fmt/elf
	mkdir -p $@
	lndir $< $@

mm: $(OBJ)/mm
	CFLAGS="$(CFLAGS)" TOP=$(TOP) OBJ=$(OBJ) $(MAKE) -C $<

$(OBJ)/mm: $(TOP)/mm
	mkdir -p $@
	lndir $< $@

jz: $(OBJ)/arch/mips/jz4740
	CFLAGS="$(CFLAGS)" TOP=$(TOP) OBJ=$(OBJ) $(MAKE) -C $<

$(OBJ)/arch/mips/jz4740: $(TOP)/arch/mips/jz4740
	mkdir -p $@
	lndir $< $@

#usr.bin: $(OBJ)/usr/lib $(OBJ)/usr/sys_cmd $(OBJ)/usr/ls

usr.bin: $(OBJ)/usr/libc $(OBJ)/usr/syslib $(OBJ)/usr/sys_cmd/sys_cmd $(OBJ)/usr/ls $(OBJ)/usr/fb

$(OBJ)/usr/libc: $(TOP)/usr/libc
	mkdir -p $@
	lndir $< $@
	CFLAGS="$(CFLAGS_USR)" TOP=$(TOP) OBJ=$(OBJ) ARCH=$(ARCH) $(MAKE) -C $@

$(OBJ)/usr/syslib: $(TOP)/usr/syslib $(OBJ)/usr/syslib/mips 
	mkdir -p $@
	lndir $< $@
	CFLAGS="$(CFLAGS_USR)" TOP=$(TOP) OBJ=$(OBJ) $(MAKE) -C $<

$(OBJ)/usr/syslib/mips: $(TOP)/usr/syslib/mips
	mkdir -p $@
	lndir $< $@
	CFLAGS="$(CFLAGS_USR)" TOP=$(TOP) OBJ=$(OBJ) $(MAKE) -C $<

$(OBJ)/usr/sys_cmd/sys_cmd: $(TOP)/usr/sys_cmd
	mkdir -p $(dir $@)
	lndir $< $(dir $@)
	CFLAGS="$(CFLAGS_USR)" LDFLAGS="$(LDFLAGS_USR)" LDLIBS="$(LDLIBS_USR)" TOP=$(TOP) OBJ=$(OBJ) $(MAKE) -C $<

$(OBJ)/usr/ls: $(TOP)/usr/ls
	mkdir -p $@
	lndir $< $@
	CFLAGS="$(CFLAGS_USR)" LDFLAGS="$(LDFLAGS_USR)" LDLIBS="$(LDLIBS_USR)" TOP=$(TOP) OBJ=$(OBJ) $(MAKE) -C $<


$(OBJ)/usr/fb: $(TOP)/usr/fb
	mkdir -p $@
	lndir $< $@
	CFLAGS="$(CFLAGS_USR)" LDFLAGS="$(LDFLAGS_USR)" LDLIBS="$(LDLIBS_USR)" TOP=$(TOP) OBJ=$(OBJ) $(MAKE) -C $<



clean:
	rm -rf $(OBJECTS) uCore uCore.bin uCore.img uroot rootf.img
	rm -rf tmp obj *.o
	$(MAKE) -C ipl clean
	$(MAKE) -C $(TOP)/usr/libc clean
