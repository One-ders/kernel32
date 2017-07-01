
#include <sys.h>
#include <types.h>
#include <malloc.h>


void *malloc(size_t size) {
	int no_pgs=(size+(PAGE_SIZE-1))/PAGE_SIZE;
	int i;
	for(i=0;i<9;i++) {
		if ((1<<i)>=no_pgs) {
			return get_pages(i);
		}
	}
	return 0;

}

void free(void *ptr) {
	if (!ptr) return;
	put_page(ptr);
}
