
all: $(OBJ)/stddrv $(OBJ)/stddrv/stddrv.o

$(OBJ)/stddrv:
	mkdir -p $@

$(OBJ)/stddrv/stddrv.o: $(OBJ)/stddrv/procps.o $(OBJ)/stddrv/devls.o $(OBJ)/stddrv/kmem.o
	$(CC) $(LDFLAGS) -r -nostdlib  -o $@ $^


%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<
