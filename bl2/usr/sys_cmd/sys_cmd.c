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
 * @(#)sys_cmd.c
 */

#include <sys/select.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <mycore/sys.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>
#include <mycore/io.h>
#include <mycore/sys_env.h>
#include <mycore/procps.h>
#include <mycore/devls.h>
#include <mycore/nand.h>
#include <mycore/kmem.h>

static int dumptlb_fnc(int argc, char **argv, struct Env *env);

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
		dprintf(env->io_fd, "could not open proc data\n");
		return -1;
	}

	rc=io_read(fd2,&pd,sizeof(pd));
	if(rc>0) {
		dprintf(env->io_fd, "task(%3d@%08x) %14s, sp=0x%08x, pc=0x%08x, prio=%x, state=%c, atics=%d\n",
		pd.id, pd.addr, pd.name, pd.sp, pd.pc, pd.prio_flags, toNum(pd.state),pd.active_tics);
	}
	io_close(fd2);
	return 0;
}

static int ps_fnc(int argc, char **argv, struct Env *env) {
	int fd=io_open("procps");
	struct dents dents[256];
	char buf[16];
	int rc,i=0;
	int tic;

	if (fd<0) {
		dprintf(env->io_fd,"could not open ps driver\n");
		return -1;
	}

	rc=io_control(fd,READDIR,dents,256);

	if (rc<0) {
		dprintf(env->io_fd,"directory error\n");
		return 0;
	}

	tic=get_current_tic();
	dprintf(env->io_fd, "uptime: %s, current tic: %d\n", ts_format(tic,buf,16), tic);
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
		dprintf(env->io_fd, "could not open dev data\n");
		return -1;
	}

	rc=io_read(fd2,&dd,sizeof(dd));
	if(rc>0) {
		int i;
		dprintf(env->io_fd, "%12s: ", name);
		for(i=0;i<dd.numofpids;i++) {
			if (i) {
				dprintf(env->io_fd,", %d",dd.pid[i]);
			} else {
				dprintf(env->io_fd,"%d",dd.pid[i]);
			}
		}
		dprintf(env->io_fd,"\n");
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
		dprintf(env->io_fd,"could not open dev\n");
		return -1;
	}
	rc=io_control(fd,READDIR,dents,128);
	dprintf(env->io_fd,"=========== Installed drivers =============\n");
	for(i=0;i<rc;i++) {
		get_devdata(fd,dents[i].name,env);
        }
        dprintf(env->io_fd,"========= End Installed drivers ===========\n");
	io_close(fd);
        return 0;
}

