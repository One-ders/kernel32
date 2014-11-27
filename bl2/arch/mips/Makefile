
all: $(OBJ)/lib $(OBJ)/lib/mips.o

$(OBJ)/lib:
	mkdir $(OBJ)/lib

$(OBJ)/lib/mips.o: mips.o
	cp $^ $@

mips.o: string.o ctype.o system_init.o switch.o mpu.o ffs.o
	$(CC) $(LDFLAGS) -r -nostdlib  -o $@ $^

%.o: %.S
	$(CC) $(CFLAGS) -c -o $@ $<

