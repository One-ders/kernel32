
#AS=arm-none-eabi-as
#CC=arm-none-eabi-gcc
##CFLAGS=-g -O2 -DDEBUG -DDRIVERSUPPORT -DUNECESS -Wall -mlittle-endian -mthumb -mcpu=cortex-m4 -mthumb-interwork -mfloat-abi=hard -mfpu=fpv4-sp-d16 -fno-builtin -I../incl
#CFLAGS=-g -O2 -DDEBUG -DDRIVERSUPPORT -DUNECESS -Wall -mlittle-endian -mthumb -mcpu=cortex-m4 -mthumb-interwork -mfloat-abi=soft -fno-builtin -I../../incl
#LDFLAGS=-g -mlittle-endian -mthumb -mcpu=cortex-m4 -mthumb-interwork -fno-builtin


all: $(OBJ)/lib $(OBJ)/lib/stm32.o

$(OBJ)/lib:
	mkdir $@

$(OBJ)/lib/stm32.o: stm32.o
	cp $^ $@

#
#bsp.o:	startup_stm32f4xx.o system_stm32f4xx.o usart_drv.o stterm_drv.o

stm32.o: system_init.o mpu.o switch.o
	$(CC) $(LDFLAGS) -r -nostdlib -o $@ $^

clean:
	rm -rf *.o *.a