static int kmem_fnc(int argc, char **argv, struct Env *env) {
	int fd=io_open("kmem");
	int rc=0;
	int r=0,w=0;
	char *nptr;
	int opt;
	unsigned long int address=0;

	if (fd<0) {
		dprintf(env->io_fd,"could not open dev kmem\n");
		return -1;
	}

	while((opt=getopt(argc,argv,"r:w:"))!=-1) {
		switch(opt) {
			case 'r':
				nptr=optarg;
				address=strtoul(optarg,&nptr,0);
				if (nptr==optarg) {
					dprintf(env->io_fd, "address value %s, not a number\n",optarg);
					rc = -1;
					goto out;
				}
				r=1;
				dprintf(env->io_fd,"read addr %x\n",address);
				break;
			case 'w':
				nptr=optarg;
				address=strtoul(optarg,&nptr,0);
				if (nptr==optarg) {
					dprintf(env->io_fd, "address value %s, not a number\n",optarg);
					rc = -1;
					goto out;
				}
				w=1;
				dprintf(env->io_fd,"write addr %x\n",address);
				break;
			default: {
				dprintf(env->io_fd, "kmem arg error %c\n",opt);
				rc=-1;
				goto out;
			}
		}
	}


	if (r) {  // read command
		unsigned int nbytes=16;
		int i=0,j;
		unsigned int nwords;

		if (optind<argc) {
			nptr=argv[optind];
			nbytes=strtoul(argv[optind],&nptr,0);
			if (nptr==argv[optind]) {
				dprintf(env->io_fd, "nbytes value %s, not a number\n",argv[optind]);
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
			dprintf(env->io_fd,"%08x:\t",address+(i<<2));
			for(j=0;(j<4)&&i<nwords;i++,j++) {
				unsigned int vbuf;
				rc=io_read(fd,&vbuf,4);
				if (rc<0) {
					rc=-11;
					goto out;
				}
				dprintf(env->io_fd,"%08x ", vbuf);
			}
			dprintf(env->io_fd,"\n");
		}
	} else if (w) {
		unsigned long int value;

		if (optind<argc) {
			nptr=argv[optind];
			value=strtoul(argv[optind],&nptr,0);
			if (nptr==argv[optind]) {
				dprintf(env->io_fd, "value %s, not a number\n",argv[optind]);
				rc = -1;
				goto out;
			}
		}

		rc=io_lseek(fd,address,SEEK_SET);
		if (rc==-1) {
			goto out;
		}

		dprintf(env->io_fd,"%08x:\t=%08x",address,value);
		rc=io_write(fd,&value,4);
		if (rc<0) {
			rc=-1;
			goto out;
		}
	} else {
		dprintf(env->io_fd, "kmem: unknown operation\n");
	}
out:
	io_close(fd);
	return rc;
}

static int nand_fnc(int argc, char **argv, struct Env *env) {
	int fd;
	int rc=0;
	char cmd;
	char *nptr;
	int dev_no;
	char dev_name[8];
	unsigned long int value=0;
	unsigned long int size=0;
	unsigned long int block_no;
	unsigned long int address=0;

/*      nand read <nr> <offset> <size>
        nand write <nr> <offset> <data>
        nand erase <nr> <block>
*/

	if (argc<2) {
		dprintf(env->io_fd,"nand <nr> read  <offset> <size>\n");
		dprintf(env->io_fd,"nand <nr> write <offset> <data>\n");
		dprintf(env->io_fd,"nand <nr> erase <block>\n");
		return 0;
	}

	dev_no=strtoul(argv[1],&nptr,0);
	if (nptr==argv[1]) {
		dprintf(env->io_fd,"strtoul failed for nand %s\n",argv[1]);
		return 0;
	}

	sprintf(dev_name,"nand%d",dev_no);

	if (strcmp(argv[2],"read")==0) {
		cmd='r';
	} else if (strcmp(argv[2],"write")==0) {
		cmd='w';
	} else if (strcmp(argv[2],"erase")==0) {
		cmd='e';
	} else {
		dprintf(env->io_fd,"nand: unknown cmd %s\n",argv[2]);
		return 0;
	}

	if (cmd=='r' || cmd=='w') {
		address=strtoul(argv[3],&nptr,0);
		if (nptr==argv[3]) {
			dprintf(env->io_fd,"not a offset %s\n", argv[3]);
			return 0;
		}
	}

	if (cmd=='e') {
		block_no=strtoul(argv[3],&nptr,0);
		if (nptr==argv[3]) {
			dprintf(env->io_fd,"not a block number %s\n", argv[3]);
			return 0;
		}
	}

	if (cmd=='r') {
		size=strtoul(argv[4],&nptr,0);
		if (nptr==argv[4]) {
			dprintf(env->io_fd,"not a size %s\n", argv[4]);
			return 0;
		}
	}

	if (cmd=='w') {
		value=strtoul(argv[4],&nptr,0);
		if (nptr==argv[4]) {
			dprintf(env->io_fd,"not a size %s\n", argv[4]);
			return 0;
		}
	}

	dprintf(env->io_fd,"nand %d %c\n", dev_no, cmd);


	fd=io_open(dev_name);
	if (fd<0) {
		dprintf(env->io_fd,"could not open dev nand\n");
		return -1;
	}


	if (cmd=='r') {  // read command
		unsigned char vbuf[4096];
		unsigned int nbytes=16;
		int i=0,j,k;
		unsigned int nwords;
		int rc;

		rc=io_lseek(fd,address,SEEK_SET);
		if (rc==-1) {
			goto out;
		}

		rc=io_control(fd,NAND_READ_RAW,vbuf,2048+64);
//		nwords=((rc+3)>>2);
		nwords=2048>>2;
		while(i<nwords) {
			dprintf(env->io_fd,"%08x:\t",address+(i<<2));
			for(j=0;(j<4)&&i<nwords;i++,j++) {
				dprintf(env->io_fd,"%08x ", ((unsigned int *)vbuf)[i]);
			}
			dprintf(env->io_fd,"\n");
		}
		dprintf(env->io_fd, "=========== OOB Data ===========\n");
		nwords=64>>2;
		k=i;
		i=0;
		while(i<nwords) {
			dprintf(env->io_fd,"%08x:\t",(i<<2));
			for(j=0;(j<4)&&i<nwords;i++,j++) {
				dprintf(env->io_fd,"%08x ", ((unsigned int *)vbuf)[i+k]);
			}
			dprintf(env->io_fd,"\n");
		}
#if 0
	} else if (w) {
		unsigned long int value;

		if (gd.optind<argc) {
			nptr=argv[gd.optind];
			value=strtoul(argv[gd.optind],&nptr,0);
			if (nptr==argv[gd.optind]) {
				dprintf(env->io_fd, "value %s, not a number\n",argv[gd.optind]);
				rc = -1;
				goto out;
			}
		}

		rc=io_lseek(fd,address,SEEK_SET);
		if (rc==-1) {
			goto out;
		}

		dprintf(env->io_fd,"%08x:\t=%08x",address,value);
		rc=io_write(fd,&value,4);
		if (rc<0) {
			rc=-1;
			goto out;
		}
#endif
	} else {
		dprintf(env->io_fd, "nand: unknown operation\n");
	}
out:
	io_close(fd);
	return rc;
}


static int debug_fnc(int argc, char **argv, struct Env *env) {
	int dbglev;
	if (argc>1) {
		char *nptr;
		unsigned int address=strtoul(argv[1],&nptr,0);
		if (nptr!=argv[1]) {
			dbglev=address;
		} else if (strcmp(argv[1],"on")==0) {
			dbglev=0xffffffff;
		} else if (strcmp(argv[1],"off")==0) {
			dbglev=0;
		} else {
			dprintf(env->io_fd,"debug <on> | <off>\n");
			return 0;
		}
	} else {
		dbglev=0;
	}
	dprintf(env->io_fd,"setting debug level to %x\n", dbglev);
	set_debug_level(dbglev);
	return 0;
}

static int block_fnc(int argc, char **argv, struct Env *env) {
	int rc;
	dprintf(env->io_fd, "blocking %s, ", argv[1]);
	rc=block_task(argv[1]);
	dprintf(env->io_fd, "returned %d\n", rc);

	return 0;
}

static int unblock_fnc(int argc, char **argv, struct Env *env) {
	int rc;
	dprintf(env->io_fd, "unblocking %s, ", argv[1]);
	rc=unblock_task(argv[1]);
	dprintf(env->io_fd, "returned %d\n", rc);

	return 0;
}

static int kill_fnc(int argc, char **argv, struct Env *env) {
	int rc;
	dprintf(env->io_fd, "killing %s, ", argv[1]);
	rc=kill_process(argv[1]);
	dprintf(env->io_fd, "returned %d\n", rc);

	return 0;
}


static int setprio_fnc(int argc, char **argv, struct Env *env) {
	int rc;
	int prio=strtoul(argv[2],0,0);
	if (prio>MAX_PRIO) {
		dprintf(env->io_fd,"setprio: prio must be between 0-4\n");
		return 0;
	}
	dprintf(env->io_fd, "setprio of %s to %d", argv[1], prio);
	rc=setprio_task(argv[1],prio);
	dprintf(env->io_fd, "returned %d\n", rc);

	return 0;
}

static int reboot_fnc(int argc, char **argv, struct Env *env) {
	dprintf(env->io_fd, "rebooting\n\n\n");
	usleep(100*1000);
	_reboot_(0x5a5aa5a5);
	return 0;
}

#if 1
extern int fb_test(void *);

static int testprog(int argc, char **argv, struct Env *env) {
	thread_create(fb_test,"groda",6,1,"fb_test");
	return 0;
}
#endif

static int fork_test(int argc, char **argv, struct Env *env) {
	int rc=my_fork();
	if (!rc) {
		dprintf(env->io_fd, "in new process\n");
		while(1) {
			msleep(100);
		}
	} else {
		dprintf(env->io_fd, "in  parent, pid of new process is %d\n",rc);
	}

	return 0;
}

static int loadnrun_fnc(int argc, char **argv, struct Env *env) {
	int fd;
	int rc;
	int npid;
	struct stat stbuf;
	int bg=0;

	if (strcmp(argv[0],"loadnrun")==0) {
		argv++;
		argc--;
	}

	if (!strcmp(argv[argc-1],"&")) {
		bg=1;
		argc--;
	}

	if (argc<1) {
		dprintf(env->io_fd, "need at least file name of loadfile\n");
		return -1;
	}

	memset(&stbuf,0,sizeof(stbuf));
	rc=stat(argv[0], &stbuf);
	if (rc<0) {
		dprintf(env->io_fd, "error %d for stat file %s\n",errno,argv[0]);
		return -errno;
	}

	npid=my_fork();

	if (npid==-1) {
		printf("fork failed\n");
		return -1;
	} else if (npid) {
		int status;
		if (!bg) {
			wait(&status);
		}
	} else {
		char *newenviron[] = { NULL };
		execve(argv[0], &argv[0], newenviron);
		dprintf(env->io_fd, "return after execve... Errrir\n");
		exit(0);
	}
	return 0;
}

static int meminfo_fnc(int argc, char **argv, struct Env *env) {
	int fd=io_open("kmem");
	int rc=0;
	int r=0,w=0;
	char *nptr;
	int opt;
	unsigned long int address=0;
	struct meminfo meminfo;

	if (fd<0) {
		dprintf(env->io_fd,"could not open dev kmem\n");
		return -1;
	}

	memset(&meminfo,0,sizeof(meminfo));
	rc=io_control(fd,KMEM_GET_MEMINFO, &meminfo, sizeof(meminfo));
	if (rc<0) {
		io_close(fd);
		return -1;
	}

	dprintf(env->io_fd," Total ammount of Ram %d bytes\n", meminfo.totalRam);
	dprintf(env->io_fd," Nr of free Pages %d\n", meminfo.freePages);
	dprintf(env->io_fd," Nr of shared Pages %d\n", meminfo.sharedPages);
	io_close(fd);
	return 0;
}

static int psdumpmap_fnc(int argc, char **argv, struct Env *env) {
	int fd=io_open("kmem");
	int rc=0;
	int r=0,w=0;
	char *nptr;
	int opt;
	unsigned long int address=0;
	struct ps_memmap ps_memmap;

	if (fd<0) {
		dprintf(env->io_fd,"could not open dev kmem\n");
		return -1;
	}

	if (argc!=2) {
		io_close(fd);
		return -1;
	}

	memset(&ps_memmap,0,sizeof(ps_memmap));
	ps_memmap.ps_name=argv[1];
	ps_memmap.vaddr=0;
	rc=io_control(fd,KMEM_GET_FIRST_PSMAP, &ps_memmap, sizeof(ps_memmap));
	if (rc<0) {
		io_close(fd);
		return -1;
	}
	while(1) {
		dprintf(env->io_fd, "vaddr=%8x, pte=%8x, sh info=%x\n", ps_memmap.vaddr, ps_memmap.pte, ps_memmap.sh_data);
		rc=io_control(fd,KMEM_GET_NEXT_PSMAP, &ps_memmap, sizeof(ps_memmap));
		if (rc<0) break;
	}

	io_close(fd);
	return 0;
}

static int dumptlb_fnc(int argc, char **argv, struct Env *env) {
	int fd=io_open("kmem");
	int rc=0;

	if (fd<0) {
		dprintf(env->io_fd,"could not open dev kmem\n");
		return -1;
	}
	rc=io_control(fd,KMEM_DUMP_TLB, 0, 0);

	io_close(fd);
	return rc;
}

static int flushtlb_fnc(int argc, char **argv, struct Env *env) {
	int fd=io_open("kmem");
	int rc=0;

	if (fd<0) {
		dprintf(env->io_fd,"could not open dev kmem\n");
		return -1;
	}
	rc=io_control(fd,KMEM_FLUSH_TLB, 0, 0);

	io_close(fd);
	return rc;
}

static int loop_fnc(void *dum) {
	while(1);
	return 1;
}

static int runloop_fnc(int argc, char **argv, struct Env *env) {
	thread_create(loop_fnc,0,0,3,"loop");
	return 0;
}

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
		{"nand",nand_fnc},
		{"testprog",testprog},
		{"fork",fork_test},
		{"meminfo", meminfo_fnc},
		{"dumpmap", psdumpmap_fnc},
		{"dump_tlb", dumptlb_fnc},
		{"flush_tlb", flushtlb_fnc},
		{"loadnrun", loadnrun_fnc},
		{"runloop", runloop_fnc},
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
//	int fd=io_open(dum);
	struct Env env;
	static int u_init=0;
//	if (fd<0) return;

	env.io_fd=1;

	setlinebuf(stdout);
	fflush(stdout);
	printf("Starting sys_mon\n");

	if (!u_init) {
		u_init=1;
		install_cmd_node(&my_cmd_node,0);
//		init_blinky();
#ifdef TEST_USB_SERIAL
		thread_create(main,"usb_serial0",12,1,"sys_mon:usb");
#else
//		init_cec_a1();
#endif
	}

//	blinky(2, barg, &env);

	while(1) {
		int rc;
		rc=readline_r(0,"\n--->",buf,200);
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
			cmd=lookup_cmd(argv[0],1);
			if (cmd) {
				int rc;
				dprintf(1,"\n");
				rc=cmd->fnc(argc,argv,&env);
				if (rc<0) {
					dprintf(1,"%s returned %d\n",argv[0],rc);
				}
			} else {
				dprintf(1,"\n");
				rc=loadnrun_fnc(argc,argv,&env);
			}
		}
	}
}
