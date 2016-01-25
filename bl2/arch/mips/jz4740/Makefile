

#irq.c  jz4740.c  jz4740.h  jz_serial.c  traps.S

all: $(OBJ)/lib	$(OBJ)/lib/jz.o

$(OBJ)/lib:
	mkdir $@

$(OBJ)/lib/jz.o: jz.o
	cp $^ $@

jz.o: traps.o jz4740.o irq.o jz_mm.o jz_serial.o nand_funcs.o clocks.o
	$(CC) $(LDFLAGS) -r -nostdlib -o $@ $^

%.o: %.S
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<
