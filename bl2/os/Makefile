
AS=arm-none-eabi-as
CC=arm-none-eabi-gcc
CFLAGS=-g -O2 -DDEBUG -DDRIVERSUPPORT -DUNECESS -Wall -mlittle-endian -mthumb -mcpu=cortex-m4 -mthumb-interwork -mfloat-abi=hard -mfpu=fpv4-sp-d16 -fno-builtin -I../incl
LDFLAGS=-g -mlittle-endian -mthumb -mcpu=cortex-m4 -mthumb-interwork -mfloat-abi=hard -mfpu=fpv4-sp-d16 -fno-builtin


all: ../lib ../lib/os.o ../lib/init_sect.o ../lib/end_sect.o

#../lib/syslib.a: os.o init_sect.o end_sect.o
#	ar -cr $@ $^
#

../lib:
	mkdir ../lib

../lib/os.o: os.o
	cp $^ $@

../lib/init_sect.o: init_sect.o
	cp $^ $@

../lib/end_sect.o: end_sect.o
	cp $^ $@

os.o: sys.o io.o
	$(CC) $(LDFLAGS) -r -nostdlib  -o $@ $^

sys.o: ../incl/sys.h ../incl/io.h

io.o: ../incl/io.h ../incl/sys.h

init_sect.o: sys_init.c
	$(CC) $(CFLAGS) -c -o $@ -DSECTION_BEGIN $^

end_sect.o: sys_init.c
	$(CC) $(CFLAGS) -c -o $@ -DSECTION_END $^

clean:
	rm -rf *.o *.a
