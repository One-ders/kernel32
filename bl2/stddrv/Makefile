
AS=arm-none-eabi-as
CC=arm-none-eabi-gcc
#CFLAGS=-g -O2 -DDEBUG -DDRIVERSUPPORT -DUNECESS -Wall -mlittle-endian -mthumb -mcpu=cortex-m4 -mthumb-interwork -mfloat-abi=hard -mfpu=fpv4-sp-d16 -fno-builtin -I../incl
CFLAGS=-g -O2 -DDEBUG -DDRIVERSUPPORT -DUNECESS -Wall -mlittle-endian -mthumb -mcpu=cortex-m4 -mthumb-interwork -mfloat-abi=soft -fno-builtin -I../incl
LDFLAGS=-g -mlittle-endian -mthumb -mcpu=cortex-m4 -mthumb-interwork -fno-builtin


all: ../incl ../incl/gpio_drv.h ../lib ../lib/bsp.o

../incl:
	mkdir ../incl

../incl/gpio_drv.h: gpio_drv.h
	cp $^ $@

../lib:
	mkdir ../lib

../lib/bsp.o: bsp.o
	cp $^ $@

#
#bsp.o:	startup_stm32f4xx.o system_stm32f4xx.o usart_drv.o stterm_drv.o

bsp.o:	startup_stm32f4xx.o usart_drv.o gpio_drv.o
	$(CC) $(LDFLAGS) -r -nostdlib -o $@ $^

usart_drv.o: usart_drv.c usart_drv.h ../incl/sys.h

stterm_drv.o: stterm_drv.c ../incl/sys.h

gpio_drv.o: gpio_drv.c gpio_drv.h


clean:
	rm -rf *.o *.a
