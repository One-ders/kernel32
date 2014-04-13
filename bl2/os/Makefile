
AS=arm-none-eabi-as
CC=arm-none-eabi-gcc
#CFLAGS=-g -O2 -DDEBUG -Wall -mlittle-endian -mthumb -mcpu=cortex-m4 -mthumb-interwork -mfloat-abi=hard -mfpu=fpv4-sp-d16 -fno-builtin -I../incl
CFLAGS=-g -O2 -DDEBUG -Wall -mlittle-endian -mthumb -mcpu=cortex-m4 -mthumb-interwork -mfloat-abi=soft -fno-builtin -I../incl
LDFLAGS=-g -mlittle-endian -mthumb -mcpu=cortex-m4 -mthumb-interwork -fno-builtin


all: ../incl ../incl/sys.h ../incl/io.h ../lib ../lib/os.o ../lib/init_sect.o ../lib/end_sect.o

#../lib/syslib.a: os.o init_sect.o end_sect.o
#	ar -cr $@ $^
#

../incl:
	mkdir ../incl

../incl/sys.h: sys.h
	cp $^ $@

../incl/io.h: io.h
	cp $^ $@

../lib:
	mkdir ../lib

../lib/os.o: os.o
	cp $^ $@

../lib/init_sect.o: init_sect.o
	cp $^ $@

../lib/end_sect.o: end_sect.o
	cp $^ $@

os.o: sys.o io.o sys_cmd.o sys_tasklib.o
	$(CC) $(LDFLAGS) -r -nostdlib  -o $@ $^

sys.o: ../incl/sys.h ../incl/io.h

io.o: ../incl/io.h ../incl/sys.h

sys_cmd.o: sys.h

sys_tasklib.o: sys.h io.h

init_sect.o: sys_init.c
	$(CC) $(CFLAGS) -c -o $@ -DSECTION_BEGIN $^

end_sect.o: sys_init.c
	$(CC) $(CFLAGS) -c -o $@ -DSECTION_END $^

clean:
	rm -rf *.o *.a
