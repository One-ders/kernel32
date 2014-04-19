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


#define offsetof(st, m) __builtin_offsetof(st, m)

#define container_of(ptr, type, member) ({ \
		const typeof( ((type *)0)->member ) *__mptr = (ptr); \
		(type *)( (char *)__mptr - offsetof(type,member) );})


#ifdef DEBUG
#define DLEV_SCHED 4
extern int dbglev;
extern int sys_printf(const char *format, ...);
#define DEBUGP(lev,a ...) { if (dbglev>lev) sys_printf(a);}
#else
#define DEBUGP(lev,a ...)
#endif

#define ASSERT(a) { if (!(a)) {io_setpolled(1); sys_printf("assert stuck\n");} while (!(a)) ; }

extern unsigned int sys_irqs;
extern struct task *volatile ready[5];
extern struct task *volatile ready_last[5];

struct blocker {
	struct blocker *next;
	struct blocker *next2;
	unsigned int events;
	unsigned int wakeup_tic;
};

struct blocker_list {
	struct blocker *first;
	struct blocker *last;
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
	char            *name;            /* 0-3 */
	void            *sp;              /* 4-7 */

	struct task     *next;            /* 8-11 */   /* link of task of same state */
	struct task     *next2;           /* 12-15 */  /* all tasks chain */
	int             state;            /* 16-19 */
	int             prio_flags;       /* 20-23 */
	void            *bp;              /* 24-27 */ /* communication buffer for sleeping context */
	int             bsize;            /* 28-31 */ /* size of the buffer */
	void          	*estack;	  /* 32-35 */
	struct user_fd  *fd_list;         /* 36-39 */ /* open driver list */
	unsigned int    active_tics;      /* 36-39 */
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


extern struct task * volatile current;
extern volatile unsigned int tq_tic;

void init_sys(void);
void start_sys(void);
void *getSlab_256(void);

/* interface towards arch functions */
int init_memory_protection(void);
int unmap_stack_memory(unsigned long int addr);
int map_stack_page(unsigned long int addr,unsigned int size);
int map_tmp_stack_page(unsigned long int addr,unsigned int size);
int map_next_stack_page(unsigned long int new_addr, unsigned long int old_addr);
int unmap_tmp_stack_page(void);
int activate_memory_protection(void);

void init_switcher(void);
void switch_on_return(void);
void switch_now(void);



int thread_create(void *fnc, void *val, unsigned int val_size, int prio, char *name);
int sleep(unsigned int ms);
int block_task(char *name);
int unblock_task(char *name);
int setprio_task(char *name,int prio);

void *sys_sleep(unsigned int ms);
void *sys_sleepon(struct blocker *so, void *bp, int bsize, unsigned int *ms_sleep);
void *sys_wakeup(struct blocker *so, void *bp, int bsize);

void *sys_sleepon_update_list(struct blocker *b, struct blocker_list *blocker_list);
void *sys_wakeup_from_list(struct blocker_list *blocker_list);

int sleepable(void);

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

struct device_handle {
	unsigned int user_data1;
};

typedef int (*DRV_CBH)(struct device_handle *, int event, void *user_ref);

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
