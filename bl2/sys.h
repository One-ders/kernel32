


#ifdef DEBUG
#define DLEV_SCHED 4
extern int dbglev;
extern int io_printf(const char *format, ...);
#define DEBUGP(lev,a ...) { if (dbglev>lev) io_printf(a);}
#else
#define DEBUGP(lev,a ...)
#endif

struct task;

struct sleep_obj {
	struct task *task_list;
	struct task *task_list_last;
};

extern volatile unsigned int tq_tic;

void init_sys(void);
void *getSlab_256(void);
int sleep(unsigned int ms);
int thread_create(void *fnc, void *val, int stacksz, char *name);
int sleep_on(struct sleep_obj *so, void *buf, int blen);
int wakeup(struct sleep_obj *so, void *dbuf, int dlen);

#ifdef DRIVERSUPPORT

typedef int (*DRV_CBH)(void);

int io_open(const char *name);
int io_read(int fd, char *buf, int size);
int io_write(int fd, const char *buf, int size);
int io_control(int fd, int cmd, void *b, int size);
int io_close(int fd);

/*  driver */
struct driver {
	char *name;
	int (*open)(struct driver *, DRV_CBH drv_cb);
	int (*close)(int driver_instance);
	int (*control)(int driver_instance, int cmd, void *, int len);
	int (*init)(void);
	int (*start)(void);
	struct driver *next;
};

int driver_publish(struct driver *);
int driver_unpublish(struct driver *);
struct driver *driver_lookup(char *name);

#endif

