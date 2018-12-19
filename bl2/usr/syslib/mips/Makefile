
USR_LIB=$(OBJ)/usr/syslib

all: $(USR_LIB) $(USR_LIB)/start.o $(USR_LIB)/syscall_lib.o

$(USR_LIB):
	mkdir $@



$(USR_LIB)/syscall_lib.o: $(USR_LIB)/mips/syscall_lib.o
	cp $^ $@

$(USR_LIB)/start.o: $(USR_LIB)/mips/start.o
	cp $^ $@

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.S
	$(CC) $(CFLAGS) -c -o $@ $<


clean:
	rm -rf *.o
