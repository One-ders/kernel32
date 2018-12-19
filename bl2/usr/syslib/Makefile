

USR_LIB=$(OBJ)/usr/syslib

OFILES:=c_start.o sys_env.o usr_io.o  errno.o

OBJECTS:=$(addprefix $(USR_LIB)/,$(OFILES))

all: $(USR_LIB) $(USR_LIB)/usrlib.o

$(USR_LIB):
	mkdir $@

$(USR_LIB)/usrlib.o: $(OBJECTS)
	$(LD) -r -o $@ $^ $(USR_LIB)/syscall_lib.o

usrlib.o: $(OBJECTS)

clean:
	rm -rf *.o
