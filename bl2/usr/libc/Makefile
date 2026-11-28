
###CONFIG_FLAGS=$(MUSL_CONFIG_FLAGS)
CONFIG_FLAGS=--build=mipsel-linux --disable-shared
#CONFIG_FLAGS=--build=mipsel-linux --disable-shared --enable-debug

all:  $(OBJ)/usr/libc/libc.a

$(OBJ)/usr/libc/libc.a: $(OBJ)/usr/config.mak
	make


$(OBJ)/usr/config.mak: $(OBJ)/usr/include
	CROSS_COMPILE=$(COMPILER_PREFIX) $(OBJ)/usr/libc/musl/configure $(CONFIG_FLAGS)

$(OBJ)/usr/include: $(OBJ)/usr/libc/musl/configure
	mkdir -p $(OBJ)/usr/include
	lndir $(OBJ)/usr/libc/musl/include $(OBJ)/usr/include
	mkdir -p $(OBJ)/usr/include/mycore
	lndir $(TOP)/usr/os-include/mycore $(OBJ)/usr/include/mycore
	mkdir $(OBJ)/usr/include/bits
	ln -s $(OBJ)/usr/libc/musl/arch/generic/bits/* $(OBJ)/usr/include/bits/
	cp -f $(OBJ)/usr/libc/musl/arch/$(ARCH)/bits/* $(OBJ)/usr/include/bits/
	patch -p 0 -d $(OBJ)/usr/libc < musl_001.patch
	patch -p 0 -d $(OBJ)/usr/libc < musl_002.patch

$(OBJ)/usr/libc/musl/configure: $(OBJ)/usr/libc
	(cd $(OBJ)/usr/libc;\
         git clone git://git.musl-libc.org/musl;\
         cd musl;\
         git checkout tags/v1.1.22 )

$(OBJ)/usr/libc:
	mkdir -p $(OBJ)/usr/libc
#
#$(OBJ)/usr/config.mak: $(TOP)/usr/include
#	CROSS_COMPILE=$(COMPILER_PREFIX) $(TOP)/usr/libc/musl/configure $(CONFIG_FLAGS)

#$(TOP)/usr/include: $(TOP)/usr/libc/musl/configure
#	ln -s $(TOP)/usr/libc/musl/include $(TOP)/usr/
#	ln -s $(TOP)/usr/os-include/mycore $(TOP)/usr/include/
#	mkdir $(TOP)/usr/include/bits
#	ln -s $(TOP)/usr/libc/musl/arch/generic/bits/* $(TOP)/usr/include/bits/
#	cp -f $(TOP)/usr/libc/musl/arch/$(ARCH)/bits/* $(TOP)/usr/include/bits/
#	patch -p 0 -d $(TOP)/usr/libc < musl_001.patch
#	patch -p 0 -d $(TOP)/usr/libc < musl_002.patch

#$(TOP)/usr/libc/musl/configure:
#	(cd $(TOP)/usr/libc;\
#         git clone git://git.musl-libc.org/musl;\
#         cd musl;\
#         git checkout tags/v1.1.22 )

clean:
	rm -rf musl
	rm -rf ../include
