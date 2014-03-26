#include "io.h"


#ifdef DEBUG
#define DLEV_SCHED 4
extern int dbglev;
extern int sys_printf(const char *format, ...);
#define DEBUGP(lev,a ...) { if (dbglev>lev) sys_printf(a);}
#else
#define DEBUGP(lev,a ...)
#endif

extern int sys_lev;
struct task;

struct sleep_obj {
	char *name;
	struct task *task_list;
	struct task *task_list_last;
};

extern volatile unsigned int tq_tic;

void init_sys(void);
void start_sys(void);
void *getSlab_256(void);


int thread_create(void *fnc, void *val, int prio, char *name);
int sleep(unsigned int ms);
int sleep_on(struct sleep_obj *so, void *buf, int blen);
int wakeup(struct sleep_obj *so, void *dbuf, int dlen);
int block_task(char *name);
int unblock_task(char *name);
int setprio_task(char *name,int prio);

void *sys_sleep(unsigned int ms);
void *sys_sleepon(struct sleep_obj *so, void *bp, int bsize);
void *sys_wakeup(struct sleep_obj *so, void *bp, int bsize);

static inline __attribute__ ((always_inline))
void disable_interrupt(void) {
	asm("cpsid i");
}

static inline __attribute__ ((always_inline))
void enable_interrupt(void) {
	asm("cpsie i");
}


#define EV_READ  1
#define EV_WRITE 2
#define EV_STATE 4
typedef int (*DRV_CBH)(int ufd, int event, void *user_ref);

#define RD_CHAR 1
#define WR_CHAR	2
#define IO_POLL	3
#define IO_CALL 4
#define IO_NOCALL 5
#define WR_POLLED_MODE 6
#define WR_GET_RESULT 7
#define F_SETFL	8

#define O_NONBLOCK 1

/* Standard return codes from driver read, write control */
#define DRV_OK 		0
#define DRV_ERR 	1
#define DRV_AGAIN 	11
#define DRV_INPROGRESS 	12

int io_open(const char *name);
int io_read(int fd, char *buf, int size);
int io_write(int fd, const char *buf, int size);
int io_control(int fd, int cmd, void *b, int size);
int io_close(int fd);
int io_select(int nfds, fd_set *rfds, fd_set *wfds, fd_set *stfds, unsigned int *tout);


#define SVC_CREATE_TASK 1
#define SVC_SLEEP       2
#define SVC_SLEEP_ON    3
#define SVC_WAKEUP      4
#define SVC_IO_OPEN     5
#define SVC_IO_READ     6
#define SVC_IO_WRITE    7
#define SVC_IO_CONTROL  8
#define SVC_IO_CLOSE    9
#define SVC_IO_SELECT   10
#define SVC_KILL_SELF   11
#define SVC_BLOCK_TASK  12
#define SVC_UNBLOCK_TASK 13
#define SVC_SETPRIO_TASK 14

struct sel_args {
	int nfds;
	fd_set *rfds;
	fd_set *wfds;
	fd_set *stfds;
	unsigned int *tout;
};


/*  driver */
struct driver_ops {
	int (*open)(void *driver_instance, DRV_CBH drv_cb, void *usr_ref, int user_fd);
	int (*close)(int driver_fd);
	int (*control)(int driver_fd, int cmd, void *, int len);
	int (*init)(void *driver_instance);
	int (*start)(void *driver_instance);
};

struct driver {
	char *name;
	void *instance;
	struct driver_ops *ops;
	struct driver *next;
};

int driver_publish(struct driver *);
int driver_unpublish(struct driver *);
struct driver *driver_lookup(char *name);

/********* Cmd funcs ************/

struct Env {
        int io_fd;
};

typedef int (*cmdFunc)(int,char **, struct Env *);

struct cmd {
        char *name;
        cmdFunc fnc;
};

struct cmd_node {
	char *name;
	struct cmd *cmds;
	struct cmd_node *next;
};

extern struct cmd_node *root_cmd_node;

int generic_help_fnc(int argc, char **argv, struct Env *env);
int install_cmd_node(struct cmd_node *, struct cmd_node *parent);


#if 0
#define INIT_FUNC(a) void *init_func_##a __attribute__((section(".init_funcs"))) = a
#else

#define ASMLINE(a) #a
#define INIT_FUNC(fnc) asm(".section	.init_funcs,\"a\",%progbits\n\t");\
			asm(ASMLINE(.extern fnc\n\t));\
			asm(ASMLINE(.long fnc\n\t));\
			asm(".section	.text\n\t")
#endif
