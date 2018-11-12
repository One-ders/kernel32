
#include <sys.h>
#include <types.h>
#include <malloc.h>


void *malloc(size_t size) {
	int no_pgs=(size+(PAGE_SIZE-1))/PAGE_SIZE;
	int i;

	sys_printf("malloc: size=%d, no_pgs=%d\n",size,no_pgs);
	for(i=0;i<9;i++) {
		if ((1<<i)>=no_pgs) {
			return get_pages(i);
		}
	}
	sys_printf("malloc: return NULL\n");
	return 0;

}

void free(void *ptr) {
	if (!ptr) return;
	put_page(ptr);
}
