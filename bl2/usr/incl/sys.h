
typedef unsigned int fd_set;

#define FD_SET(a,b)     ((*(b))|=(1<<a))
#define FD_CLR(a,b)     ((*(b))&=~(1<<a))
#define FD_ISSET(a,b)   ((*(b))&(1<<a))
#define FD_ZERO(a)      ((*(b))=0)


#define MAX_PRIO	4

int thread_create(void *fnc, void *val, unsigned int val_size,
                int prio, char *name);
int sleep(unsigned int ms);
int block_task(char *name);
int unblock_task(char *name);
int setprio_task(char *name,int prio);
int set_debug_level(unsigned int dbglev);
int _reboot_(unsigned int cookie);

int io_open(const char *drvname);
int io_read(int fd, void *buf, int size);
int io_write(int fd, const void *buf, int size);
int io_control(int fd, int cmd, void *d, int sz);
int io_close(int fd);
int io_select(int nfds, fd_set *rfds, fd_set *wfds, fd_set *stfds, unsigned int *tout);

#define RD_CHAR 1
#define WR_CHAR 2
#define IO_POLL 3
#define IO_CALL 4
#define IO_NOCALL 5
#define WR_POLLED_MODE 6
#define WR_GET_RESULT 7
#define F_SETFL 8
#define READDIR 9
#define DYNOPEN 10

#define O_NONBLOCK 1

#define EV_READ  1
#define EV_WRITE 2
#define EV_STATE 4


