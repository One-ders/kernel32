

all: $(OBJ)/lib $(OBJ)/lib/BCM2837.o


$(OBJ)/lib:
	mkdir $@

$(OBJ)/lib/BCM2837.o: BCM2837.o
	cp $^ $@

BCM2837.o: exc.o uart0.o arch_start.o irq.o sys_timer.o sdhost.o gpio.o
	$(CC) $(LDFLAGS) -r -nostdlib -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.s
	$(CC) $(CFLAGS) -c -o $@ $<
