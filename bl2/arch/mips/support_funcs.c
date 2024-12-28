#include <io.h>
#include <sys.h>
#include <frame.h>
#include <string.h>

#include "syscall.h.in"   // musl linux syscalls

extern unsigned long int *curr_pgd;


unsigned int get_svc_number(void *sp, unsigned int *domain)  {
	struct fullframe *ff=(struct fullframe *)sp;

	if (ff->v0<4000) {
		*domain=SYSCALL_NATIVE;
		return ff->v0;
	}

	*domain=SYSCALL_LNX;
	switch(ff->v0) {
		case __NR_exit: 	return LNX_EXIT;
		case __NR_fork: 	return LNX_FORK;
		case __NR_read:		return LNX_READ;
		case __NR_write:	return LNX_WRITE;
		case __NR_open:		return LNX_OPEN;
		case __NR_close:	return LNX_CLOSE;
		case __NR_waitpid:	return LNX_WAITPID;
		case __NR_creat:	return LNX_CREAT;
		case __NR_link:		return LNX_LINK;
		case __NR_unlink:	return LNX_UNLINK;
		case __NR_execve:	return LNX_EXECVE;
		case __NR_chdir:	return LNX_CHDIR;
		case __NR_time:		return LNX_TIME;
		case __NR_mknod:	return LNX_MKNOD;
		case __NR_chmod:	return LNX_CHMOD;
		case __NR_lchown:	return LNX_LCHOWN;
		case __NR_break:	return LNX_BREAK;
		case __NR_lseek:	return LNX_LSEEK;
		case __NR_getpid:	return LNX_GETPID;
		case __NR_mount:	return LNX_MOUNT;
		case __NR_umount:	return LNX_UMOUNT;
		case __NR_setuid:	return LNX_SETUID;
		case __NR_getuid:	return LNX_GETUID;
		case __NR_stime:	return LNX_STIME;
		case __NR_ptrace:	return LNX_PTRACE;
		case __NR_alarm:	return LNX_ALARM;
		case __NR_pause:	return LNX_PAUSE;
		case __NR_utime:	return LNX_UTIME;
		case __NR_stty:		return LNX_STTY;
		case __NR_gtty:		return LNX_GTTY;
		case __NR_access:	return LNX_ACCESS;
		case __NR_nice:		return LNX_NICE;
		case __NR_ftime:	return LNX_FTIME;
		case __NR_sync:		return LNX_SYNC;
		case __NR_kill:		return LNX_KILL;
		case __NR_rename:	return LNX_RENAME;
		case __NR_mkdir:	return LNX_MKDIR;
		case __NR_rmdir:	return LNX_RMDIR;
		case __NR_dup:		return LNX_DUP;
		case __NR_pipe:		return LNX_PIPE;
		case __NR_times:	return LNX_TIMES;
		case __NR_prof:		return LNX_PROF;
		case __NR_brk:		return LNX_BRK;
		case __NR_setgid:	return LNX_SETGID;
		case __NR_getgid:	return LNX_GETGID;
		case __NR_signal:	return LNX_SIGNAL;
		case __NR_geteuid:	return LNX_GETEUID;
		case __NR_getegid:	return LNX_GETEGID;
		case __NR_acct:		return LNX_ACCT;
		case __NR_umount2:	return LNX_UMOUNT2;
		case __NR_lock:		return LNX_LOCK;
		case __NR_ioctl:	return LNX_IOCTL;
		case __NR_fcntl:	return LNX_FCNTL;
		case __NR_mpx:		return LNX_MPX;
		case __NR_setpgid:	return LNX_SETPGID;
		case __NR_ulimit:	return LNX_ULIMIT;
		case __NR_umask:	return LNX_UMASK;
		case __NR_chroot:	return LNX_CHROOT;
		case __NR_ustat:	return LNX_USTAT;
		case __NR_dup2:		return LNX_DUP2;
		case __NR_getppid:	return LNX_GETPPID;
		case __NR_getpgrp:	return LNX_GETPGRP;
		case __NR_setsid:	return LNX_SETSID;
		case __NR_sigaction:	return LNX_SIGACTION;
		case __NR_sgetmask:	return LNX_SGETMASK;
		case __NR_ssetmask:	return LNX_SSETMASK;
		case __NR_setreuid:	return LNX_SETREUID;
		case __NR_setregid:	return LNX_SETREGID;
		case __NR_sigsuspend:	return LNX_SIGSUSPEND;
		case __NR_sigpending:	return LNX_SIGPENDING;
		case __NR_sethostname:	return LNX_SETHOSTNAME;
		case __NR_setrlimit:	return LNX_SETRLIMIT;
		case __NR_getrlimit:	return LNX_GETRLIMIT;
		case __NR_getrusage:	return LNX_GETRUSAGE;
		case __NR_gettimeofday:	return LNX_GETTIMEOFDAY;
		case __NR_settimeofday:	return LNX_SETTIMEOFDAY;
		case __NR_getgroups:	return LNX_GETGROUPS;
		case __NR_setgroups:	return LNX_SETGROUPS;
		case __NR_symlink:	return LNX_SYMLINK;
		case __NR_readlink:	return LNX_READLINK;
		case __NR_uselib:	return LNX_USELIB;
		case __NR_swapon:	return LNX_SWAPON;
		case __NR_reboot:	return LNX_REBOOT;
		case __NR_readdir:	return LNX_READDIR;
		case __NR_mmap:		return LNX_MMAP;
		case __NR_munmap:	return LNX_MUNMAP;
		case __NR_truncate:	return LNX_TRUNCATE;
		case __NR_ftruncate:	return LNX_FTRUNCATE;
		case __NR_fchmod:	return LNX_FCHMOD;
		case __NR_fchown:	return LNX_FCHOWN;
		case __NR_getpriority:	return LNX_GETPRIORITY;
		case __NR_setpriority:	return LNX_SETPRIORITY;
		case __NR_profil:	return LNX_PROFIL;
		case __NR_statfs:	return LNX_STATFS;
		case __NR_fstatfs:	return LNX_FSTATFS;
		case __NR_ioperm:	return LNX_IOPERM;
		case __NR_socketcall:	return LNX_SOCKETCALL;
		case __NR_syslog:	return LNX_SYSLOG;
		case __NR_setitimer:	return LNX_SETITIMER;
		case __NR_getitimer:	return LNX_GETITIMER;
		case __NR_stat:		return LNX_STAT;
		case __NR_lstat:	return LNX_LSTAT;
		case __NR_fstat:	return LNX_FSTAT;
		case __NR_iopl:		return LNX_IOPL;
		case __NR_vhangup:	return LNX_VHANGUP;
		case __NR_idle:		return LNX_IDLE;
		case __NR_vm86:		return LNX_VM86;
		case __NR_wait4:	return LNX_WAIT4;
		case __NR_swapoff:	return LNX_SWAPOFF;
		case __NR_sysinfo:	return LNX_SYSINFO;
		case __NR_ipc:		return LNX_IPC;
		case __NR_fsync:	return LNX_FSYNC;
		case __NR_sigreturn:	return LNX_SIGRETURN;
		case __NR_clone:	return LNX_CLONE;
		case __NR_setdomainname: return LNX_SETDOMAINNAME;
		case __NR_uname:	return LNX_UNAME;
		case __NR_modify_ldt:	return LNX_MODIFY_LDT;
		case __NR_adjtimex:	return LNX_ADJTIMEX;
		case __NR_mprotect:	return LNX_MPROTECT;
		case __NR_sigprocmask:	return LNX_SIGPROCMASK;
		case __NR_create_module: return LNX_CREATE_MODULE;
		case __NR_init_module:	return LNX_INIT_MODULE;
		case __NR_delete_module: return LNX_DELETE_MODULE;
		case __NR_get_kernel_syms: return LNX_GET_KERNEL_SYMS;
		case __NR_quotactl:	return LNX_QUOTACTL;
		case __NR_getpgid:	return LNX_GETPGID;
		case __NR_fchdir:	return LNX_FCHDIR;
		case __NR_bdflush:	return LNX_BDFLUSH;
		case __NR_sysfs:	return LNX_SYSFS;
		case __NR_personality:	return LNX_PERSONALITY;
		case __NR_afs_syscall:	return LNX_AFS_SYSCALL;
		case __NR_setfsuid:	return LNX_SETFSUID;
		case __NR_setfsgid:	return LNX_SETFSGID;
		case __NR__llseek:	return LNX_LLSEEK;
		case __NR_getdents:	return LNX_GETDENTS;
		case __NR__newselect:	return LNX_NEWSELECT;
		case __NR_flock:	return LNX_FLOCK;
		case __NR_msync:	return LNX_MSYNC;
		case __NR_readv:	return LNX_READV;
		case __NR_writev:	return LNX_WRITEV;
		case __NR_cacheflush:	return LNX_CACHEFLUSH;
		case __NR_cachectl:	return LNX_CACHECTL;
		case __NR_sysmips:	return LNX_SYSMIPS;
		case __NR_getsid:	return LNX_GETSID;
		case __NR_fdatasync:	return LNX_FDATASYNC;
		case __NR__sysctl:	return LNX_SYSCTL;
		case __NR_mlock:	return LNX_MLOCK;
		case __NR_munlock:	return LNX_MUNLOCK;
		case __NR_mlockall:	return LNX_MLOCKALL;
		case __NR_munlockall:	return LNX_MUNLOCKALL;
		case __NR_sched_setparam: return LNX_SCHED_SETPARAM;
		case __NR_sched_getparam: return LNX_SCHED_GETPARAM;
		case __NR_sched_setscheduler: return LNX_SCHED_SETSCHEDULER;
		case __NR_sched_getscheduler: return LNX_SCHED_GETSCHEDULER;
		case __NR_sched_yield:	return LNX_YIELD;
		case __NR_sched_get_priority_max: return LNX_SCHED_GET_PRIORITY_MAX;
		case __NR_sched_get_priority_min: return LNX_SCHED_GET_PRIORITY_MIN;
		case __NR_sched_rr_get_interval: return LNX_SCHED_RR_GET_INTERVAL;
		case __NR_nanosleep:	return LNX_NANOSLEEP;
		case __NR_mremap:	return LNX_MREMAP;
		case __NR_accept:	return LNX_ACCEPT;
		case __NR_bind:		return LNX_BIND;
		case __NR_connect:	return LNX_CONNECT;
		case __NR_getpeername:	return LNX_GETPEERNAME;
		case __NR_getsockname:	return LNX_GETSOCKNAME;
		case __NR_getsockopt:	return LNX_GETSOCKOPT;
		case __NR_listen:	return LNX_LISTEN;
		case __NR_recv:		return LNX_RECV;
		case __NR_recvfrom:	return LNX_RECVFROM;
		case __NR_recvmsg:	return LNX_RECVMSG;
		case __NR_send:		return LNX_SEND;
		case __NR_sendmsg:	return LNX_SENDMSG;
		case __NR_sendto:	return LNX_SENDTO;
		case __NR_setsockopt:	return LNX_SETSOCKOPT;
		case __NR_shutdown:	return LNX_SHUTDOWN;
		case __NR_socket:	return LNX_SOCKET;
		case __NR_socketpair:	return LNX_SOCKETPAIR;
		case __NR_setresuid:	return LNX_SETRESUID;
		case __NR_getresuid:	return LNX_GETRESUID;
		case __NR_query_module: return LNX_QUERY_MODULE;
		case __NR_poll:		return LNX_POLL;
		case __NR_nfsservctl:	return LNX_NFSSERVCTL;
		case __NR_setresgid:	return LNX_SETRESGID;
		case __NR_getresgid:	return LNX_GETRESGID;
		case __NR_prctl:	return LNX_PRCTL;
		case __NR_rt_sigreturn: return LNX_RT_SIGRETURN;
		case __NR_rt_sigaction: return LNX_RT_SIGACTION;
		case __NR_rt_sigprocmask: return LNX_RT_SIGPROCMASK;
		case __NR_rt_sigpending: return LNX_RT_SIGPENDING;
		case __NR_rt_sigtimedwait: return LNX_RT_SIGTIMEDWAIT;
		case __NR_rt_sigqueueinfo: return LNX_RT_SIGQUEUEINFO;
		case __NR_rt_sigsuspend: return LNX_RT_SIGSUSPEND;
		case __NR_pread64:	return LNX_PREAD64;
		case __NR_pwrite64:	return LNX_PWRITE64;
		case __NR_chown:	return LNX_CHOWN;
		case __NR_capget:	return LNX_CAPGET;
		case __NR_capset:	return LNX_CAPSET;
		case __NR_sigaltstack:	return LNX_SIGALTSTACK;
		case __NR_sendfile:	return LNX_SENDFILE;
		case __NR_getpmsg:	return LNX_GETPMSG;
		case __NR_putpmsg:	return LNX_PUTPMSG;
		case __NR_mmap2:	return LNX_MMAP2;
		case __NR_truncate64:	return LNX_TRUNCATE64;
		case __NR_ftruncate64:	return LNX_FTRUNCATE64;
		case __NR_stat64:	return LNX_STAT64;
		case __NR_lstat64:	return LNX_LSTAT64;
		case __NR_fstat64:	return LNX_FSTAT64;
		case __NR_pivot_root:	return LNX_PIVOT_ROOT;
		case __NR_mincore:	return LNX_MINCORE;
		case __NR_madvise:	return LNX_MADVISE;
		case __NR_getdents64:	return LNX_GETDENTS64;
		case __NR_fcntl64:	return LNX_FCNTL64;
		default: return -1;
	}

	return -1;
}

