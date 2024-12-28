
all: $(OBJ)/lib $(OBJ)/lib/aarch64.o

$(OBJ)/lib:
	mkdir $(OBJ)/lib

$(OBJ)/lib/aarch64.o: aarch64.o
	cp $^ $@

#aarch64.o: traps.o string.o ctype.o system_init.o switch.o mpu.o irq.o ffs.o support_funcs.o jz_mm.o mm_tlb.o irq.o c_start.o
aarch64.o: startup_A53.o string.o ctype.o system_init.o ffs.o switch.o support_funcs.o A53_mm.o mm_tlb.o
	$(CC) $(LDFLAGS) -r -nostdlib  -o $@ $^

%.o: %.S
	$(CC) $(CFLAGS) -c -o $@ $<

