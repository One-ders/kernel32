
all: $(OBJ)/lib $(OBJ)/lib/mips.o

$(OBJ)/lib:
	mkdir $(OBJ)/lib

$(OBJ)/lib/mips.o: mips.o
	cp $^ $@

mips.o: traps.o string.o ctype.o system_init.o switch.o mpu.o irq.o ffs.o support_funcs.o jz_mm.o mm_tlb.o irq.o c_start.o
	$(CC) $(LDFLAGS) -r -nostdlib  -o $@ $^

%.o: %.S
	$(CC) $(CFLAGS) -c -o $@ $<

