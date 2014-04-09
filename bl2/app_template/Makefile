
AS=arm-none-eabi-as
CC=arm-none-eabi-gcc
CFLAGS=-g -O2 -DDEBUG -DDRIVERSUPPORT -DUNECESS -Wall -mlittle-endian -mthumb -mcpu=cortex-m4 -mthumb-interwork -mfloat-abi=hard -mfpu=fpv4-sp-d16 -fno-builtin -I../incl
LDFLAGS=-g -T../incl/stm32_flash.ld -mlittle-endian -mthumb -mcpu=cortex-m4 -mthumb-interwork -mfloat-abi=hard -mfpu=fpv4-sp-d16 -fno-builtin


all:

//all: app

app: app_part1.o app_drv.o
	$(CC) $(LDFLAGS) -o $@ ../lib/init_sect.o ../lib/os.o ../lib/bsp.o $^ ../lib/end_sect.o

app_part1.o:

app_drv.o: app_drv.c app_drv.h

clean:
	rm -rf *.o app
