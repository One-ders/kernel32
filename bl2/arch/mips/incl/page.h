
#define PAGE_SIZE 4096
#define PAGE_MASK (PAGE_SIZE-1)
#define PAGE_SHIFT 12
#define PAGE_ALIGN(a) (a&~PAGE_MASK)
