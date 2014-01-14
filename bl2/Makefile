

AS=arm-none-eabi-as
CC=arm-none-eabi-gcc
CFLAGS=-g -O2 -DDEBUG -DDRIVERSUPPORT -DUNECESS -Wall -mlittle-endian -mthumb -mcpu=cortex-m4 -mthumb-interwork -mfloat-abi=hard -mfpu=fpv4-sp-d16 -fno-builtin
LDFLAGS=-g -Tstm32_flash.ld -mlittle-endian -mthumb -mcpu=cortex-m4 -mthumb-interwork -mfloat-abi=hard -mfpu=fpv4-sp-d16 -fno-builtin

all: blinky

blinky: init_sect.o main.o sys.o io.o startup_stm32f4xx.o system_stm32f4xx.o usart_drv.o stterm_drv.o end_sect.o
	$(CC)  $(LDFLAGS) -o $@ $^

sys.o: sys.h io.h

io.o: io.h sys.h

main.o: sys.h io.h

usart_drv.o: usart_drv.c usart_drv.h

stterm_drv.o: stterm_drv.c

init_sect.o: sys_init.c
	$(CC) $(CFLAGS) -c -o $@ -DSECTION_BEGIN $^

end_sect.o: sys_init.c
	$(CC) $(CFLAGS) -c -o $@ -DSECTION_END $^



clean:
	rm -rf *.o blinky


