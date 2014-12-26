

all: cec_a1_mon.o

cec_a1_mon.o: main.o asynchio.o cec.o a1.o
	$(CC) -r -nostdlib -o $@ $^

main.o: main.c
	$(CC) $(CFLAGS) -c -o $@ $<

asynchio.o: asynchio.c asynchio.h
	$(CC) $(CFLAGS) -c -o $@ $<

#cec_drv.o: cec_drv.c cec_drv.h

#a1_drv.o: a1_drv.c a1_drv.h

cec.o: cec.c cec.h
	$(CC) $(CFLAGS) -c -o $@ $<

a1.o: a1.c a1.h
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf *.o  cec_a1_mon
