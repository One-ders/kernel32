#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/select.h>
#include <mycore/sys.h>

extern unsigned long int __bss_start__;
extern unsigned long int __bss_end__;
extern int main(int argc, char **argv);

void c_start(int argc, char **argv) {
	unsigned long int size=((unsigned long int)&__bss_end__)-
				((unsigned long int)&__bss_start__);
#ifdef IN_KERNEL
	main(1,((void *)0x3f000));
#else
	sbrk(size);
	memset(&__bss_start__,0,size);
	main(argc,argv);
#endif
	exit(0);
	while(1);
}
