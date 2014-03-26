
AS=arm-none-eabi-as
CC=arm-none-eabi-gcc
CFLAGS=-g -O2 -DDEBUG -DDRIVERSUPPORT -DUNECESS -Wall -mlittle-endian -mthumb -mcpu=cortex-m4 -mthumb-interwork -mfloat-abi=hard -mfpu=fpv4-sp-d16 -fno-builtin -I../incl
LDFLAGS=-g -T../incl/stm32_flash.ld -mlittle-endian -mthumb -mcpu=cortex-m4 -mthumb-interwork -mfloat-abi=hard -mfpu=fpv4-sp-d16 -fno-builtin


all: cec_a1_ctrl

cec_a1_ctrl: main.o pulse_eight.o cec.o cec_drv.o
	$(CC) $(LDFLAGS) -o $@ ../lib/init_sect.o ../lib/led_drv.o ../lib/hr_timer.o ../lib/os.o ../lib/bsp.o ../lib/usb_serial_drv.o ../stmusb/stm_usb_drv.o $^ ../lib/end_sect.o 


main.o: main.c

cec_drv.o: cec_drv.c cec_drv.h

pulse_eight.o: pulse_eight.c asynchio.h

cec.o: cec.c cec.h

clean: 
	rm -rf *.o  cec_a1_ctrl

