

all: cec_a1_ctrl.o cec_a1_drivers.o

cec_a1_ctrl.o: main.o asynchio.o pulse_eight.o cec.o a1.o
	$(CC) -r -nostdlib -o $@ $^

cec_a1_drivers.o: cec_drv.o a1_drv.o
	$(CC) -r -nostdlib -o $@ $^

%_drv.o: %_drv.c
	$(CC) $(CFLAGS_DRV) -c -o $@ $<
	
main.o: main.c
	$(CC) $(CFLAGS) -c -o $@ $<

asynchio.o: asynchio.c asynchio.h
	$(CC) $(CFLAGS) -c -o $@ $<

#cec_drv.o: cec_drv.c cec_drv.h

#a1_drv.o: a1_drv.c a1_drv.h

pulse_eight.o: pulse_eight.c asynchio.h
	$(CC) $(CFLAGS) -c -o $@ $<

cec.o: cec.c cec.h
	$(CC) $(CFLAGS) -c -o $@ $<

a1.o: a1.c a1.h
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf *.o  cec_a1_ctrl
