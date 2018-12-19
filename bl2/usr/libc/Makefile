
CONFIG_FLAGS=--build=mipsel-linux --disable-shared
#CONFIG_FLAGS=--build=mipsel-linux --disable-shared --enable-debug

all:  $(OBJ)/usr/libc/libc.a

$(OBJ)/usr/libc/libc.a: $(OBJ)/usr/config.mak
	make


$(OBJ)/usr/config.mak: $(TOP)/usr/include
	CROSS_COMPILE=$(COMPILER_PREFIX) $(TOP)/usr/libc/musl/configure $(CONFIG_FLAGS)

$(TOP)/usr/include: $(TOP)/usr/libc/musl/configure
	ln -s $(TOP)/usr/libc/musl/include $(TOP)/usr/
	ln -s $(TOP)/usr/os-include/mycore $(TOP)/usr/include/
	ln -s $(TOP)/usr/libc/musl/arch/$(ARCH)/bits $(TOP)/usr/include/
#                         libc/musl/arch/mips/bits

$(TOP)/usr/libc/musl/configure:
	(cd $(TOP)/usr/libc; git clone git://git.musl-libc.org/musl)

clean:
	rm -rf musl
	rm -rf ../include
