
#include <io.h>
#include <sys.h>
#include <frame.h>
#include <string.h>

#include "syscall.h.in" // musl linux syscalls

//#include <system_params.h>

static int lnx_sys_map(unsigned int lnx_scall) {

	switch(lnx_scall) {
		case __NR_dup:		return LNX_DUP;
		case __NR_fcntl:	return LNX_FCNTL;
		case __NR_ioctl:	return LNX_IOCTL;
		case __NR_flock:	return LNX_FLOCK;
		case __NR_statfs:	return LNX_STATFS;
		case __NR_fstatfs:	return LNX_FSTATFS;
		case __NR_truncate:	return LNX_TRUNCATE;
		case __NR_ftruncate:	return LNX_FTRUNCATE;
		case __NR_chdir:	return LNX_CHDIR;
		case __NR_fchdir:	return LNX_FCHDIR;
		case __NR_chroot:	return LNX_CHROOT;
		case __NR_fchmod:	return LNX_FCHMOD;
		case __NR_fchown:	return LNX_FCHOWN;
		case __NR_close:	return LNX_CLOSE;
		case __NR_getdents64:	return LNX_GETDENTS64;
		case __NR_lseek:	return LNX_LSEEK;
		case __NR_read:		return LNX_READ;
		case __NR_write:	return LNX_WRITE;
		case __NR_readv:	return LNX_READV;
		case __NR_writev:	return LNX_WRITEV;
		case __NR_newfstatat:   return LNX_NEWFSTATAT;
		case __NR_fstat:	return LNX_FSTAT;
		case __NR_sync:		return LNX_SYNC;
		case __NR_fsync:	return LNX_FSYNC;
		case __NR_exit:		return LNX_EXIT;
		case __NR_nanosleep:	return LNX_NANOSLEEP;
		case __NR_getitimer:	return LNX_GETITIMER;
		case __NR_setitimer:	return LNX_SETITIMER;
		case __NR_ptrace:	return LNX_PTRACE;
		case __NR_kill:		return LNX_KILL;
		case __NR_sigaltstack:	return LNX_SIGALTSTACK;
		case __NR_setpriority:	return LNX_SETPRIORITY;
		case __NR_getpriority:	return LNX_GETPRIORITY;
		case __NR_reboot:	return LNX_REBOOT;
		case __NR_socket:	return LNX_SOCKET;
		case __NR_socketpair:	return LNX_SOCKETPAIR;
		case __NR_bind:		return LNX_BIND;
		case __NR_listen:	return LNX_LISTEN;
		case __NR_accept:	return LNX_ACCEPT;
		case __NR_connect:	return LNX_CONNECT;
		case __NR_getsockname:	return LNX_GETSOCKNAME;
		case __NR_getpeername:	return LNX_GETPEERNAME;
		case __NR_sendto:	return LNX_SENDTO;
		case __NR_recvfrom:	return LNX_RECVFROM;
		case __NR_setsockopt:	return LNX_SETSOCKOPT;
		case __NR_getsockopt:	return LNX_GETSOCKOPT;
		case __NR_shutdown:	return LNX_SHUTDOWN;
		case __NR_sendmsg:	return LNX_SENDMSG;
		case __NR_recvmsg:	return LNX_RECVMSG;
		case __NR_readahead:	return LNX_READAHEAD;
		case __NR_brk:		return LNX_BRK;
		case __NR_munmap:	return LNX_MUNMAP;
		case __NR_mremap:	return LNX_MREMAP;
		case __NR_clone:	return LNX_CLONE;
		case __NR_execve:	return LNX_EXECVE;
		case __NR_mmap:		return LNX_MMAP;
		case __NR_mprotect:	return LNX_MPROTECT;
		case __NR_msync:	return LNX_MSYNC;
		case __NR_mlock:	return LNX_MLOCK;
		case __NR_munlock:	return LNX_MUNLOCK;
		case __NR_mlockall:	return LNX_MLOCKALL;
		case __NR_munlockall:	return LNX_MUNLOCKALL;
		case __NR_mincore:	return LNX_MINCORE;
		case __NR_madvise:	return LNX_MADVISE;
		case __NR_wait4:	return LNX_WAIT4;
		default: return -1;
	}
	return -1;
}

int get_svc_number(void *sp, unsigned int *domain) {
	unsigned long int *sp_ptr=(unsigned long int *)sp;
	unsigned int svc_no=sp_ptr[0]&0xffff; // svc no, stored in SR_EL1
	// check svc number: 0 is Musl linux
	// 			else Native

	if (svc_no==0) {
		int mapped_svc=lnx_sys_map(sp_ptr[10]);
		*domain=SYSCALL_LNX;
		if (mapped_svc<0) {
			io_setpolled(1);
			sys_printf("no map for lnx svc %x\n", sp_ptr[10]);
			while(1);
		}
		return (mapped_svc);
	} else {
		*domain=SYSCALL_NATIVE;
		return svc_no;
	}
	return -1;
}

unsigned long int get_svc_arg(void *svc_sp, int arg) {
	return ((unsigned long int *)svc_sp)[arg+2];
}

