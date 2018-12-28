

all: $(OBJ)/usr/fb $(OBJ)/usr/fb/fb

$(OBJ)/usr/fb:
	mkdir $@

$(OBJ)/usr/fb/fb: $(OBJ)/usr/fb/fb.o
	$(LD) -o $@ $(LDFLAGS) $^ $(LDLIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.S
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf fb *.o



test: fb.o
	gcc -o $@ $^
