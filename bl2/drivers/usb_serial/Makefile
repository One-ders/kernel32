
DRV_NAME=usb_serial_drv

SYS_INC=$(TOP)/incl
SYS_LIB=$(OBJ)/lib

DEP_INC=-I$(TOP)/stmusb

#AS=arm-none-eabi-as
#CC=arm-none-eabi-gcc
##CFLAGS=-g -O2 -DDEBUG -DDRIVERSUPPORT -DUNECESS -Wall -mlittle-endian -mthumb -mcpu=cortex-m4 -mthumb-interwork -mfloat-abi=hard -mfpu=fpv4-sp-d16 -fno-builtin -I$(SYS_INC) -I$(DEP_INC)
#CFLAGS=-g -O2 -DDEBUG -DDRIVERSUPPORT -DUNECESS -Wall -mlittle-endian -mthumb -mcpu=cortex-m4 -mthumb-interwork -mfloat-abi=soft -fno-builtin -I$(SYS_INC) -I$(DEP_INC)
#LDFLAGS=-g -mlittle-endian -mthumb -mcpu=cortex-m4 -mthumb-interwork -fno-builtin

CFLAGS+=$(DEP_INC)

usb_s_all: $(SYS_INC) $(SYS_INC)/$(DRV_NAME).h $(SYS_LIB) $(SYS_LIB)/$(DRV_NAME).o

$(SYS_INC):
	mkdir $@

$(SYS_LIB):
	mkdir $@

$(SYS_INC)/$(DRV_NAME).h: $(DRV_NAME).h
	cp $^ $@

$(SYS_LIB)/$(DRV_NAME).o: $(DRV_NAME).o
	cp $^ $@

usb_s_clean:
	rm -rf *.o
