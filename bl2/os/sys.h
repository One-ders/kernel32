/* $TSOS: , v1.1 2014/04/07 21:44:00 anders Exp $ */

/*
 * Copyright (c) 2014, Anders Franzen.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * @(#)sys.h
 */

#include <mm.h>
//#include "io.h"
#include "sys_arch.h"
#include <config.h>
#include <types.h>
#include <errno.h>


#define offsetof(st, m) __builtin_offsetof(st, m)

#define container_of(ptr, type, member) ({ \
		const typeof( ((type *)0)->member ) *__mptr = (ptr); \
		(type *)( (char *)__mptr - offsetof(type,member) );})


#ifdef DEBUG
#define DSYS_SCHED	1
#define DSYS_MEM	2
#define DSYS_FS		4
#define DSYS_KERN	8
#define DSYS_LOAD	0x10
#define DSYS_TIMER	0x20

#define DLEV_CRIT	1
#define DLEV_INFO	2

extern int trace_sys;
extern int trace_lev;
extern int sys_printf(const char *format, ...);

#define DEBUGP(subsys,lev,a ...) { if ((subsys&trace_sys)&&(trace_lev>lev)) sys_printf(a);}
#else
#define DEBUGP(lev,a ...)
#endif

#define ASSERT(a) { if (!(a)) {io_setpolled(1); sys_printf("%t: assert stuck, %s %d\n", __FILE__, __LINE__);} while (!(a)) ; }

extern unsigned int sys_irqs;
extern struct task *volatile ready[5];
extern struct task *volatile ready_last[5];
extern struct task *troot;
extern struct task * volatile current;

extern struct driver *drv_root;

/*  for select */
typedef unsigned int fd_set;

struct user_fd {
        struct driver *driver;
        struct device_handle *dev_handle;
        unsigned int flags;
        struct user_fd *next;
};

struct blocker {
	struct blocker *next;
	struct blocker *next2;
	unsigned long ev;
	struct device_handle *dh;
	struct driver *driver;
	unsigned int wake;
	unsigned int wakeup_tic;
};

struct blocker_list {
	struct blocker *first;
	struct blocker *last;
	int  (*is_ready)(void);
};

struct sel_args {
	int nfds;
	fd_set *rfds;
	fd_set *wfds;
	fd_set *stfds;
	unsigned int *tout;
};

struct sel_data {
	int nfds;
	fd_set rfds;
	fd_set wfds;
	fd_set stfds;
	unsigned int *tout;
};

struct bitmap {
	unsigned int *bm;
	int size;
};


struct bitmap *create_bitmap(unsigned int *buf, int bm_size);
#define CREATE_BITMAP(_bm_name,size)	\
unsigned int _bm_name##_buf[((size-1)/32)+1];	\
struct bitmap _bm_name##bub = {_bm_name##_buf,size}; \
struct bitmap *_bm_name = &_bm_name##bub;

int bitmap_alloc_first_free(struct bitmap *bm);
void bitmap_dealloc(struct bitmap *bm, int id);

#define FDTAB_SIZE 100

struct address_space {
#ifdef MMU
	// an array of 1024 page tables
	unsigned long int *pgd;		/* 00-03 */
	int		id;		/* 04-07 */
	unsigned long int brk;
	unsigned long int mmap_vaddr;
#endif
	int		ref;
	struct address_space	*parent;
	struct address_space	*next;
	struct address_space	*child;
	struct user_fd		*fd_list;
	struct user_fd		fd_tab[FDTAB_SIZE];
};


struct task {
	char            *name;		/* 0-3 */
	void            *sp;		/* 4-7 */
	int		id;		/* 8-11 */

	struct task     *next;		/* 12-15 */   /* link of task of same state */
	struct task     *next2;		/* 16-19 */  /* all tasks chain */
	int             state;		/* 20-23 */
	int             prio_flags;	/* 24-27 */
	void          	*estack;	/* 28-31 */
	struct user_fd  *fd_list;	/* 32-35 */ /* open driver list */
	unsigned int    active_tics;	/* 36-39 */
	struct address_space *asp;	/* 40-43 */
	int		sel_data_valid;
	struct sel_data sel_data;
	struct blocker  blocker;
	unsigned long	kheap;
};



#define TASK_STATE_IDLE		0
#define TASK_STATE_RUNNING	1
#define TASK_STATE_READY	2
#define TASK_STATE_TIMER	3
#define TASK_STATE_IO		4
#define TASK_STATE_WAIT		5
#define TASK_STATE_DEAD		6

