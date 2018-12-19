
#include <sys.h>
#include <string.h>


int bitmap_alloc_first_free(struct bitmap *bm) {
	int i;
	int j=0;
	for(i=0;i<bm->size;i=i+32) {
		j=ffs(~bm->bm[i/32]);
		if (j) {
			j--;
			bm->bm[i/32]|=(1<<j);
			break;
		}
	}
	if (i==bm->size)  return -1;
	return (j+i);
}

void bitmap_dealloc(struct bitmap *bm, int id) {
	bm->bm[id/32]&=~(1<<(id%32));
}
