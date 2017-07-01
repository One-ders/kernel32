
AS=arm-none-eabi-as
CC=arm-none-eabi-gcc
#CFLAGS=-g -O2 -DDEBUG -DDRIVERSUPPORT -DUNECESS -Wall -mlittle-endian -mthumb -mcpu=cortex-m4 -mthumb-interwork -mfloat-abi=hard -mfpu=fpv4-sp-d16 -fno-builtin -I../incl
CFLAGS=-g -O2 -DDEBUG -DDRIVERSUPPORT -DUNECESS -Wall -mlittle-endian -mthumb -mcpu=cortex-m4 -mthumb-interwork -mfloat-abi=soft -fno-builtin -I../incl
LDFLAGS=-g -T../incl/stm32/stm32_flash.ld -mlittle-endian -mthumb -mcpu=cortex-m4 -mthumb-interwork -fno-builtin


all: timer_test

timer_test: main.o timer_test.o
	$(CC) $(LDFLAGS) -o $@ ../lib/init_sect.o ../lib/os.o ../lib/bsp.o ../lib/stm32.o $^ ../lib/led_drv.o ../lib/hr_timer.o  ../lib/end_sect.o

timer_test.o:

clean:
	rm -rf *.o timer_test
