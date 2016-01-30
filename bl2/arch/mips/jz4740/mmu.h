
#define P_GLOBAL 1
#define P_VALID 2
#define P_DIRTY 4
#define P_NOT_CACHEABLE (2<<3)
#define P_CACHEABLE (3<<3)

#define MAP_WRITE P_DIRTY
#define MAP_NO_WRITE 0
#define MAP_CACHE P_CACHEABLE
#define MAP_NO_CACHE P_NOT_CACHEABLE
