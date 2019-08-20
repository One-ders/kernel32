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

#include <config.h>
#include "sys.h"
#include "sys_env.h"
#include "io.h"
#include "cec_drv.h"
#include "cec.h"
#include "a1_drv.h"
#include "a1.h"
#include "asynchio.h"
#include "iwdg_drv.h"

#include <string.h>

#ifdef DEBUG

int cec_debug;

static int debug_fnc(int argc, char **argv, struct Env *env);
static int set_fnc(int argc, char **argv, struct Env *env);
static int get_fnc(int argc, char **argv, struct Env *env);
static int scan_fnc(int argc, char **argv, struct Env *env);
static int wake_fnc(int argc, char **argv, struct Env *env);
#ifdef WITH_SONY_A1
static int send_a1_fnc(int argc, char **argv, struct Env *env);
#endif

static struct cmd cmds[] = {
	{"help", generic_help_fnc},
	{"debug", debug_fnc},
	{"set", set_fnc},
	{"get", get_fnc},
	{"scan",scan_fnc},
	{"wake",wake_fnc},
#ifdef WITH_SONY_A1
	{"send_a1",send_a1_fnc},
#endif
	{0,0}
};

static struct cmd_node cmn = {
	"cec",
	cmds,
};

static int debug_fnc(int argc, char **argv, struct Env *env) {
	fprintf(env->io_fd,"debug called with %d args\n",argc);
	if (argc!=2) {
		goto out_err;
	}

	if (strcmp(argv[1],"on")==0) {
		cec_debug=1;
	} else if (strcmp(argv[1],"off")==0) {
		cec_debug=0;
	} else {
		goto out_err;
	}
	return 0;

out_err:
	fprintf(env->io_fd,"debug needs argument (on or off)\n");
	return -1;

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
#ifdef WITH_SONY_A1
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

#endif

extern int init_pulse_eight(void);

void cec_gw(void *dum) {

#ifdef WITH_SONY_A1
	a1_init_a1();
#endif
	cec_init_cec();
	init_pulse_eight();

#ifdef DEBUG
	install_cmd_node(&cmn,root_cmd_node);
#endif

	while(1) do_event();
}

void watchdog(void *dum) {
	int fd=io_open(IWDG_DRV);
	char *bub="hej";
	if (fd<0) {
		return;
	}
	io_control(fd,F_SETFL,(void *)O_NONBLOCK,0);
	while(1) {
		sleep(250);
		io_write(fd,bub,1);
	}
}


//int main(void) {
int init_cec_a1(void) {
	/* create some jobs */
	thread_create(watchdog,0,0,3,"watchdog");
	thread_create(cec_gw,0,0,1,"cec_gw");
	return 0;
}
