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
 * @(#)sys_tasklib.c
 */
#include "stm32f4xx.h"
#include "io.h"
#include "sys.h"

#include <string.h>

/*
 *    Task side library calls
 */

/*
 * Inline assembler helper directive: call SVC with the given immediate
 */
#define svc(code) asm volatile ("svc %[immediate]\n\t"::[immediate] "I" (code))
 
/*
 * Use a normal C function, the compiler will make sure that this is going
 * to be called using the standard C ABI which ends in a correct stack
 * frame for our SVC call
 */

int svc_create_task(void *fnc, void *val, int prio, char *name);
int svc_sleep(unsigned int);
#if 0
int svc_sleep_on(struct sleep_obj *, void *buf, int size);
int svc_wakeup(struct sleep_obj *, void *buf, int size);
#endif

int svc_io_open(const char *);
int svc_io_read(int fd, const char *b, int size);
int svc_io_write(int fd, const char *b, int size);
int svc_io_control(int fd, int cmd, void *b, int s);
int svc_io_close(int fd);
int svc_io_select(struct sel_args *sel_args);
int svc_block_task(char *name);
int svc_unblock_task(char *name);
int svc_setprio_task(char *name, int prio);


__attribute__ ((noinline)) int svc_create_task(void *fnc, void *val, int prio, char *name) {
	register int rc asm("r0");
	svc(SVC_CREATE_TASK);
	return rc;
}


__attribute__ ((noinline)) int svc_kill_self() {
	svc(SVC_KILL_SELF);
	return 0;
}


__attribute__ ((noinline)) int svc_sleep(unsigned int ms) {
	register int rc asm("r0");
	svc(SVC_SLEEP); 
	return rc;
}

#if 0
__attribute__ ((noinline)) int svc_sleep_on(struct sleep_obj *so, void *buf, int bsize) {
	register int rc asm("r0");
	svc(SVC_SLEEP_ON);
	return rc;
}

__attribute__ ((noinline)) int svc_wakeup(struct sleep_obj *so, void *buf, int bsize) {
	register int rc asm("r0");
	svc(SVC_WAKEUP);
	return rc;
}
#endif


__attribute__ ((noinline)) int svc_io_open(const char *name) {
	register int rc asm("r0");
	svc(SVC_IO_OPEN);
	return rc;
}

__attribute__ ((noinline)) int svc_io_read(int fd, const char *b, int size) {
	register int rc asm("r0");
	svc(SVC_IO_READ);
	return rc;
}

__attribute__ ((noinline)) int svc_io_write(int fd, const char *b, int size) {
	register int rc asm("r0");
	svc(SVC_IO_WRITE);
	return rc;
}

__attribute__ ((noinline)) int svc_io_control(int fd, int cmd, void *b, int s) {
	register int rc asm("r0");
	svc(SVC_IO_CONTROL);
	return rc;
}

__attribute__ ((noinline)) int svc_io_close(int fd) {
	register int rc asm("r0");
	svc(SVC_IO_CLOSE);
	return rc;
}

__attribute__ ((noinline)) int svc_io_select(struct sel_args *sel_args) {
	register int rc asm("r0");
	svc(SVC_IO_SELECT);
	return rc;
}

__attribute__ ((noinline)) int svc_block_task(char *name) {
	register int rc asm("r0");
	svc(SVC_BLOCK_TASK);
	return rc;
}

__attribute__ ((noinline)) int svc_unblock_task(char *name) {
	register int rc asm("r0");
	svc(SVC_UNBLOCK_TASK);
	return rc;
}

__attribute__ ((noinline)) int svc_setprio_task(char *name, int prio) {
	register int rc asm("r0");
	svc(SVC_SETPRIO_TASK);
	return rc;
}





int thread_create(void *fnc, void *val, int prio, char *name) {
	return svc_create_task(fnc, val, prio, name);
}


int sleep(unsigned int ms) {
	unsigned int tics=ms/10;
	return svc_sleep(tics);
}

#if 0
int sleep_on(struct sleep_obj *so, void *buf, int bsize) {
	return svc_sleep_on(so,buf,bsize);
}

int wakeup(struct sleep_obj *so, void *dbuf, int dsize) {
	return svc_wakeup(so,dbuf,dsize);
}
#endif

int block_task(char *name) {
	return svc_block_task(name);
}

int unblock_task(char *name) {
	return svc_unblock_task(name);
}

int setprio_task(char *name,int prio) {
	return svc_setprio_task(name,prio);
}



/**********************************************************************/
/*  Driver service calls                                              */

int io_open(const char *drvname) {
	return svc_io_open(drvname);
}

int io_read(int fd, char *buf, int size) {
	return svc_io_read(fd,buf,size);
}

int io_write(int fd, const char *buf, int size) {
	return svc_io_write(fd,buf,size);
}

int io_control(int fd, int cmd, void *d, int sz) {
	return svc_io_control(fd,cmd,d,sz);
}

int io_close(int fd) {
	return svc_io_close(fd);
}

int io_select(int nfds, fd_set *rfds, fd_set *wfds, fd_set *stfds, unsigned int *tout) {
	struct sel_args sel_args;
	sel_args.nfds=nfds;
	sel_args.rfds=rfds;
	sel_args.wfds=wfds;
	sel_args.stfds=stfds;
	sel_args.tout=tout;
//	printf("io_select: nfds=%d, rfds=%x, wfds=%x, stfds=%x, tout=%x\n",
//			nfds,rfds,wfds,stfds,tout);
	return svc_io_select(&sel_args);
}

