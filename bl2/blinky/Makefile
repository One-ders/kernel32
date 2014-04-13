
AS=arm-none-eabi-as
CC=arm-none-eabi-gcc
#CFLAGS=-g -O2 -DDEBUG -DDRIVERSUPPORT -DUNECESS -Wall -mlittle-endian -mthumb -mcpu=cortex-m4 -mthumb-interwork -mfloat-abi=hard -mfpu=fpv4-sp-d16 -fno-builtin -I../incl
CFLAGS=-g -O2 -DDEBUG -DDRIVERSUPPORT -DUNECESS -Wall -mlittle-endian -mthumb -mcpu=cortex-m4 -mthumb-interwork -mfloat-abi=soft -fno-builtin -I../incl
LDFLAGS=-g -T../incl/stm32_flash.ld -mlittle-endian -mthumb -mcpu=cortex-m4 -mthumb-interwork -fno-builtin


all: blinky

blinky: main.o
	$(CC) $(LDFLAGS) -o $@ ../lib/init_sect.o ../lib/os.o ../lib/bsp.o ../lib/stm32.o $^ ../lib/led_drv.o ../lib/usb_serial_drv.o ../stmusb/stm_usb_drv.o ../lib/end_sect.o

main.o:

clean:
	rm -rf *.o blinky
