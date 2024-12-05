
all: $(OBJ)/lib $(OBJ)/lib/CortexA53.o

$(OBJ)/lib:
	mkdir $(OBJ)/lib

$(OBJ)/lib/CortexA53.o: CortexA53.o
	cp $^ $@

#CortexA53.o: traps.o string.o ctype.o system_init.o switch.o mpu.o irq.o ffs.o support_funcs.o jz_mm.o mm_tlb.o irq.o c_start.o
CortexA53.o: startup_A53.o string.o ctype.o system_init.o ffs.o switch.o support_funcs.o A53_mm.o mm_tlb.o
	$(CC) $(LDFLAGS) -r -nostdlib  -o $@ $^

%.o: %.S
	$(CC) $(CFLAGS) -c -o $@ $<

