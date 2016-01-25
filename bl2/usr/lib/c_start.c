#include <string.h>
#include <sys.h>

extern unsigned long int __bss_start__;
extern unsigned long int __bss_end__;
extern int main(int argc, char **argv);

void c_start(void) {
	unsigned long int size=((unsigned long int)&__bss_end__)-
				((unsigned long int)&__bss_start__);
	sbrk(size);
	memset(&__bss_start__,0,size);

//	main(1,((void *)0x3f000));
	main(0x7ffffff8,8);
	while(1);
}
