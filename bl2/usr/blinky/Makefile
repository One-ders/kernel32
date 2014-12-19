
USR_CMD=$(OBJ)/usr/blinky

all: $(USR_CMD) $(USR_CMD)/blinky.o

$(USR_CMD):
	mkdir $@

#$(USR_CMD)/blinky: $(USR_CMD)/blinky.o
#	$(LD) $(LDFLAGS) -o $@ $<

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.S
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf blinky *.o
