
USR_LIB=$(OBJ)/usr/lib

all: $(USR_LIB) $(USR_LIB)/syscall_lib.o

$(USR_LIB):
	mkdir $@



$(USR_LIB)/syscall_lib.o: $(USR_LIB)/mips/syscall_lib.o
	cp $^ $@

clean:
	rm -rf *.o
