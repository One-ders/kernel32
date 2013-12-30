

AS=arm-none-eabi-as
CC=arm-none-eabi-gcc
CFLAGS=-g -O2 -Wall -mlittle-endian -mthumb -mcpu=cortex-m4 -mthumb-interwork -mfloat-abi=hard -mfpu=fpv4-sp-d16
LDFLAGS=-g -Tstm32_flash.ld -mlittle-endian -mthumb -mcpu=cortex-m4 -mthumb-interwork -mfloat-abi=hard -mfpu=fpv4-sp-d16

all: blinky

blinky: main.o startup_stm32f4xx.o system_stm32f4xx.o
	$(CC)  $(LDFLAGS) -o $@ $^

clean:
	rm -rf *.o blinky
