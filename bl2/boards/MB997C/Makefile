
DRIVERS:=hr_timer.o
DRIVERS+=stddrv.o
DRIVERS+=usb_core_drv.o
DRIVERS+=usb_serial_drv.o
DRIVERS+=usr_drv.o

DTARGETS=$(DRIVERS:.o=)
DOBJS=$(patsubst %, $(OBJ)/drv_obj/%, $(DRIVERS))

BOARD=MB997C
TOP:=$(shell pwd)/../..
OBJ:=$(TOP)/boards/$(BOARD)/obj
GCC_BASE:=$(shell dirname `which arm-none-eabi-gcc`)/..

GIT_VERSION:=$(shell git describe --abbrev=4 --dirty --always --tags)

export AS:=arm-none-eabi-as
export CC:=arm-none-eabi-gcc
export LD:=arm-none-eabi-ld
export OBJCOPY:=arm-none-eabi-objcopy

CFLAGS=-I. -I$(TOP)/incl -I$(TOP)/boards/$(BOARD) -I$(TOP)/arch/stm32 -I$(TOP)/arch/stm32/incl -g -O2 -DDEBUG -Wall -mlittle-endian -mthumb -mcpu=cortex-m4 -mthumb-interwork -mfloat-abi=soft -fno-builtin

CFLAGS_USR:=-I$(TOP)/usr/incl -I$(TOP)/boards/$(BOARD) -I$(GCC_BASE)/lib/gcc/arm-none-eabi/4.8.3/include -g -Os -DDEBUG -fno-builtin -ffreestanding -nostdinc -Wall -mlittle-endian -mthumb -mcpu=cortex-m4 -mthumb-interwork -mfloat-abi=soft -fno-builtin

#LDFLAGS=-g -mlittle-endian -mthumb -mcpu=cortex-m4 -mthumb-interwork -mfloat-abi=hard -mfpu=fpv4-sp-d16 -fno-builtin
LDFLAGS=-g -T./stm32_flash.ld -mlittle-endian -mthumb -mcpu=cortex-m4 -mthumb-interwork -fno-builtin -nostartfiles

LDFLAGS_USR=-g -r $(OBJ)/usr/lib/usrlib.o

all: myCore

myCore: os version stm32f407 stm32 $(DTARGETS) usr.bin.o
	$(CC) $(LDFLAGS) -o $@ $(OBJ)/os/init_sect.o \
				$(OBJ)/version.c \
				$(OBJ)/os/os.o \
				$(OBJ)/arch/stm32/stm32f407/stm32f407.o \
				$(OBJ)/arch/stm32/stm32.o \
				$(DOBJS) \
				$(OBJ)/os/end_sect.o \
				$(OBJ)/usr/usr.bin.o


#				$(OBJ)/stddrv/stddrv.o
#				$(OBJ)/arch/stm32/usb/usb_drv.o
#				$(OBJ)/drivers/usb_serial/usb_serial_drv.o
#				$(OBJ)/usr/usr.drv.o
#				$(OBJ)/arch/stm32/stm32f407/hr_timer/hr_timer.o

version:
	echo "const char *ver=\"$(BOARD)-$(GIT_VERSION)\";" > $(OBJ)/version.c


usr.bin.o: $(OBJ)/usr/sys_cmd/sys_cmd.o $(OBJ)/cec_a1_ctrl/cec_a1_ctrl.o
	$(LD) -o $(OBJ)/usr/usr.bin.o $(LDFLAGS_USR) $^
	$(OBJCOPY) --prefix-symbols=__usr_ $(OBJ)/usr/usr.bin.o

#usr.bin.o: $(OBJ)/usr/sys_cmd/sys_cmd.o $(OBJ)/usr/blinky/blinky.o $(OBJ)/cec_a1_mon/cec_a1_mon.o
#	$(LD) -o $(OBJ)/usr/usr.bin.o $(LDFLAGS_USR) $^
#	$(OBJCOPY) --prefix-symbols=__usr_ $(OBJ)/usr/usr.bin.o

#usr.bin.o: $(OBJ)/usr/sys_cmd/sys_cmd.o $(OBJ)/usr/blinky/blinky.o
#	$(LD) -o $(OBJ)/usr/usr.bin.o $(LDFLAGS_USR) $^
#	$(OBJCOPY) --prefix-symbols=__usr_ $(OBJ)/usr/usr.bin.o

$(OBJ)/usr/lib/usrlib.o: $(OBJ)/usr/lib $(OBJ)/usr/lib/arm/syscall_lib.o
	CFLAGS="$(CFLAGS_USR)" TOP=$(TOP) OBJ=$(OBJ) $(MAKE) -C $(OBJ)/usr/lib

$(OBJ)/usr/lib: $(TOP)/usr/lib
	mkdir -p $@
	lndir $< $@

$(OBJ)/usr/lib/arm/syscall_lib.o: $(TOP)/usr/lib
	mkdir -p $(OBJ)/usr/lib/arm
	lndir $< $(OBJ)/usr/lib
	CFLAGS="$(CFLAGS_USR)" TOP=$(TOP) OBJ=$(OBJ) $(MAKE) -C $(OBJ)/usr/lib/arm


$(OBJ)/usr/sys_cmd/sys_cmd.o: $(OBJ)/usr/lib/usrlib.o $(OBJ)/usr/sys_cmd
	CFLAGS="$(CFLAGS_USR)" TOP="$(TOP)" OBJ="$(OBJ)" LDFLAGS="$(LDFLAGS_USR)" $(MAKE) -C $(OBJ)/usr/sys_cmd

$(OBJ)/usr/sys_cmd: $(TOP)/usr/sys_cmd
	mkdir -p $@
	lndir $< $@


