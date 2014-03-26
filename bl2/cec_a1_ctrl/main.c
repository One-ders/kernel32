
#include "stm32f4xx_conf.h"
#include "sys.h"
#include "io.h"
#include "hr_timer.h"
#include "cec_drv.h"
#include "asynchio.h"
#include "cec.h"

#include <string.h>

int cec_debug;

static int debug_fnc(int argc, char **argv, struct Env *env);
static int set_fnc(int argc, char **argv, struct Env *env);
static int get_fnc(int argc, char **argv, struct Env *env);
static int scan_fnc(int argc, char **argv, struct Env *env);
static int wake_fnc(int argc, char **argv, struct Env *env);

static struct cmd cmds[] = {
	{"help", generic_help_fnc},
	{"debug", debug_fnc},
	{"set", set_fnc},
	{"get", get_fnc},
	{"scan",scan_fnc},
	{"wake",wake_fnc},
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


#define MAX(a,b)	(a>b)?a:b

extern int init_pulse_eight(void);

struct EventInfo {
	EventHandler rdhandler;
	void *rduref;
	EventHandler wrhandler;
	void *wruref;
	EventHandler sthandler;
	void *sturef;
};

static struct EventInfo fds[32];
static fd_set rfds;
static fd_set wfds;
static fd_set stfds;
static unsigned int nfds;


int register_event(int fd, int event,
			EventHandler handler,
			void *uref) {
	int i;
	struct EventInfo *ei=&fds[fd];
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

static int do_eventloop(void) {
	int i;
	fd_set lrfds=rfds;
	fd_set lwfds=wfds;
	fd_set lstfds=stfds;

	int rc=io_select(nfds+1,&lrfds,&lwfds,&lstfds,0);
	if (rc<0) return rc;
	for(i=0;i<nfds+1;i++) {
		if (FD_ISSET(i,&lwfds)) {
			if (fds[i].wrhandler) {
				fds[i].wrhandler(i,EV_WRITE,fds[i].wruref);
				rc--;
				if (!rc) return 0;
			}
		}
		if (!rc) return 0;
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
	return 0;
}

void cec_gw(void *dum) {

	cec_init_cec();
	init_pulse_eight();

	install_cmd_node(&cmn,root_cmd_node);

	while(1) do_eventloop();
}

int main(void) {
//	struct app_data app_data;

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
