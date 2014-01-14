

#define RD_CHAR 1
#define WR_CHAR 2
#define WR_POLLED_MODE 3

void init_io(void);
int io_puts(const char *str);
int io_add_c(const char c);
int io_printf(const char *format, ...);

int fprintf(int fd, const char *format, ...);
#define printf(b...) fprintf(0,b)
