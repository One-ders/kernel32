

#irq.c  jz4740.c  jz4740.h  jz_serial.c  jz_lcd.c traps.S

all: $(OBJ)/lib	$(OBJ)/lib/jz.o

$(OBJ)/lib:
	mkdir $@

$(OBJ)/lib/jz.o: jz.o
	cp $^ $@

jz.o: jz4740.o jz_serial.o jz_tic.o jz_lcd.o nand_funcs.o clocks.o jz_nand.o
	$(CC) $(LDFLAGS) -r -nostdlib -o $@ $^

%.o: %.S
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<
