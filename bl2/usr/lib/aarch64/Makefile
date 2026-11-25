
USR_LIB=$(OBJ)/usr/lib

all: $(USR_LIB)/syscall_lib.o

$(USR_LIB)/syscall_lib.o: $(USR_LIB)/aarch64/syscall_lib.o
	cp $^ $@

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.S
	$(CC) $(CFLAGS) -c -o $@ $<


clean:
	rm -rf *.o
