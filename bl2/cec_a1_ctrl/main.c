/* $CecA1GW: main.c, v1.1 2014/04/07 21:44:00 anders Exp $ */

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
 * @(#)main.c
 */

#include "sys.h"
#include "io.h"
#include "cec_drv.h"
#include "cec.h"
#include "a1_drv.h"
#include "a1.h"
#include "asynchio.h"

#include <string.h>

#if DEBUG

int cec_debug;

static int debug_fnc(int argc, char **argv, struct Env *env);
static int set_fnc(int argc, char **argv, struct Env *env);
static int get_fnc(int argc, char **argv, struct Env *env);
static int scan_fnc(int argc, char **argv, struct Env *env);
static int wake_fnc(int argc, char **argv, struct Env *env);
static int send_a1_fnc(int argc, char **argv, struct Env *env);

static struct cmd cmds[] = {
	{"help", generic_help_fnc},
	{"debug", debug_fnc},
	{"set", set_fnc},
	{"get", get_fnc},
	{"scan",scan_fnc},
	{"wake",wake_fnc},
	{"send_a1",send_a1_fnc},
	{0,0}
};

static struct cmd_node cmn = {
	"cec",
	cmds,
};

static int debug_fnc(int argc, char **argv, struct Env *env) {
	if (argc==1) {
		cec_debug=1;
	} else {
		cec_debug=0;
	}
	return 0;
}

static int set_fnc(int argc, char **argv, struct Env *env) {
	char *var=argv[1];
	char *val=argv[2];
	int rc=-1;
	int myfd=io_open(CEC_DRV);
	if (myfd<0) return -1;
	if (argc!=3) {
		fprintf(env->io_fd,"wrong arg count\n");
		io_close(myfd);
		return -1;
	}

	if (!__builtin_strcmp(var,"addr_ack_mask")) {
		unsigned int ack_mask=strtoul(val,0,0);
		printf("setting ack_mask to 0x%x, from %s\n",ack_mask,val);
		rc=io_control(myfd,CEC_SET_ACK_MSK,&ack_mask,sizeof(ack_mask));
	} else if (!__builtin_strcmp(var,"promisc")) {
		unsigned int val_i=strtoul(val,0,0);
		rc=io_control(myfd,CEC_SET_PROMISC,&val_i,sizeof(val_i));
	} else {
		fprintf(env->io_fd,"unknown var %s\n",var);
	}
	io_close(myfd);
	return rc;
}

static int get_fnc(int argc, char **argv, struct Env *env) {
	char *var=argv[1];
	int rc=-1;
	int myfd=io_open(CEC_DRV);
	if (myfd<0) return -1;
	if (argc!=2) {
		fprintf(env->io_fd,"wrong arg count\n");
		io_close(myfd);
		return -1;
	}

	if (!__builtin_strcmp(var,"addr_ack_mask")) {
		unsigned int ack_mask;
		rc=io_control(myfd,CEC_GET_ACK_MSK,&ack_mask,sizeof(ack_mask));
		if (!rc) {
			fprintf(env->io_fd,"ack_mask=%x\n",ack_mask);
		}
	} else if (!__builtin_strcmp(var,"promisc")) {
		unsigned int val;
		rc=io_control(myfd,CEC_GET_PROMISC,&val,sizeof(val));
		if (!rc) {
			fprintf(env->io_fd,"promisc=%x\n",val);
		}
	} else {
		fprintf(env->io_fd,"unknown var %s\n",var);
	}
	io_close(myfd);
	return rc;
}


static int scan_fnc(int argc, char **argv, struct Env *env) {
	int i;
	int myfd=io_open(CEC_DRV);
	if (myfd<0) {
		fprintf(env->io_fd,"could not open cec_drv\n");
		return -1;
	}
	for (i=0;i<0xf;i++) {
		unsigned char addr=0xf0|i;
		int rc=io_write(myfd,(char *)&addr,1);
		if (rc<0) {
			fprintf(env->io_fd,"send %x failed\n",addr);
		} else {
			fprintf(env->io_fd,"send %x success return %d\n",addr,rc);
		}
	}
	io_close(myfd);
	return 0;
}

static int wake_fnc(int argc, char **argv, struct Env *env) {
	wakeup_usb_dev();
	return 0;
}

/* Note the commands are run from the sys_mon task, so dont use
 * task specific data like filedescriptors
 */
static int send_a1_fnc(int argc, char **argv, struct Env *env) {
        int myfd=io_open(A1_DRV0);
        unsigned char buf[argc];
        int i;
        int rc;
        for (i=1;i<argc;i++) {
                unsigned int val=strtoul(argv[i],0,16);
                printf("got byte %x\n",val);
                buf[i-1]=val;
        }
        rc=io_write(myfd,(char *)buf,argc-1);
        printf("a1: write returned %d\n",rc);
        io_close(myfd);
        return 0;
}

#endif

extern int init_pulse_eight(void);

void cec_gw(void *dum) {

	a1_init_a1();
	cec_init_cec();
	init_pulse_eight();

	install_cmd_node(&cmn,root_cmd_node);

	while(1) do_event();
}

int main(void) {

	/* initialize the executive */
	init_sys();
	init_io();

	/* start the executive */
	start_sys();
	printf("In main, starting tasks\n");

	/* create some jobs */
	thread_create(cec_gw,0,1,"cec_gw");
	while (1);
}
