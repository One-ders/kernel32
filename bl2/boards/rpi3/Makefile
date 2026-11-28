
BOARD=rpi3
TOP:=$(shell pwd)/../..
OBJ:=$(TOP)/boards/$(BOARD)/obj
BDIR:=$(TOP)/boards/$(BOARD)
ARCH:=aarch64

GIT_VERSION:=$(shell git describe --abbrev=4 --dirty --always --tags)

export COMPILER_PREFIX:=/usr/bin/aarch64-linux-gnu-
export GCC_INCL:=/usr/lib/gcc/aarch64-linux-gnu/16/include
export GCC_LIB:=/usr/lib/gcc/aarch64-linux-gnu/16
export MUSL_CONFIG_FLAGS:=--build=aarch64-linux --disable-shared

export AS:=aarch64-linux-gnu-as
export CC:=aarch64-linux-gnu-gcc
export LD:=aarch64-linux-gnu-ld
export OBJCOPY:=aarch64-linux-gnu-objcopy

#include $(TOP)/os/Makefile

CFLAGS:=-I. -I$(TOP)/incl -I$(TOP)/boards/$(BOARD) -I$(TOP)/arch/aarch64 -I$(TOP)/arch/aarch64/incl/BCM2837 -I$(TOP)/arch/aarch64/incl -I$(TOP)/arch/aarch64/BCM2837 -I/usr/lib/gcc/aarch64-linux-gnu/16/include -g -fno-builtin -ffreestanding -nostdinc -O2 -mstrict-align -mno-outline-atomics -D__KERNEL__ -DDEBUG -DDEFAULT_TRACE_LEV=0x0 -DDEFAULT_TRACE_SYS=0x0

CFLAGS_USR:=-I$(OBJ)/usr/include -I$(OBJ)/usr/libc/obj/include -I$(GCC_INCL) -g -Os -fno-builtin -ffreestanding -nostdinc -O2

LDFLAGS:=-g -T./rpi3.ld

LDFLAGS_USR:=-g -T$(OBJ)/../aarch64_usr.ld  $(OBJ)/usr/syslib/start.o $(OBJ)/usr/syslib/usrlib.o

LDLIBS_USR:=-L$(OBJ)/usr/libc/lib -lc -L$(GCC_LIB) -lgcc

all: kernel8.img uroot.img

fsize=$(shell stat -c "%s" tmp/fot4)
nblock=$(shell expr $(fsize) / 4096)

uroot.img: usr.bin
	rm -rf uroot
	mkdir uroot
	cp obj/usr/sys_cmd/sys_cmd uroot/init
	cp obj/usr/ls/ls uroot/ls
	cp obj/usr/fb/fb uroot/fb
	cp -r $(TOP)/usr/files/* uroot/.
	mkyaffs2image 1 uroot raw_rootf.img
	python3 -c "import sys; sys.stdout.buffer.write(b'\xff' * 4325376)" > rootf.img
	dd if=raw_rootf.img of=rootf.img conv=notrunc
	rm raw_rootf.img

kernel8.img: uCore
	$(OBJCOPY) -O binary $< $@

uCore: version bin_fmt os aarch64_lib aarch64 BCM2837 mm stddrv stdlib fb fs
	$(LD) $(LDFLAGS) -o $@ $(OBJ)/lib/init_sect.o $(OBJ)/lib/aarch64.o \
		$(OBJ)/lib/BCM2837.o $(OBJ)/version.o $(OBJ)/lib/os.o \
		$(OBJ)/lib/mm.o $(OBJ)/stddrv/stddrv.o $(OBJ)/stdlib/stdlib.o \
		$(OBJ)/lib/fb.o $(OBJ)/lib/fs.o $(OBJ)/lib/bin_fmt.o \
		$(OBJ)/arch/aarch64_lib/strings.o $(OBJ)/lib/end_sect.o \
		-L/usr/lib/gcc/aarch64-linux-gnu/16 -lgcc

version:
	mkdir -p $(OBJ)
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

aarch64: $(OBJ)/arch/aarch64
	CFLAGS="$(CFLAGS)" TOP=$(TOP) OBJ=$(OBJ) $(MAKE) -C $<

$(OBJ)/arch/aarch64: $(TOP)/arch/aarch64
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

BCM2837: $(OBJ)/arch/aarch64/BCM2837
	CFLAGS="$(CFLAGS)" TOP=$(TOP) OBJ=$(OBJ) $(MAKE) -C $<

$(OBJ)/arch/aarch64/BCM2837: $(TOP)/arch/aarch64/BCM2837
	mkdir -p $@
	lndir $< $@


#$(OBJ)/usr/libc $(OBJ)/usr/syslib $(OBJ)/usr/sys_cmd/sys_cmd $(OBJ)/usr/ls $(OBJ)/usr/fb

usr.bin: $(OBJ)/usr/libc $(OBJ)/usr/syslib $(OBJ)/usr/sys_cmd/sys_cmd $(OBJ)/usr/ls $(OBJ)/usr/fb

$(OBJ)/usr/libc: $(TOP)/usr/libc
	mkdir -p $@
	lndir $< $@
	CFLAGS="$(CFLAGS_USR)" TOP=$(TOP) OBJ=$(OBJ) ARCH=$(ARCH) $(MAKE) -C $@

$(OBJ)/usr/syslib: $(TOP)/usr/syslib $(OBJ)/usr/syslib/aarch64
	mkdir -p $@
	lndir $< $@
	CFLAGS="$(CFLAGS_USR)" TOP=$(TOP) OBJ=$(OBJ) $(MAKE) -C $<

$(OBJ)/usr/syslib/aarch64: $(TOP)/usr/syslib/aarch64
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


#$(OBJ)/usr/lib/arm/syscall_lib.o: $(TOP)/usr/lib
#	mkdir -p $(OBJ)/usr/lib/arm
#	lndir $< $(OBJ)/usr/lib
#	CFLAGS="$(CFLAGS_USR)" TOP=$(TOP) OBJ=$(OBJ) $(MAKE) -C $(OBJ)/usr/lib/aarch64

#$(OBJ)/usr/lib/arm/ffs.o: $(TOP)/usr/lib
#	mkdir -p $(OBJ)/usr/lib/arm
#	lndir $< $(OBJ)/usr/lib
#	CFLAGS="$(CFLAGS_USR)" TOP=$(TOP) OBJ=$(OBJ) $(MAKE) -C $(OBJ)/usr/lib/aarch64


clean:
	rm -rf $(OBJECTS) uCore uCore.bin uroot rootf.img kernel8.img
	rm -rf tmp obj *.o
	$(MAKE) -C $(TOP)/usr/libc clean
