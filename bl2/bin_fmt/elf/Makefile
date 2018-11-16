
OBJS=elf_funcs.o

LIB_NAME=bin_fmt

SYS_INC=$(TOP)/incl
SYS_LIB=$(OBJ)/lib

CFLAGS+=$(DEP_INC)

bin_fmt_all: $(SYS_LIB) $(SYS_LIB)/$(LIB_NAME).o

$(SYS_LIB):
	mkdir $@

$(SYS_LIB)/$(LIB_NAME).o: $(LIB_NAME).o
	cp $^ $@

$(LIB_NAME).o: $(OBJS)
	$(LD) -r -o $@ $^


test: test.o elf_funcs.o
	gcc -o $@ $^
