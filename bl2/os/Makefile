
#AS=arm-none-eabi-as
#CC=arm-none-eabi-gcc
##CFLAGS=-g -O2 -DDEBUG -Wall -mlittle-endian -mthumb -mcpu=cortex-m4 -mthumb-interwork -mfloat-abi=hard -mfpu=fpv4-sp-d16 -fno-builtin -I../incl
#CFLAGS=-g -O2 -DDEBUG -Wall -mlittle-endian -mthumb -mcpu=cortex-m4 -mthumb-interwork -mfloat-abi=soft -fno-builtin -I../incl
#LDFLAGS=-g -mlittle-endian -mthumb -mcpu=cortex-m4 -mthumb-interwork -mfloat-abi=hard -mfpu=fpv4-sp-d16 -fno-builtin


all: $(TOP)/incl $(TOP)/incl/sys.h $(TOP)/incl/io.h $(OBJ)/lib $(OBJ)/lib/os.o $(OBJ)/lib/init_sect.o $(OBJ)/lib/end_sect.o

#../lib/syslib.a: os.o init_sect.o end_sect.o
#	ar -cr $@ $^
#

$(TOP)/incl:
	mkdir $(TOP)/incl

$(TOP)/incl/sys.h: sys.h
	cp $^ $@

$(TOP)/incl/io.h: io.h
	cp $^ $@

$(OBJ)/lib:
	mkdir $(OBJ)/lib

$(OBJ)/lib/os.o: $(OBJ)/os/os.o
	cp $^ $@

$(OBJ)/lib/init_sect.o: $(OBJ)/os/init_sect.o
	cp $^ $@

$(OBJ)/lib/end_sect.o: $(OBJ)/os/end_sect.o
	cp $^ $@

$(OBJ)/os/os.o: $(OBJ)/os/sys.o $(OBJ)/os/io.o
	$(CC) $(LDFLAGS) -r -nostdlib  -o $@ $^

$(OBJ)/os/sys.o: $(TOP)/incl/sys.h $(TOP)/incl/io.h

$(OBJ)/os/io.o: $(TOP)/incl/io.h $(TOP)/incl/sys.h

#$(OBJ)/os/sys_cmd.o: sys.h

$(OBJ)/os/sys_tasklib.o: sys.h io.h

$(OBJ)/os/init_sect.o: sys_init.c
	$(CC) $(CFLAGS) -c -o $@ -DSECTION_BEGIN $^

$(OBJ)/os/end_sect.o: sys_init.c
	$(CC) $(CFLAGS) -c -o $@ -DSECTION_END $^

clean:
	rm -rf *.o *.a
