


#ifdef DEBUG
#define DLEV_SCHED 4
extern int dbglev;
extern int io_printf(const char *format, ...);
#define DEBUGP(lev,a ...) { if (dbglev>lev) io_printf(a);}
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
int sleep(unsigned int ms);
int thread_create(void *fnc, void *val, int prio, char *name);
int sleep_on(struct sleep_obj *so, void *buf, int blen);
int wakeup(struct sleep_obj *so, void *dbuf, int dlen);

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

#ifdef DRIVERSUPPORT

typedef int (*DRV_CBH)(void *user_ref);

int io_open(const char *name);
int io_read(int fd, char *buf, int size);
int io_write(int fd, const char *buf, int size);
int io_control(int fd, int cmd, void *b, int size);
int io_close(int fd);

/*  driver */
struct driver_ops {
	int (*open)(void *driver_instance, DRV_CBH drv_cb, void *usr_ref);
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

#if 0
#define INIT_FUNC(a) void *init_func_##a __attribute__((section(".init_funcs"))) = a
#else

#define ASMLINE(a) #a
#define INIT_FUNC(fnc) asm(".section	.init_funcs,\"a\",%progbits\n\t");\
			asm(ASMLINE(.extern fnc\n\t));\
			asm(ASMLINE(.long fnc\n\t));\
			asm(".section	.text\n\t")
#endif

#endif