$(OBJ)/usr/blinky/blinky.o: $(OBJ)/usr/lib/usrlib.o $(OBJ)/usr/blinky
	CFLAGS="$(CFLAGS_USR)" TOP=$(TOP) OBJ=$(OBJ) $(MAKE) -C $(OBJ)/usr/blinky

$(OBJ)/usr/blinky: $(TOP)/usr/blinky
	mkdir -p $@
	lndir $< $@

$(OBJ)/cec_a1_mon/cec_a1_mon.o: $(OBJ)/usr/lib/usrlib.o $(OBJ)/cec_a1_mon
	CFLAGS="$(CFLAGS_USR)" TOP=$(TOP) OBJ=$(OBJ) $(MAKE) -C $(OBJ)/cec_a1_mon

$(OBJ)/cec_a1_mon: $(TOP)/cec_a1_mon
	mkdir -p $@
	lndir $< $@

$(OBJ)/cec_a1_ctrl/cec_a1_ctrl.o: $(OBJ)/usr/lib/usrlib.o $(OBJ)/cec_a1_ctrl
	CFLAGS="$(CFLAGS_USR)" CFLAGS_DRV="$(CFLAGS)" TOP=$(TOP) OBJ=$(OBJ) $(MAKE) -C $(OBJ)/cec_a1_ctrl

$(OBJ)/cec_a1_ctrl: $(TOP)/cec_a1_ctrl
	mkdir -p $@
	lndir $< $@


usr_drv: $(OBJ)/cec_a1_ctrl/cec_a1_ctrl.o
	$(LD) -r -o $(OBJ)/usr/usr.drv.o $(OBJ)/cec_a1_ctrl/cec_a1_drivers.o
	mkdir -p $(OBJ)/drv_obj
	cp $(OBJ)/usr/usr.drv.o  $(OBJ)/drv_obj/$@.o

#cec_a1_ctrl: $(OBJ)/cec_a1_ctrl
#	CFLAGS="$(CFLAGS_USR)" CFLAGS_DRV="$(CFLAGS)" \
#	LDFLAGS="$(LDFLAGS_USR)" LDFLAGS_DRV="-r" \
#	TOP=$(TOP) OBJ=$(OBJ) $(MAKE) -C $<

#$(OBJ)/cec_a1_ctrl: $(TOP)/cec_a1_ctrl
#	mkdir -p $@
#	lndir $< $@



stddrv: $(OBJ)/stddrv
	CFLAGS="$(CFLAGS)" TOP=$(TOP) OBJ=$(OBJ) $(MAKE) -C $<
	mkdir -p $(OBJ)/drv_obj
	cp  $(OBJ)/stddrv/stddrv.o $(OBJ)/drv_obj/$@.o

$(OBJ)/stddrv: $(TOP)/stddrv
	mkdir -p $@
	lndir $< $@

os: $(OBJ)/os
	CFLAGS="$(CFLAGS)" TOP=$(TOP) OBJ=$(OBJ) $(MAKE) -C $<

$(OBJ)/os: $(TOP)/os
	mkdir -p $@
	lndir $< $@

usb_serial_drv: $(OBJ)/drivers/usb_serial
	CFLAGS="$(CFLAGS)" TOP=$(TOP) OBJ=$(OBJ) $(MAKE) -C $<
	mkdir -p $(OBJ)/drv_obj
	cp $(OBJ)/drivers/usb_serial/usb_serial_drv.o  $(OBJ)/drv_obj/$@.o

$(OBJ)/drivers/usb_serial: $(TOP)/drivers/usb_serial
	mkdir -p $@
	lndir $< $@

stmusb: $(OBJ)/stmusb
	CFLAGS="$(CFLAGS)" TOP=$(TOP) OBJ=$(OBJ) $(MAKE) -C $<

$(OBJ)/stmusb: $(TOP)/stmusb
	mkdir -p $@
	lndir $< $@

stm32: $(OBJ)/arch/stm32
	CFLAGS="$(CFLAGS)" TOP=$(TOP) OBJ=$(OBJ) $(MAKE) -C $<

$(OBJ)/arch/stm32: $(TOP)/arch/stm32
	mkdir -p $@
	lndir $< $@

usb_core_drv: $(OBJ)/arch/stm32/usb
	CFLAGS="$(CFLAGS)" TOP=$(TOP) OBJ=$(OBJ) $(MAKE) -C $<
	mkdir -p $(OBJ)/drv_obj
	cp  $(OBJ)/arch/stm32/usb/usb_drv.o $(OBJ)/drv_obj/$@.o

$(OBJ)/arch/stm32/usb: $(TOP)/arch/stm32/usb
	mkdir -p $@
	lndir $< $@


stm32f407: stm32 stm32f407do

stm32f407do: $(OBJ)/arch/stm32/stm32f407
	CFLAGS="$(CFLAGS)" TOP=$(TOP) OBJ=$(OBJ) $(MAKE) -C $<

$(OBJ)/arch/stm32/stm32f407: $(TOP)/arch/stm32/stm32f407
	mkdir -p $@
	lndir $< $@

hr_timer: stm32f407 hr_timer_build
	mkdir -p $(OBJ)/drv_obj
	cp  $(OBJ)/arch/stm32/stm32f407/hr_timer/hr_timer.o $(OBJ)/drv_obj/$@.o

hr_timer_build: $(OBJ)/arch/stm32/stm32f407/hr_timer
	CFLAGS="$(CFLAGS)" TOP=$(TOP) OBJ=$(OBJ) $(MAKE) -C $<

$(OBJ)/arch/stm32/stm32f407/hr_timer: $(TOP)/arch/stm32/stm32f407/hr_timer
	mkdir -p $@
	lndir $< $@

clean:
	rm -rf $(OBJECTS) myCore
	rm -rf tmp
	rm -rf obj
