
BOARD=rpi3
TOP:=$(shell pwd)/../..
OBJ:=$(TOP)/boards/$(BOARD)/obj
ARCH:=aarch64

GIT_VERSION:=$(shell git describe --abbrev=4 --dirty --always --tags)

export COMPILER_PREFIX:=/usr/bin/aarch64-linux-gnu-

export AS:=aarch64-linux-gnu-as
export CC:=aarch64-linux-gnu-gcc
export LD:=aarch64-linux-gnu-ld
export OBJCOPY:=aarch64-linux-gnu-objcopy

#include $(TOP)/os/Makefile

CFLAGS:=-I. -I$(TOP)/incl -I$(TOP)/boards/$(BOARD) -I$(TOP)/arch/CortexA53 -I$(TOP)/arch/CortexA53/incl/BCM2837 -I$(TOP)/arch/CortexA53/incl -I$(TOP)/arch/CortexA53/BCM2837 -I/usr/lib/gcc/aarch64-linux-gnu/14/include -g -fno-builtin -ffreestanding -nostdinc -O2 -mstrict-align -mno-outline-atomics -D__KERNEL__ -DDEBUG -DDEFAULT_TRACE_LEV=0x0 -DDEFAULT_TRACE_SYS=0x0

#CFLAGS_USR:=-I$(TOP)/usr/include -I$(OBJ)/usr/libc/obj/include -I/opt/cross/lib/gcc/mipsel-linux/8.1.0/include -g -Os -fno-builtin -ffreestanding -nostdinc -mtune=r4600 -mips32 -O2 -mabicalls -DMMU -DMIPS -D__mips_o32 -msoft-float
#CFLAGS_USR:=-I$(TOP)/usr/incl -I/opt/cross/lib/gcc/mipsel-linux/8.1.0/include -g -Os -nostdinc -mtune=r4600 -mips32 -O2 -mabicalls -DMMU -DMIPS -D__mips_o32

LDFLAGS:=-g -T./rpi3.ld

#LDFLAGS_USR:=-g $(OBJ)/usr/lib/start.o $(OBJ)/usr/lib/usrlib.o $(OBJ)/usr/lib/stdlib/stdlib.o $(OBJ)/usr/lib/stdio/libstdio.o $(OBJ)/usr/lib/sys/sys.o $(OBJ)/usr/lib/gdtoa/gdtoa.o $(OBJ)/usr/lib/termios/libtermios.o -T$(OBJ)/../mips_usr.ld -L/opt/cross/lib/gcc/mipsel-linux/8.1.0 -lgcc
#LDFLAGS_USR:=-g -T$(OBJ)/../mips_usr.ld  $(OBJ)/usr/syslib/start.o $(OBJ)/usr/syslib/usrlib.o
#LDLIBS_USR:=-L$(OBJ)/usr/libc/lib -lc -L/opt/cross/lib/gcc/mipsel-linux/8.1.0 -lgcc


#all: uCore.img uroot.img
#all: uCore.bin

all: kernel8.img

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


kernel8.img: uCore
	$(OBJCOPY) -O binary $< $@

ipl/boot.bin:
	$(MAKE) -C ipl

#usr.bin.o

uCore: os version aarch64_lib CortexA53 BCM2837 mm stddrv stdlib fb fs bin_fmt
	$(LD) $(LDFLAGS) -o $@ $(OBJ)/lib/init_sect.o $(OBJ)/lib/CortexA53.o $(OBJ)/lib/BCM2837.o $(OBJ)/version.o $(OBJ)/lib/os.o $(OBJ)/lib/mm.o $(OBJ)/stddrv/stddrv.o $(OBJ)/stdlib/stdlib.o $(OBJ)/lib/fb.o $(OBJ)/lib/fs.o $(OBJ)/lib/bin_fmt.o $(OBJ)/arch/aarch64_lib/strings.o $(OBJ)/lib/end_sect.o -L/usr/lib/gcc/aarch64-linux-gnu/14 -lgcc

#$(LOBJ)/usr/usr.bin.o

