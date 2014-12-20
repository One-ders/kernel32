
TOP:=$(shell pwd)/../..
OBJ:=$(TOP)/boards/discovery/obj
GCC_BASE:=$(shell dirname `which arm-none-eabi-gcc`)/..

export AS:=arm-none-eabi-as
export CC:=arm-none-eabi-gcc
export LD:=arm-none-eabi-ld
export OBJCOPY:=arm-none-eabi-objcopy
 
CFLAGS=-I. -I$(TOP)/incl -I$(TOP)/boards/discovery -I$(TOP)/arch/stm32 -I$(TOP)/arch/stm32/incl -g -O2 -DDEBUG -Wall -mlittle-endian -mthumb -mcpu=cortex-m4 -mthumb-interwork -mfloat-abi=soft -fno-builtin

CFLAGS_USR:=-I$(TOP)/usr/incl -I$(GCC_BASE)/lib/gcc/arm-none-eabi/4.8.3/include -g -Os -DDEBUG -fno-builtin -ffreestanding -nostdinc -Wall -mlittle-endian -mthumb -mcpu=cortex-m4 -mthumb-interwork -mfloat-abi=soft -fno-builtin

#LDFLAGS=-g -mlittle-endian -mthumb -mcpu=cortex-m4 -mthumb-interwork -mfloat-abi=hard -mfpu=fpv4-sp-d16 -fno-builtin
LDFLAGS=-g -T./stm32_flash.ld -mlittle-endian -mthumb -mcpu=cortex-m4 -mthumb-interwork -fno-builtin -nostartfiles

LDFLAGS_USR=-g -r $(OBJ)/usr/lib/usrlib.o

#OBJECTS=start.o jz_serial.o jz4740.o
#OBJECTS=traps.o nand_funcs.o c_start.o jz_serial.o $(TOP)/lib/lib.o

all: myCore

myCore: os stm32f04 stm32 stddrv usb_serial stmusb hr_timer usr.drv.o usr.bin.o
	$(CC) $(LDFLAGS) -o $@ $(OBJ)/os/init_sect.o \
				$(OBJ)/os/os.o \
				$(OBJ)/arch/stm32/stm32f04/stm32f04.o \
				$(OBJ)/arch/stm32/stm32.o \
				$(OBJ)/stddrv/stddrv.o \
				$(OBJ)/lib/usb_serial_drv.o \
				$(OBJ)/arch/stm32/stm32f04/hr_timer/hr_timer.o \
				$(OBJ)/stmusb/stm_usb_drv.o \
				$(OBJ)/usr/usr.drv.o \
				$(OBJ)/os/end_sect.o \
				$(OBJ)/usr/usr.bin.o

usr.bin.o: $(OBJ)/usr/sys_cmd/sys_cmd.o $(OBJ)/usr/blinky/blinky.o $(OBJ)/cec_a1_ctrl/cec_a1_ctrl.o
	$(LD) -o $(OBJ)/usr/usr.bin.o $(LDFLAGS_USR) $^
	$(OBJCOPY) --prefix-symbols=__usr_ $(OBJ)/usr/usr.bin.o

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
	CFLAGS="$(CFLAGS_USR)" TOP=$(TOP) OBJ=$(OBJ) $(MAKE) -C $(OBJ)/usr/sys_cmd

$(OBJ)/usr/sys_cmd: $(TOP)/usr/sys_cmd
	mkdir -p $@
	lndir $< $@


$(OBJ)/usr/blinky/blinky.o: $(OBJ)/usr/lib/usrlib.o $(OBJ)/usr/blinky
	CFLAGS="$(CFLAGS_USR)" TOP=$(TOP) OBJ=$(OBJ) $(MAKE) -C $(OBJ)/usr/blinky

$(OBJ)/usr/blinky: $(TOP)/usr/blinky
	mkdir -p $@
	lndir $< $@

$(OBJ)/cec_a1_ctrl/cec_a1_ctrl.o: $(OBJ)/usr/lib/usrlib.o $(OBJ)/cec_a1_ctrl
	CFLAGS="$(CFLAGS_USR)" CFLAGS_DRV="$(CFLAGS)" TOP=$(TOP) OBJ=$(OBJ) $(MAKE) -C $(OBJ)/cec_a1_ctrl

$(OBJ)/cec_a1_ctrl: $(TOP)/cec_a1_ctrl
	mkdir -p $@
	lndir $< $@

usr.drv.o: $(OBJ)/cec_a1_ctrl/cec_a1_ctrl.o
	$(LD) -r -o $(OBJ)/usr/usr.drv.o $(OBJ)/cec_a1_ctrl/cec_a1_drivers.o

#cec_a1_ctrl: $(OBJ)/cec_a1_ctrl
#	CFLAGS="$(CFLAGS_USR)" CFLAGS_DRV="$(CFLAGS)" \
#	LDFLAGS="$(LDFLAGS_USR)" LDFLAGS_DRV="-r" \
#	TOP=$(TOP) OBJ=$(OBJ) $(MAKE) -C $<

#$(OBJ)/cec_a1_ctrl: $(TOP)/cec_a1_ctrl
#	mkdir -p $@
#	lndir $< $@
	


stddrv: $(OBJ)/stddrv
	CFLAGS="$(CFLAGS)" TOP=$(TOP) OBJ=$(OBJ) $(MAKE) -C $<

$(OBJ)/stddrv: $(TOP)/stddrv
	mkdir -p $@
	lndir $< $@

os: $(OBJ)/os
	CFLAGS="$(CFLAGS)" TOP=$(TOP) OBJ=$(OBJ) $(MAKE) -C $<

$(OBJ)/os: $(TOP)/os
	mkdir -p $@
	lndir $< $@

usb_serial: $(OBJ)/drivers/usb_serial
	CFLAGS="$(CFLAGS)" TOP=$(TOP) OBJ=$(OBJ) $(MAKE) -C $<

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

stm32f04: stm32 stm32f04do

stm32f04do: $(OBJ)/arch/stm32/stm32f04
	CFLAGS="$(CFLAGS)" TOP=$(TOP) OBJ=$(OBJ) $(MAKE) -C $<

$(OBJ)/arch/stm32/stm32f04: $(TOP)/arch/stm32/stm32f04
	mkdir -p $@
	lndir $< $@

hr_timer: stm32f04 hr_timer_build

hr_timer_build: $(OBJ)/arch/stm32/stm32f04/hr_timer
	CFLAGS="$(CFLAGS)" TOP=$(TOP) OBJ=$(OBJ) $(MAKE) -C $<

$(OBJ)/arch/stm32/stm32f04/hr_timer: $(TOP)/arch/stm32/stm32f04/hr_timer
	mkdir -p $@
	lndir $< $@

clean:
	rm -rf $(OBJECTS) myCore
	rm -rf tmp
	rm -rf obj
