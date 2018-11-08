#include <string.h>
#include <sys.h>

extern unsigned long int __bss_start__;
extern unsigned long int __bss_end__;
extern int main(int argc, char **argv);

void c_start(void) {
	unsigned long int size=((unsigned long int)&__bss_end__)-
				((unsigned long int)&__bss_start__);
#ifdef MMU
	sbrk(size);
	memset(&__bss_start__,0,size);
	main(0x7ffffff8,(void *)8);
#else
	main(1,((void *)0x3f000));
#endif
	while(1);
}
