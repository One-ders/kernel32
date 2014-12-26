/* $FrameWorks: , v1.1 2014/04/07 21:44:00 anders Exp $ */

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

#include "io.h"
#include "sys_arch.h"
#include <config.h>


#define offsetof(st, m) __builtin_offsetof(st, m)

#define container_of(ptr, type, member) ({ \
		const typeof( ((type *)0)->member ) *__mptr = (ptr); \
		(type *)( (char *)__mptr - offsetof(type,member) );})


#ifdef DEBUG
#define DLEV_SCHED 4
extern int dbglev;
extern int sys_printf(const char *format, ...);
#define DEBUGP(lev,a ...) { if (dbglev>lev) { sys_printf("%t: "); sys_printf(a);}}
#else
#define DEBUGP(lev,a ...)
#endif

#define ASSERT(a) { if (!(a)) {io_setpolled(1); sys_printf("%t: assert stuck\n");} while (!(a)) ; }

extern unsigned int sys_irqs;
extern struct task *volatile ready[5];
extern struct task *volatile ready_last[5];
extern struct task *troot;
extern struct task * volatile current;

extern struct driver *drv_root;

struct user_fd {
        struct driver *driver;
        struct device_handle *dev_handle;
        unsigned int flags;
        struct user_fd *next;
};

struct blocker {
	struct blocker *next;
	struct blocker *next2;
	unsigned int ev;
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
#ifdef MMU
	// an array of 1024 page tables
	unsigned long int *pgd;		/* 40-43 */
#endif
	int		sel_data_valid;
	struct sel_data sel_data;
	struct blocker  blocker;
};



#define TASK_STATE_IDLE         0
#define TASK_STATE_RUNNING      1
#define TASK_STATE_READY        2
#define TASK_STATE_TIMER        3
#define TASK_STATE_IO           4
#define TASK_STATE_DEAD         6

#define MAX_PRIO                4
#define GET_PRIO(a)             ((a)->prio_flags&0x3)
#define SET_PRIO(a,b)           ((a)->prio_flags=(b)&0xf)
#define GET_TMARK(a)            ((a)->prio_flags&0x10)
#define SET_TMARK(a)            ((a)->prio_flags|=0x10)
#define CLR_TMARK(a)            ((a)->prio_flags&=0xEF)


extern volatile unsigned int tq_tic;

void start_up(void);
void init_sys(void);
void start_sys(void);
void *getSlab_256(void);
void *get_page(void);
void put_page(void *);

/* interface towards arch functions */
int init_memory_protection(void);
void init_sys_arch(void);
void setup_return_stack(struct task *t, void *stackp,
					unsigned long int fnc,
					unsigned long int ret_fnc,
					void *arg0, 
					void *arg1);
int unmap_stack_memory(unsigned long int addr);
int map_stack_page(unsigned long int addr,unsigned int size);
int map_tmp_stack_page(unsigned long int addr,unsigned int size);
int map_next_stack_page(unsigned long int new_addr, unsigned long int old_addr);
int unmap_tmp_stack_page(void);
int activate_memory_protection(void);
struct task *create_user_context(void);
int load_init(struct task *);

void init_switcher(void);
void switch_on_return(void);
void switch_now(void);

void init_irq(void);
void config_sys_tic(unsigned int ms);
void board_reboot(void);

unsigned int get_svc_number(void *sp);
unsigned long int get_svc_arg(void *sp, int arg_ix);
void set_svc_ret(void *sp, long int val);
unsigned long int get_stacked_pc(struct task *t);


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

struct device_handle {
	unsigned int user_data1;
};

typedef int (*DRV_CBH)(struct device_handle *, int event, void *user_ref);

struct dyn_open_args {
	char *name;
	struct device_handle *dh;
};

#define RD_CHAR 1
#define WR_CHAR	2
#define IO_POLL	3
#define IO_CALL 4
#define IO_NOCALL 5
#define WR_POLLED_MODE 6
#define WR_GET_RESULT 7
#define F_SETFL	8
#define READDIR 9
#define DYNOPEN 10

#define O_NONBLOCK 1

/* Standard return codes from driver read, write control */
#define DRV_OK 		0
#define DRV_ERR 	1
#define DRV_AGAIN 	11
#define DRV_INPROGRESS 	12

struct dent {
	char	name[32];
};

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
#define SVC_SETDEBUG_LEVEL 15
#define SVC_REBOOT	16
#define SVC_GETTIC	17

struct task_create_args {
	void *fnc;
	void *val;
	unsigned int val_size;
	int prio;
	char *name;
};


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