#define MAX_PRIO                4
#define GET_PRIO(a)             ((a)->prio_flags==4?4:(a)->prio_flags&0x3)
#define SET_PRIO(a,b)           ((a)->prio_flags=(b)&0xf)
#define GET_TMARK(a)            ((a)->prio_flags&0x10)
#define SET_TMARK(a)            ((a)->prio_flags|=0x10)
#define CLR_TMARK(a)            ((a)->prio_flags&=0xEF)


extern struct task main_task;
extern volatile unsigned int tq_tic;

void start_up(void);
void init_sys(void);
void start_sys(void);
void *getSlab_256(void);
void *get_page(void);
void *get_pages(unsigned int order);
void put_page(void *);

/* interface towards arch functions */
void init_sys_arch(void);
int setup_return_stack(struct task *t, void *stackp,
					unsigned long int fnc,
					unsigned long int ret_fnc,
					char **args,
					char **env);
int array_size(char **argv);
int args_size(char **argv);
int copy_arguments(char **argv_new, char **argv,
                        char *arg_storage, int nr_args);
struct task *create_user_context(void);
void delete_user_context(struct task *);
int share_process_pages(struct task *to, struct task *from);
int load_init(struct task *);
int load_binary(char *npath);

void init_switcher(void);
void switch_on_return(void);
void switch_now(void);

void init_irq(void);
void config_sys_tic(unsigned int ms);
void board_reboot(void);
void wait_irq(void);

unsigned int get_svc_number(void *sp);
unsigned long int get_svc_arg(void *sp, int arg_ix);
void set_svc_ret(void *sp, long int val);
void set_svc_lret(void *sp, long int val);
unsigned long int get_stacked_pc(struct task *t);
unsigned long int get_usr_pc(struct task *t);

#ifdef MMU
void *sys_sbrk(struct task *t, long int incr);
int sys_brk(struct task *t, void *nbrk);
#endif

void SysTick_Handler(void);
int sys_udelay(unsigned int usec);

/*****************************************************/



void *sys_sleep(unsigned int ms);
void *sys_sleepon(struct blocker *so, unsigned int *ms_sleep);
void *sys_wakeup(struct blocker *so);

void *sys_sleepon_update_list(struct blocker *b, struct blocker_list *blocker_list);
void *sys_wakeup_from_list(struct blocker_list *blocker_list);

int task_sleepable(void);

#define EV_READ  1
#define EV_WRITE 2
#define EV_STATE 4

#define PROT_EXEC       1
#define PROT_READ       2
#define PROT_WRITE      4
#define PROT_NONE       0

#define MAP_SHARED      1
#define MAP_PRIVATE     2
#define MAP_ANONYMOUS   4
#define MAP_FIXED       5
#define MAP_GROWSDOWN   6


struct device_handle {
	unsigned int user_data1;
	struct device_handle *next;
};

typedef int (*DRV_CBH)(struct device_handle *, int event, void *user_ref);

struct dyn_open_args {
	char			*name;
	struct device_handle	*dh;
};

#define RD_CHAR 1
#define WR_CHAR	2
#define IO_POLL	3
#define IO_CALL 4
#define IO_NOCALL 5
#define WR_POLLED_MODE 6
#define WR_GET_RESULT 7
#define READDIR 9
#define DYNOPEN 10
#define IO_LSEEK 11
#define IO_MMAP 12
#define IO_MUNMAP 13

/* Values for the second argument to `fcntl'.  */
#define F_DUPFD         0       /* Duplicate file descriptor.  */
#define F_GETFD         1       /* Get file descriptor flags.  */
#define F_SETFD         2       /* Set file descriptor flags.  */
#define F_GETFL         3       /* Get file status flags.  */
#define F_SETFL         4       /* Set file status flags.  */


#define F_GETLK       5       /* Get record locking info.  */
#define F_SETLK       6       /* Set record locking info (non-blocking).  */
#define F_SETLKW      7

#define F_SETOWN     8
#define F_GETOWN     9


#define F_GETLK64      12      /* Get record locking info.  */
#define F_SETLK64      13      /* Set record locking info (non-blocking).  */
#define F_SETLKW64     14      /* Set record locking info (blocking).  */



#define O_NONBLOCK 0200

