

all: $(OBJ)/lib	$(OBJ)/lib/jz.o

$(OBJ)/lib:
	mkdir $@

$(OBJ)/lib/jz.o: jz.o
	cp $^ $@

#jz.o: setup.o jz4750d.o jz_serial.o jz4750_lcd.o jz_nand.o clocks.o
jz.o: setup.o jz4750d.o jz_tcu.o jz_serial.o jz4750_lcd.o jz4750_tve.o jz_nand.o jz_wdt.o
	$(CC) $(LDFLAGS) -r -nostdlib -o $@ $^

%.o: %.S
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<