unsigned long int get_svc_arg(void *sp,int arg) {
	unsigned long int a;
	if (arg<4) {
		a=((unsigned long int *)sp)[4+arg];
	} else {
		struct fullframe *ff=(struct fullframe *)sp;
		unsigned long int *a_stack=(unsigned long int *)ff->sp;
		a=a_stack[arg];
	}
//	sys_printf("get_svc_arg: arg %x is %x\n", arg, a);
	return a;
}

void set_svc_ret(void *sp, long int val)  {
	struct fullframe *ff=(struct fullframe *)sp;
	ff->v0=val;
}

void set_svc_lret(void *sp, long int val)  {
	struct fullframe *ff=(struct fullframe *)sp;
	ff->a3=val;
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
			char **argv,
			char **envp) {
//	unsigned long int *stackp=(unsigned long int *)(((char *)t)+4096);
	unsigned long int *stackp=(unsigned long int *)stackp_v;
	unsigned long int v_stack;
	int nr_args;
	char **argv_new;
	char *args_storage;
	int  args_storage_size;

	DEBUGP(DSYS_SCHED,DLEV_INFO, "t at %x, stackp at %x\n",
			t, stackp);
	if (t->asp->ref==1) {
		v_stack = 0x80000000;
	} else {
		unsigned long int brk=(t->asp->brk-1)>>PAGE_SHIFT;
		brk+=2;
		t->asp->brk=brk<<PAGE_SHIFT;
		v_stack = t->asp->brk;
	}

	nr_args=array_size(argv);
	if (nr_args>=20) {
		return -1;
	}

	args_storage_size=args_size(argv);

	// allocate space on process stack
	v_stack-=args_storage_size;			// arguments
	v_stack=v_stack&(~7);
	args_storage=(char *)v_stack;

	v_stack-=((nr_args+1)*sizeof(unsigned long int)); // argument pointers

	v_stack=v_stack&(~7);	// stack aligned at 8, missaligned long longs give ugly printouts
	argv_new=(char **)v_stack;

	curr_pgd=t->asp->pgd;  /* repoint page table directory while loading */
	copy_arguments(argv_new, argv, args_storage, nr_args);
	argv_new[nr_args]=0;
	curr_pgd=current->asp->pgd;

	*(--stackp)=0;		// hi
	*(--stackp)=0;		// lo

	*(--stackp)=fnc;	// epc
	*(--stackp)=0x04008000;	// cause

	*(--stackp)=0xfc13;	// status
	*(--stackp)=0;		// ra
	*(--stackp)=0;		// fp
	*(--stackp)=v_stack; // user sp

	*(--stackp)=0;		// gp
	*(--stackp)=0;		// k1
	*(--stackp)=0;		// k0
	*(--stackp)=0;		// t9
	*(--stackp)=0;		// t8
	*(--stackp)=0;		// s7
	*(--stackp)=0;		// s6
	*(--stackp)=0;		// s5

	*(--stackp)=0;		// s4
	*(--stackp)=0;		// s3
	*(--stackp)=0;		// s2
	*(--stackp)=0;		// s1
	*(--stackp)=0;		// s0
	*(--stackp)=0;		// t7
	*(--stackp)=0;		// t6
	*(--stackp)=0;		// t5

	*(--stackp)=0;		// t4
	*(--stackp)=0;		// t3

	*(--stackp)=0;		// t2
	*(--stackp)=0;		// t1

	*(--stackp)=0;		// t0

	*(--stackp)=0;		// a3
	*(--stackp)=0;		// a2
	*(--stackp)=(long unsigned int)argv_new;// a1
	*(--stackp)=(long unsigned int)nr_args; // a0

//	*(--stackp)=0x0000ff10; // a0
//	*(--stackp)=(unsigned long int)arg0; // a0
	*(--stackp)=0x00000000; // v1
	*(--stackp)=fnc; // v0

	*(--stackp)=0;		// at
	*(--stackp)=0;		// zero

	t->sp=stackp;	// kernel sp
}

void adjust_stackptr(struct task *t) {
	register unsigned long int sp=(unsigned long int)t->sp;
	__asm__ __volatile__("move $sp,%0\n\t"
				:
				: "r" (sp));
}

unsigned long int get_usr_pc(struct task *t) {
	// tbd
	return 0;
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

unsigned long int get_mmap_vaddr(struct task *current, unsigned int size) {
	int psize=(((size-1)/PAGE_SIZE)+1)*PAGE_SIZE;
	current->asp->mmap_vaddr-=psize;
	return current->asp->mmap_vaddr;
};
