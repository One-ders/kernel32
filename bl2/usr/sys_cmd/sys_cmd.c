/* $Nosix/Leanaux: , v1.1 2014/04/07 21:44:00 anders Exp $ */

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
 * @(#)sys_cmd.c
 */

#include <sys.h>
#include <string.h>
#include <io.h>
#include <sys_env.h>
#include <procps.h>
#include <devls.h>

struct dents {
	char name[32];
};

#define READDIR 9
#define DYNOPEN 10

static char toNum(int state) {
        switch(state) {
                case TASK_STATE_IDLE: return 'i';
                case TASK_STATE_RUNNING: return 'r';
                case TASK_STATE_READY: return 'w';
                case TASK_STATE_TIMER: return 't';
                case TASK_STATE_IO: return 'b';
                default: return '?';
        }
}

static int get_procdata(int fd, char *name, struct Env *env) {
	struct procdata pd;
	int rc;
	int fd2;

	if ((fd2=io_control(fd, DYNOPEN, name, 0))<0) {
		fprintf(env->io_fd, "could not open proc data\n");
		return -1;
	}
	
	rc=io_read(fd2,&pd,sizeof(pd));
	if(rc>0) {
		fprintf(env->io_fd, "task(%3d@%x) %12s, sp=0x%08x, pc=0x%08x, prio=%x, state=%c, atics=%d\n",
		pd.id, pd.addr, pd.name, pd.sp, pd.pc, pd.prio_flags, toNum(pd.state),pd.active_tics);
	}
	io_close(fd2);
	return 0;
}

static int ps_fnc(int argc, char **argv, struct Env *env) {
	int fd=io_open("procps");
	struct dents dents[10];
	int rc,i=0;
	int tic;

	if (fd<0) {
		fprintf(env->io_fd,"could not open ps driver\n");
		return -1;
	}
	
	rc=io_control(fd,READDIR,dents,10);
	
	if (rc<0) {
		fprintf(env->io_fd,"directory error\n");
		return 0;
	}

	tic=get_current_tic();
	fprintf(env->io_fd, "uptime: %t, current tic: %d\n", tic);
	while(i<rc) {
		get_procdata(fd,dents[i].name,env);
		i++;
	}
	io_close(fd);
	return 0;
}

static int get_devdata(int fd, char *name, struct Env *env) {
	struct devdata dd;
	int rc;
	int fd2;
	if ((fd2=io_control(fd, DYNOPEN, name, 0))<0) {
		fprintf(env->io_fd, "could not open dev data\n");
		return -1;
	}
	
	rc=io_read(fd2,&dd,sizeof(dd));
	if(rc>0) {
		int i;
		fprintf(env->io_fd, "%12s: ", name);
		for(i=0;i<dd.numofpids;i++) {
			if (i) {
				fprintf(env->io_fd,", %d",dd.pid[i]);
			} else {
				fprintf(env->io_fd,"%d",dd.pid[i]);
			}
		}
		fprintf(env->io_fd,"\n");
	}
	io_close(fd2);
	return 0;
}


static int lsdrv_fnc(int argc, char **argv, struct Env *env) {
	int fd=io_open("dev");
//	struct devdata d;
	struct dents dents[32];
	int i,rc;
	if (fd<0) {
		fprintf(env->io_fd,"could not open dev\n");
		return -1;
	}
	rc=io_control(fd,READDIR,dents,128);
	fprintf(env->io_fd,"=========== Installed drivers =============\n");
	for(i=0;i<rc;i++) {
		get_devdata(fd,dents[i].name,env);
        }
        fprintf(env->io_fd,"========= End Installed drivers ===========\n");
	io_close(fd);
        return 0;
}

static int kmem_fnc(int argc, char **argv, struct Env *env) {
	int fd=io_open("kmem");
	int rc=0;
	int r=0,w=0;
	struct getopt_data gd;
	char *nptr;
	int opt;
	unsigned long int address=0;

	if (fd<0) {
		fprintf(env->io_fd,"could not open dev kmem\n");
		return -1;
	}

	getopt_data_init(&gd);

	while((opt=getopt_r(argc,argv,"r:w:",&gd))!=-1) {
		switch(opt) {
			case 'r':
				nptr=gd.optarg;
				address=strtoul(gd.optarg,&nptr,0);
				if (nptr==gd.optarg) {
					fprintf(env->io_fd, "address value %s, not a number\n",gd.optarg);
					rc = -1;
					goto out;
				}
				r=1;
				fprintf(env->io_fd,"read addr %x\n",address);
				break;
			case 'w':
				nptr=gd.optarg;
				address=strtoul(gd.optarg,&nptr,0);
				if (nptr==gd.optarg) {
					fprintf(env->io_fd, "address value %s, not a number\n",gd.optarg);
					rc = -1;
					goto out;
				}
				w=1;
				fprintf(env->io_fd,"write addr %x\n",address);
				break;
			default: {
				fprintf(env->io_fd, "kmem arg error %c\n",opt);
				rc=-1;
				goto out;
			}
		}
	}


	if (r) {  // read command
		unsigned int nbytes=16;
		int i=0,j;
		unsigned int nwords;

		if (gd.optind<argc) {
			nptr=argv[gd.optind];
			nbytes=strtoul(argv[gd.optind],&nptr,0);
			if (nptr==argv[gd.optind]) {
				fprintf(env->io_fd, "nbytes value %s, not a number\n",argv[gd.optind]);
				rc = -1;
				goto out;
			}
		}

		rc=io_lseek(fd,address,SEEK_SET);
		if (rc==-1) {
			goto out;
		}

		nwords=((nbytes+3)>>2);
		while(i<nwords) {
			fprintf(env->io_fd,"%08x:\t",address+(i<<2));
			for(j=0;(j<4)&&i<nwords;i++,j++) {
				unsigned int vbuf;
				rc=io_read(fd,&vbuf,4);
				if (rc<0) {
					rc=-11;
					goto out;
				}
				fprintf(env->io_fd,"%08x ", vbuf);
			}
			fprintf(env->io_fd,"\n");
		}
	} else if (w) {
		unsigned long int value;

		if (gd.optind<argc) {
			nptr=argv[gd.optind];
			value=strtoul(argv[gd.optind],&nptr,0);
			if (nptr==argv[gd.optind]) {
				fprintf(env->io_fd, "value %s, not a number\n",argv[gd.optind]);
				rc = -1;
				goto out;
			}
		}

		rc=io_lseek(fd,address,SEEK_SET);
		if (rc==-1) {
			goto out;
		}

		fprintf(env->io_fd,"%08x:\t=%08x",address,value);
		rc=io_write(fd,&value,4);
		if (rc<0) {
			rc=-1;
			goto out;
		}
	} else {
		fprintf(env->io_fd, "kmem: unknown operation\n");
	}
out:
	io_close(fd);
	return rc;
}

static int debug_fnc(int argc, char **argv, struct Env *env) {
	int dbglev;
	if (argc>1) {
		if (__builtin_strcmp(argv[1],"on")==0) {
			dbglev=10;
		} else if (__builtin_strcmp(argv[1],"off")==0) {
			dbglev=0;
		} else {
			fprintf(env->io_fd,"debug <on> | <off>\n");
			return 0;
		}
	} else {
		dbglev=0;
	}
	set_debug_level(dbglev);
	return 0;
} 

static int block_fnc(int argc, char **argv, struct Env *env) {
	int rc;
	fprintf(env->io_fd, "blocking %s, ", argv[1]);
	rc=block_task(argv[1]);
	fprintf(env->io_fd, "returned %d\n", rc);

	return 0;
}

static int unblock_fnc(int argc, char **argv, struct Env *env) {
	int rc;
	fprintf(env->io_fd, "unblocking %s, ", argv[1]);
	rc=unblock_task(argv[1]);
	fprintf(env->io_fd, "returned %d\n", rc);

	return 0;
}

static int kill_fnc(int argc, char **argv, struct Env *env) {
	int rc;
	fprintf(env->io_fd, "killing %s, ", argv[1]);
	rc=kill_task(argv[1]);
	fprintf(env->io_fd, "returned %d\n", rc);

	return 0;
}


static int setprio_fnc(int argc, char **argv, struct Env *env) {
	int rc;
	int prio=strtoul(argv[2],0,0);
	if (prio>MAX_PRIO) {
		fprintf(env->io_fd,"setprio: prio must be between 0-4\n");
		return 0;
	}
	fprintf(env->io_fd, "set prio of %s to %d", argv[1], prio);
	rc=setprio_task(argv[1],prio);
	fprintf(env->io_fd, "returned %d\n", rc);

	return 0;
}

static int reboot_fnc(int argc, char **argv, struct Env *env) {
	fprintf(env->io_fd, "rebooting\n\n\n");
	sleep(100);
	_reboot_(0x5a5aa5a5);
	return 0;
}

#if 0
extern int fb_test(void *);

static int testprog(int argc, char **argv, struct Env *env) {
	thread_create(fb_test,"groda",6,1,"fb_test");
	return 0;
}
#endif

static struct cmd cmd_root[] = {
		{"help", generic_help_fnc},
		{"ps", ps_fnc},
		{"lsdrv",lsdrv_fnc},
		{"debug", debug_fnc},
		{"block",block_fnc},
		{"unblock",unblock_fnc},
		{"setprio",setprio_fnc},
		{"kill", kill_fnc},
		{"reboot",reboot_fnc},
		{"kmem",kmem_fnc},
//		{"testprog",testprog},
		{0,0}
};

static struct cmd_node my_cmd_node = {
	"",
	cmd_root,
};

//void init_blinky(void);

void init_cec_a1();

char *barg[] = {"blinky", "on"};

void main(void *dum) {
	char buf[256];
	int fd=io_open(dum);
	struct Env env;
	static int u_init=0;
	if (fd<0) return;

	env.io_fd=fd;
	fprintf(fd,"Starting sys_mon\n");

	if (!u_init) {
		u_init=1;
		install_cmd_node(&my_cmd_node,0);
//		init_blinky();
#ifdef TEST_USB_SERIAL
		thread_create(main,"usb_serial0",12,1,"sys_mon:usb");
#else
		init_cec_a1();
#endif
	}

//	blinky(2, barg, &env);

	while(1) {
		int rc;
		rc=readline_r(fd,"\n--->",buf,200);
		if (rc>0) {
			struct cmd *cmd;
			int argc;
			char *argv[16];
			if (rc>200) {
				rc=200;
			}
			buf[rc]=0;
			rc=argit(buf,rc,argv);
			if (rc<0) {
				continue;
			}
			argc=rc;
			cmd=lookup_cmd(argv[0],fd);
			if (cmd) {
				int rc;
//				fprintf(fd,":iofd is %d\n",env.io_fd);
				fprintf(fd,"\n");
				rc=cmd->fnc(argc,argv,&env); 
				if (rc<0) {
					fprintf(fd,"%s returned %d\n",argv[0],rc);
				}
			}
		}
	}
}