/* Standard return codes from driver read, write control */
#define DRV_OK 		0
#define DRV_ERR 	1
#define DRV_AGAIN 	11
#define DRV_INPROGRESS 	12

struct dent {
	char	name[32];
};

#include <sys_svc.h>
#if 0
#define SVC_CREATE_TASK 1
#define SVC_SLEEP       SVC_CREATE_TASK+1
#define SVC_SLEEP_ON    SVC_SLEEP+1
#define SVC_WAKEUP      SVC_SLEEP_ON+1
#define SVC_IO_OPEN     SVC_WAKEUP+1
#define SVC_IO_READ     SVC_IO_OPEN+1
#define SVC_IO_WRITE    SVC_IO_READ+1
#define SVC_IO_CONTROL  SVC_IO_WRITE+1
#define SVC_IO_LSEEK    SVC_IO_CONTROL+1
#define SVC_IO_CLOSE    SVC_IO_LSEEK+1
#define SVC_IO_SELECT   SVC_IO_CLOSE+1
#define SVC_IO_MMAP     SVC_IO_SELECT+1
#define SVC_IO_MUNMAP   SVC_IO_MMAP+1
#define SVC_KILL_SELF   SVC_IO_MUNMAP+1
#define SVC_BLOCK_TASK  SVC_KILL_SELF+1
#define SVC_UNBLOCK_TASK SVC_BLOCK_TASK+1
#define SVC_SETPRIO_TASK SVC_UNBLOCK_TASK+1
#define SVC_SETDEBUG_LEVEL SVC_SETPRIO_TASK+1
#define SVC_REBOOT	SVC_SETDEBUG_LEVEL+1
#define SVC_GETTIC	SVC_REBOOT+1
#define SVC_SBRK	SVC_GETTIC+1
#define SVC_BRK		SVC_SBRK+1
#define SVC_FORK	SVC_BRK+1
#endif

struct task_create_args {
	void *fnc;
	void *val;
	unsigned int val_size;
	int prio;
	char *name;
};

int allocate_task_id(struct task *t);
void dealloc_task_id(int);
int allocate_as_id(void);
void dealloc_as_id(int);
void *alloc_kheap(struct task *t, unsigned int size);
int incr_address_space_users(struct address_space *asp);
int decr_address_space_users(struct address_space *asp);
struct task *lookup_task_for_name(char *task_name);
int count_free_pages();
int count_shared_pages();
unsigned int get_pfn_sh(unsigned int ix);
int free_asp_pages(struct address_space *asp);





/*  driver */

struct driver_ops {
	struct device_handle *(*open)(void *driver_instance, DRV_CBH drv_cb, void *usr_ref);
	int (*close)(struct device_handle *);
	int (*control)(struct device_handle *, int cmd, void *, int len);
	int (*init)(void *driver_instance);
	int (*start)(void *driver_instance);
};

struct driver {
	char *name;
	void *instance;
	struct driver_ops *ops;
	unsigned int stat;
	struct driver *next;
};

int driver_publish(struct driver *);
int driver_unpublish(struct driver *);
struct driver *driver_lookup(char *name);

#define DEVICE_ROOT(root) static struct device_handle *root
#define DEVICE_HANDLE()  struct device_handle __dh
#define DEVICE_H(udata) &udata->__dh
#define DEVICE_UDATA(ustruct, dhptr) container_of(dhptr, ustruct, __dh)

void driver_user_data_init(struct device_handle **root,
			struct device_handle *handlers,
			unsigned int num_handlers);

struct device_handle *driver_user_get_udata(struct device_handle *root);

void driver_user_put_udata(struct device_handle *root,
				struct device_handle *dh);

#if 0
#define INIT_FUNC(a) void *init_func_##a __attribute__((section(".init_funcs"))) = a
#else

#define ASMLINE(a) #a

#ifdef _M_X64
#define INIT_FUNC(fnc) asm(".section	.init_funcs,\"a\",%progbits\n\t");\
			asm(ASMLINE(.align 2\n\t));\
			asm(ASMLINE(.extern fnc\n\t));\
			asm(ASMLINE(.dword fnc\n\t));\
			asm(".section	.text\n\t")
#else
#define INIT_FUNC(fnc) asm(".section	.init_funcs,\"a\",%progbits\n\t");\
			asm(ASMLINE(.extern fnc\n\t));\
			asm(ASMLINE(.long fnc\n\t));\
			asm(".section	.text\n\t")
#endif
#endif
