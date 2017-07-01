

USR_LIB=$(OBJ)/usr/lib

OBJECTS:= $(USR_LIB)/c_start.o $(USR_LIB)/sys_env.o $(USR_LIB)/usr_io.o $(USR_LIB)/string.o $(USR_LIB)/ctype.o

all: $(USR_LIB) $(USR_LIB)/usrlib.o

$(USR_LIB):
	mkdir $@

$(USR_LIB)/usrlib.o: $(OBJECTS)
	$(LD) -r -o $@ $^ $(USR_LIB)/syscall_lib.o

usrlib.o: $(OBJECTS)

clean:
	rm -rf *.o
