
#define MMU

#define P_GLOBAL 1
#define P_VALID 2
#define P_DIRTY 4
#define P_NOT_CACHEABLE (2<<3)
#define P_CACHEABLE (3<<3)
#define P_SHARED	0x80000000		// only used in memory structures not in tlb
#define P_MAP   	0x40000000		// just address map, free phys mem on release
#define P_RMASK		0x3fffffff

#define MAP_WRITE P_DIRTY
#define MAP_NO_WRITE 0
#define MAP_CACHE P_CACHEABLE
#define MAP_NO_CACHE P_NOT_CACHEABLE
