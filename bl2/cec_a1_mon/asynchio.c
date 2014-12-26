/* $CecA1GW: asynchio.c, v1.1 2014/04/07 21:44:00 anders Exp $ */

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
 * @(#)asynchio.c
 */

#include "sys.h"
#include "io.h"
#include "asynchio.h"

#include <string.h>

struct EventInfo {
	EventHandler rdhandler;
	void *rduref;
	EventHandler wrhandler;
	void *wruref;
	EventHandler sthandler;
	void *sturef;
};

static struct EventInfo fds[32];
static struct EventInfo timer;
static int timeout;

static fd_set rfds;
static fd_set wfds;
static fd_set stfds;
static unsigned int nfds;


int register_event(int fd, int event,
			EventHandler handler,
			void *uref) {
	int i;
	struct EventInfo *ei=&fds[fd];
	if ((fd<0)||(fd>31)) {
		return -1;
	}
	if (handler) { /* register */
		if (event&EV_READ) {
			ei->rdhandler=handler;
			ei->rduref=uref;
			FD_SET(fd,&rfds);
			if (fd>nfds) {
				nfds=fd;
			}
		}
		if (event&EV_WRITE) {
			ei->wrhandler=handler;
			ei->wruref=uref;
			FD_SET(fd,&wfds);
			if (fd>nfds) {
				nfds=fd;
			}
		}
		if (event&EV_STATE) {
			ei->sthandler=handler;
			ei->sturef=uref;
			FD_SET(fd,&stfds);
			if (fd>nfds) {
				nfds=fd;
			}
		}
	} else { /* deregister */
		if (event&EV_READ) {
			ei->rdhandler=0;
			FD_CLR(fd,&rfds);
		}
		if (event&EV_WRITE) {
			ei->wrhandler=0;
			FD_CLR(fd,&wfds);
		}
		if (event&EV_STATE) {
			ei->sthandler=0;
			FD_CLR(fd,&stfds);
		}
		if (fd==nfds) {
			nfds=0;
			for(i=0;i<fd;i++) {
				if (fds[i].rdhandler||
				    fds[i].wrhandler||
				    fds[i].sthandler) {
					nfds=i;
				}
			}
		}
	}
	return 0;
}

static unsigned int evloop_tim=0;

int register_timer(unsigned int ms, EventHandler handler, void *uref) {
	if (handler) {
		timer.rdhandler = handler;
		timer.rduref = uref;
		timeout=ms;
	} else {
		timeout=0;
		timer.rdhandler=0;
		timer.rduref=0;
	}
	evloop_tim=0;
	return 0;
}


int do_event(void) {
	int i;
	int rc;
	fd_set lrfds=rfds;
	fd_set lwfds=wfds;
	fd_set lstfds=stfds;

	if (!evloop_tim) {
		evloop_tim=timeout;
	}

	rc=io_select(nfds+1,&lrfds,&lwfds,&lstfds,&evloop_tim);
	if (rc<0) return rc;
	if (rc==0) {
		if (timer.rdhandler) {
			timer.rdhandler(-1,0,timer.rduref);
		}
		evloop_tim=0;
	} else {
		for(i=0;i<nfds+1;i++) {
			if (FD_ISSET(i,&lwfds)) {
				if (fds[i].wrhandler) {
					fds[i].wrhandler(i,EV_WRITE,fds[i].wruref);
					rc--;
					if (!rc) return 0;
				}
			}
			if (FD_ISSET(i,&lstfds)) {
				if (fds[i].sthandler) {
					fds[i].sthandler(i,EV_STATE,fds[i].sturef);
					rc--;
					if (!rc) return 0;
				}
			}
			if (FD_ISSET(i,&lrfds)) {
				if (fds[i].rdhandler) {
					fds[i].rdhandler(i,EV_READ,fds[i].rduref);
					rc--;
					if (!rc) return 0;
				}
			}
		}
	}
	return 0;
}
