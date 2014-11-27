
TOP:=$(shell pwd)/../../..
OBJ:=$(TOP)/arch/boards/discovery/obj

export AS:=arm-none-eabi-as
export CC:=arm-none-eabi-gcc
export LD:=arm-none-eabi-ld
export OBJCOPY:=arm-none-eabi-objcopy
 
CFLAGS=-I. -I$(TOP)/incl -I$(TOP)/arch/stm32 -I$(TOP)/arch/stm32/incl -g -O2 -DDEBUG -Wall -mlittle-endian -mthumb -mcpu=cortex-m4 -mthumb-interwork -mfloat-abi=soft -fno-builtin
#LDFLAGS=-g -mlittle-endian -mthumb -mcpu=cortex-m4 -mthumb-interwork -mfloat-abi=hard -mfpu=fpv4-sp-d16 -fno-builtin
LDFLAGS=-g -T./stm32_flash.ld -mlittle-endian -mthumb -mcpu=cortex-m4 -mthumb-interwork -fno-builtin

#OBJECTS=start.o jz_serial.o jz4740.o
OBJECTS=traps.o nand_funcs.o c_start.o jz_serial.o $(TOP)/lib/lib.o

all: os stddrv hr_timer iwdg_drv led_drv usb_serial stmusb stm32 os.o

os.o: 
	$(CC) $(LDFLAGS) -o $@ $(OBJ)/lib/init_sect.o $(OBJ)/lib/os.o $(OBJ)/lib/bsp.o $(OBJ)/lib/stm32.o $(OBJ)/lib/led_drv.o $(OBJ)/lib/usb_serial_drv.o $(OBJ)/lib/hr_timer.o $(OBJ)/stmusb/stm_usb_drv.o $(OBJ)/lib/end_sect.o

uCore.img: ipl/boot.bin uCore.bin
	mkdir -p tmp
	dd if=/dev/zero of=tmp/fot1 ibs=4096 count=1
	cat ipl/boot.bin tmp/fot1 > tmp/fot2
	dd if=tmp/fot2 of=tmp/fot3 ibs=4096 count=1
	cat tmp/fot3 uCore.bin > uCore.img

uCore.bin: uCore
	

ipl/boot.bin:
	$(MAKE) -C ipl

uCore: $(OBJECTS)
	$(LD) $(LDFLAGS) -o $@ $(OBJECTS)

traps.o: ../../mips/jz4740/traps.S
	$(CC) $(CFLAGS) -c -o $@ $<

c_start.o: ./c_start.c
	$(CC) $(CFLAGS) -c -o $@ $<



#jz4740.o: ../../../mips/jz4740/jz4740.c
#	$(CC) $(CFLAGS) -c -o $@ $<

jz_serial.o: ../../mips/jz4740/jz_serial.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(TOP)/lib/lib.o:
	$(MAKE) -C $(TOP)/lib

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

hr_timer: $(OBJ)/drivers/hr_timer
	CFLAGS="$(CFLAGS)" TOP=$(TOP) OBJ=$(OBJ) $(MAKE) -C $<

$(OBJ)/drivers/hr_timer: $(TOP)/drivers/hr_timer
	mkdir -p $@
	lndir $< $@

iwdg_drv: $(OBJ)/drivers/iwdg_drv
	CFLAGS="$(CFLAGS)" TOP=$(TOP) OBJ=$(OBJ) $(MAKE) -C $<

$(OBJ)/drivers/iwdg_drv: $(TOP)/drivers/iwdg_drv
	mkdir -p $@
	lndir $< $@

led_drv: $(OBJ)/drivers/led_drv
	CFLAGS="$(CFLAGS)" TOP=$(TOP) OBJ=$(OBJ) $(MAKE) -C $<

$(OBJ)/drivers/led_drv: $(TOP)/drivers/led_drv
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

clean:
	rm -rf $(OBJECTS) uCore uCore.bin uCore.img
	rm -rf tmp
	rm -rf obj
