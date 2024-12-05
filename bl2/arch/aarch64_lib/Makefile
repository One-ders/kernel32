

VPATH=optimized-routines/string/aarch64

OBJS=memchr.o memcmp.o memcpy.o memset.o strcpy.o strchr.o strcmp.o strcpy.o strlen.o strncmp.o strnlen.o strrchr.o

all: strings.o

#cc t.c -Wl,--defsym=custom_malloc=malloc

strings.o: $(OBJS)
	$(CC) $(LDFLAGS) -r -nostdlib -o $@ $^ -Wl,--defsym=strcmp=__strcmp_aarch64 \
	-Wl,--defsym=strncmp=__strncmp_aarch64 \
	-Wl,--defsym=strlen=__strlen_aarch64 \
	-Wl,--defsym=strnlen=__strnlen_aarch64 \
	-Wl,--defsym=strcpy=__strcpy_aarch64 \
	-Wl,--defsym=strchr=__strchr_aarch64 \
	-Wl,--defsym=memset=__memset_aarch64 \
	-Wl,--defsym=memcmp=__memcmp_aarch64 \
	-Wl,--defsym=memmove=__memmove_aarch64 \
	-Wl,--defsym=memcpy=__memcpy_aarch64
