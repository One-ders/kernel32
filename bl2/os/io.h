
typedef unsigned int fd_set;

#define FD_SET(a,b)	((*(b))|=(1<<a))
#define FD_CLR(a,b)	((*(b))&=~(1<<a))
#define FD_ISSET(a,b)	((*(b))&(1<<a))
#define FD_ZERO(a)	((*(b))=0)

void init_io(void);
int io_puts(const char *str);
int io_add_c(const char c);
int io_setpolled(int enabled);
int io_select(int nfds, fd_set *rfds, fd_set *wfds, fd_set *stfds, unsigned int *tout);

int sys_printf(const char *format, ...);

int fprintf(int fd, const char *format, ...);
#define printf(b...) fprintf(0,b)

unsigned long int strtoul(char *str, char *endp, int base);
