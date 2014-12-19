
typedef unsigned long size_t;

void *memset(void *m, int c, size_t count);
int ffs(long int i);
int ffsl(long int i);

char *strcpy(char *dest, const char *src);
char *strncpy(char *dest, const char *src, size_t n);
int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, size_t n);
unsigned long int strtoul(char *str, char *endp, int base);

char *itoa(unsigned int val, char *buf, int bz, int prepend_zero, int prepend_num);
char *xtoa(unsigned int val, char *buf, int bz, int prepend_zero, int prepend_num);