void set_svc_ret(void *svc_sp, long int val) {
	((unsigned long int *)svc_sp)[2]=val;
}

void set_svc_lret(void *sp, long int val)  {
//        struct fullframe *ff=(struct fullframe *)sp;
//        ff->a3=val;
}

int array_size(char **argv) {
        int i=0;
        while(argv[i]) {
                i++;
        }
        return i;
}

int args_size(char **argv) {
        int total_size=0;
        int i=0;

        while(argv[i]) {
                total_size+=(strlen(argv[i])+1);
                i++;
        }
        return total_size;
}

int copy_arguments(char **argv_new, char **argv,
                        char *arg_storage, int nr_args) {
        int i=0;
        char *to=arg_storage;
        while(argv[i]) {
                argv_new[i]=strcpy(to,argv[i]);
                to+=strlen(argv[i]);
                *to=0;
                to++;
                i++;
        }
        return to-arg_storage;
}

int setup_return_stack(struct task *t, void *stackp_v,
					unsigned long int fnc,
					unsigned long int ret_fnc,
					char **arg0,
					char **arg1) {
	unsigned long int stacktop=(unsigned long int)stackp_v;
	t->sp=(void *)stacktop-800;
	unsigned long int *stackp=(unsigned long int *)t->sp;

	stackp[0]=0x00000000;                // esr_el1
	stackp[1]=(unsigned long int)fnc;    // slr_el1
	stackp[2]=(unsigned long int)arg0;    // x0
	stackp[3]=(unsigned long int)arg1;    // x1
	stackp[4]=0;     // x2
	stackp[5]=0;     // x3
	stackp[6]=0;     // x4
	stackp[7]=0;     // x5
	stackp[8]=0;     // x6
	stackp[9]=0;     // x7
	stackp[10]=0;    // x8
	stackp[11]=0;    // x9
	stackp[12]=0;    // x10
	stackp[13]=0;    // x11
	stackp[14]=0;     // x12
	stackp[15]=0;     // x13
	stackp[16]=0;     // x14
	stackp[17]=0;     // x15
	stackp[18]=0;     // x16
	stackp[19]=0;     // x17
	stackp[20]=0;     // x18
	stackp[21]=0;     // x19
	stackp[22]=0;     // x20
	stackp[23]=0;     // x21
	stackp[24]=0;     // x22
	stackp[25]=0;     // x23
	stackp[26]=0;     // x24
	stackp[27]=0;     // x25
	stackp[28]=0;     // x26
	stackp[29]=0;     // x27
	stackp[30]=0;     // x28
	stackp[31]=0;     // x29
	stackp[32]=(unsigned long int)ret_fnc;//svc_destroy_self; // r30
	stackp[33]=0;     // x31

	return 0;
}

void *handle_syscall(unsigned long int *sp);

void handle_trap(void *sp, unsigned long esr, unsigned long elr, unsigned long spsr, unsigned long far, unsigned long sctlr) {
	unsigned int ec=esr>>26; // exception class
	switch(ec) {
		case 0x15:
			handle_syscall(sp);
			break;
		case 0x25:
			sys_printf("Data abort\n");
			while(1);
		default:
			io_setpolled(1);
			sys_printf("unhandle trap ec=%x\n", ec);
			while(1);
			break;
	}
}

unsigned long int get_stacked_pc(struct task *t) {
	return ((struct fullframe *)t->sp)->x15;
}

unsigned long int get_usr_pc(struct task *t) {
//	unsigned long int *estackp=(unsigned long int *)t->estack;
	unsigned long int *stackp=t->sp;

	return stackp[1];
#if 0
	unsigned long int *bstackp=estackp+512;
	unsigned long int rval;
	unsigned long int cpu_flags;

	cpu_flags=disable_interrupts();
	for(;stackp<bstackp;stackp++) {
		if (stackp[0]==0xfffffff9) {
			rval=stackp[7];
			restore_cpu_flags(cpu_flags);
			return rval;
		}
	}
	restore_cpu_flags(cpu_flags);
	return 0;
#endif
}

int sys_udelay(unsigned int usec) {
	unsigned int count=0;
	unsigned int utime=(120*usec/7);
	do {
		if (++count>utime) {
			return 0;
		}
	} while (1);
}

int sys_mdelay(unsigned int msec) {
	sys_udelay(msec*1000);
	return 0;
}

int mapmem(struct task *t, unsigned long int vaddr, unsigned long int paddr, unsigned int attr) {
	if (vaddr!=paddr)
		return -1;
	return 0;
}

unsigned long int get_mmap_vaddr(struct task *current, unsigned int size) {
	return 0;
}

static void __WFE(void) {
  asm("wfe");
}

static void __WFI(void) {
  asm("wfi");
}


int halt() {
#if 0
	unsigned int *SCR=(unsigned int *)0xe000ed10;
	if (power_mode&POWER_MODE_DEEP_SLEEP) {
		*SCR|=0x4;
	} else {
		*SCR&=~0x4;
	}

	if (power_mode&POWER_MODE_WAIT_WFI) {
		__WFI();
	} else if (power_mode&POWER_MODE_WAIT_WFE) {
		__WFE();
	}
#endif
	return 0;
}