version:
	echo "const char *ver=\"$(BOARD)-$(GIT_VERSION)\";" > $(OBJ)/version.c
	$(CC) -c -o $(OBJ)/version.o $(OBJ)/version.c

aarch64_lib: $(OBJ)/arch/aarch64_lib
	CFLAGS="$(CFLAGS)" TOP=$(TOP) OBJ=$(OBJ) $(MAKE) -C $<

$(OBJ)/arch/aarch64_lib: $(TOP)/arch/aarch64_lib
	mkdir -p $@
	lndir $(TOP)/arch/aarch64_lib $(OBJ)/arch/aarch64_lib

os: $(OBJ)/os
	CFLAGS="$(CFLAGS)" TOP=$(TOP) OBJ=$(OBJ) $(MAKE) -C $<

$(OBJ)/os: $(TOP)/os
	mkdir -p $@
	lndir $(TOP)/os $(OBJ)/os

CortexA53: $(OBJ)/arch/CortexA53
	CFLAGS="$(CFLAGS)" TOP=$(TOP) OBJ=$(OBJ) $(MAKE) -C $<

$(OBJ)/arch/CortexA53: $(TOP)/arch/CortexA53
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

#CortexA53: $(OBJ)/arch/CortexA53/BCM2837

BCM2837: $(OBJ)/arch/CortexA53/BCM2837
	CFLAGS="$(CFLAGS)" TOP=$(TOP) OBJ=$(OBJ) $(MAKE) -C $<

$(OBJ)/arch/CortexA53/BCM2837: $(TOP)/arch/CortexA53/BCM2837
	mkdir -p $@
	lndir $< $@

#usr.bin: $(OBJ)/usr/lib $(OBJ)/usr/sys_cmd $(OBJ)/usr/ls

usr.bin.o: $(OBJ)/usr/sys_cmd/sys_cmd.o $(OBJ)/usr/blinky/blinky.o $(OBJ)/usr/pin_test/pin_test.o
	$(LD) -o $(OBJ)/usr/usr.bin.o $(LDFLAGS_USR) $^
	$(OBJCOPY) --prefix-symbols=__usr_ $(OBJ)/usr/usr.bin.o

#$(OBJ)/usr/sys_cmd/sys_cmd.o: $(OBJ)/usr/lib/usrlib.o $(OBJ)/usr/sys_cmd
#        CFLAGS="$(CFLAGS_USR)" TOP="$(TOP)" OBJ="$(OBJ)" LDFLAGS="$(LDFLAGS_USR)" $(MAKE) -C $(OBJ)/usr/sys_cmd

$(OBJ)/usr/sys_cmd: $(TOP)/usr/sys_cmd
	mkdir -p $@
	lndir $< $@

usr.bin: $(OBJ)/usr/libc $(OBJ)/usr/syslib $(OBJ)/usr/sys_cmd/sys_cmd $(OBJ)/usr/ls $(OBJ)/usr/fb

$(OBJ)/usr/libc: $(TOP)/usr/libc
	mkdir -p $@
	lndir $< $@
	CFLAGS="$(CFLAGS_USR)" TOP=$(TOP) OBJ=$(OBJ) ARCH=$(ARCH) $(MAKE) -C $@

#$(OBJ)/usr/syslib: $(TOP)/usr/syslib $(OBJ)/usr/syslib/mips
#	mkdir -p $@
#	lndir $< $@
#	CFLAGS="$(CFLAGS_USR)" TOP=$(TOP) OBJ=$(OBJ) $(MAKE) -C $<
#
#$(OBJ)/usr/syslib/mips: $(TOP)/usr/syslib/mips
#	mkdir -p $@
#	lndir $< $@
#	CFLAGS="$(CFLAGS_USR)" TOP=$(TOP) OBJ=$(OBJ) $(MAKE) -C $<

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
	rm -rf $(OBJECTS) uCore uCore.bin uCore.img uroot rootf.img kernel8.img
	rm -rf tmp obj *.o
	$(MAKE) -C $(TOP)/usr/libc clean
