

all: $(OBJ)/usr/ls $(OBJ)/usr/ls/ls

$(OBJ)/usr/ls:
	mkdir $@

$(OBJ)/usr/ls/ls: $(OBJ)/usr/ls/ls.o
	$(LD) -o $@ $(LDFLAGS) $^ $(LDLIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.S
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf ls *.o



test: ls.o
	gcc -o $@ $^
