
AS=arm-none-eabi-as
CC=arm-none-eabi-gcc
CFLAGS=-g -O2 -DDEBUG -DDRIVERSUPPORT -DUNECESS -Wall -mlittle-endian -mthumb -mcpu=cortex-m4 -mthumb-interwork -mfloat-abi=hard -mfpu=fpv4-sp-d16 -fno-builtin -I../incl
LDFLAGS=-g -T../incl/stm32_flash.ld -mlittle-endian -mthumb -mcpu=cortex-m4 -mthumb-interwork -mfloat-abi=hard -mfpu=fpv4-sp-d16 -fno-builtin


all: blinky

blinky: main.o led_drv.o usb_serial_drv.o
	$(CC) $(LDFLAGS) -o $@ ../lib/init_sect.o ../lib/os.o ../lib/bsp.o $^ ../stmusb/stm_usb_drv.o ../lib/end_sect.o 

main.o:

led_drv.o: led_drv.c led_drv.h

usb_serial_drv.o: usb_serial_drv.c
	$(CC) $(CFLAGS) -I../stmusb/ -c -o $@ $^

clean: 
	rm -rf *.o blinky
