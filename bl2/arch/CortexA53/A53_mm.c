
void put_page(void *p) {
	return;
}

void *get_page(void) {
#if 0
        int i;
        for(i=0;i<(sizeof(free_page_map)/sizeof(unsigned long int));i++) {
                if (free_page_map[i]) {
                        int j=ffsl(free_page_map[i]);
                        if (!j) return 0;
//                      sys_printf("found free page att index %x, bit %x\n",
//                                      i,j);
                        j--;
                        free_page_map[i]&=~(1<<j);
//                      sys_printf("return page %x\n",(0x80000000 + (i*32*4096)
+ (j*4096)));
                        return (void *)(0x80000000 + (i*32*4096) + (j*4096));
                }
        }
#endif
        return 0;
}

int flush_tlb(void) {
	return 0;
}

int count_free_pages(void) {
	return 0;
}

void *get_pages(unsigned int order) {
	return 0;
}

int dump_tlb(void) {
	return 0;
}

