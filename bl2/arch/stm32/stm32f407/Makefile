
ODIR=$(OBJ)/arch/stm32/stm32f407

OUT=$(ODIR)/stm32f407.o

OBJECTS= \
	$(ODIR)/startup_stm32f4xx.s \
	$(ODIR)/gpio_drv.o \
	$(ODIR)/usart_drv.o \
	$(ODIR)/iwdg_drv.o \
	$(ODIR)/led_drv.o
	
all: $(ODIR) $(OUT)

$(ODIR):
	mkdir -p $@

$(OUT): $(OBJECTS)
	$(CC) $(LDFLAGS) -r -nostdlib  -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.s
	$(CC) $(CFLAGS) -c -o $@ $<

