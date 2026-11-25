
#include <io.h>
#include <sys.h>
#include <frame.h>
#include <string.h>
#include <stat.h>
#include <yaffsfs.h>

#include "sys_const.h"

#include "syscall.h.in" // musl linux syscalls

//#include <system_params.h>
int map_page(unsigned long *pgd, unsigned long vaddr, unsigned long phys_addr, unsigned long flags);

static int lnx_sys_map(unsigned int lnx_scall) {

	switch(lnx_scall) {
		case __NR_dup:		return LNX_DUP;
		case __NR_fcntl:	return LNX_FCNTL64;
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
		case __NR_openat:	return LNX_OPENAT;
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

int get_svc_number(void *sp, unsigned int ereg, unsigned int *domain) {
	unsigned long int *sp_ptr=(unsigned long int *)sp;
	unsigned int svc_no=ereg&0xffff; // svc no, stored in SR_EL1

	// check svc number: 0 is Musl linux
	// 			else Native

	if (svc_no==0) {
		int mapped_svc=lnx_sys_map(sp_ptr[8]);
		*domain=SYSCALL_LNX;
		if (mapped_svc<0) {
			io_setpolled(1);
			sys_printf("no map for lnx svc 0x%x\n", sp_ptr[8]);
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
	return ((unsigned long int *)svc_sp)[arg];
}

void set_svc_ret(void *svc_sp, long int val) {
	((unsigned long int *)svc_sp)[0]=val;
}

void set_svc_lret(void *sp, long int val)  {
//        struct fullframe *ff=(struct fullframe *)sp;
//        ff->a3=val;
}

struct aarch64_stat {
	unsigned long st_dev;        // 0x00 (8 bytes)
	unsigned long st_ino;        // 0x08 (8 bytes)
	unsigned int  st_mode;       // 0x10 (4 bytes)
	unsigned int  st_nlink;      // 0x14 (4 bytes)
	unsigned int  st_uid;        // 0x18 (4 bytes)
	unsigned int  st_gid;        // 0x1c (4 bytes)
	unsigned long st_rdev;       // 0x20 (8 bytes)
	unsigned long __pad1;        // 0x28 (8 bytes)
	long          st_size;       // 0x30 (8 bytes)
	int           st_blksize;    // 0x38 (4 bytes)
	int           __pad2;        // 0x3c (4 bytes)
	long          st_blocks;     // 0x40 (8 bytes)
	long          st_atime_sec;  // 0x48 (8 bytes)
	long          st_atime_nsec; // 0x50 (8 bytes)
	long          st_mtime_sec;  // 0x58 (8 bytes)
	long          st_mtime_nsec; // 0x60 (8 bytes)
	long          st_ctime_sec;  // 0x68 (8 bytes)
	long          st_ctime_nsec; // 0x70 (8 bytes)
	unsigned int  __unused;   // 0x78 (8 bytes)
};                               // Totalt: 128 bytes (0x80)

int arch_lnx_newfstatat(unsigned long int *svc_sp) {
	int dirfd=get_svc_arg(svc_sp,0);
        char *path=(char *)get_svc_arg(svc_sp,1);
        struct aarch64_stat *user_stbuf=(struct aarch64_stat *)get_svc_arg(svc_sp,2);
        struct stat stbuf;
        int rc;

	if (dirfd!=-100) {
		set_svc_ret(svc_sp,-1);
		set_svc_lret(svc_sp,0);
		return 0;
	}

	memset(&stbuf,0,sizeof(stbuf));
	rc=sys_lstat(path,&stbuf);
	if (rc<0) {
		set_svc_ret(svc_sp, -yaffsfs_GetLastError());
		set_svc_lret(svc_sp,-1);
	} else {
		user_stbuf->st_dev       = stbuf.st_dev;
		user_stbuf->st_ino       = stbuf.st_ino;
		user_stbuf->st_mode      = stbuf.st_mode;
		user_stbuf->st_nlink     = stbuf.st_nlink;
		user_stbuf->st_uid       = stbuf.st_uid;
		user_stbuf->st_gid       = stbuf.st_gid;
		user_stbuf->st_rdev      = stbuf.st_rdev;
		user_stbuf->st_size      = stbuf.st_size;
		user_stbuf->st_blksize   = stbuf.st_blksize;
		user_stbuf->st_blocks    = stbuf.st_blocks;
		user_stbuf->st_atime_sec = stbuf.st_atim.tv_sec;
		user_stbuf->st_mtime_sec = stbuf.st_mtim.tv_sec;
		user_stbuf->st_ctime_sec = stbuf.st_ctim.tv_sec;

		set_svc_ret(svc_sp,rc);
		set_svc_lret(svc_sp,0);
	}
	return 0;
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

int env_size(char **envp) {
        int total_size=0;
        int i=0;

        while(envp[i]) {
                total_size+=(strlen(envp[i])+1);
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

int copy_env(char **envp_new, char **envp,
		char *envp_storage, int nr_envs) {
	int i=0;
	char *to=envp_storage;
	while(envp[i]) {
		envp_new[i]=strcpy(to,envp[i]);
                to+=strlen(envp[i]);
                *to=0;
                to++;
                i++;
        }
        return to-envp_storage;
}

int setup_return_stack(struct task *t, void *stackp_v,
			unsigned long int fnc, unsigned long int ret_fnc,
			char **argv, char **envp) {
	unsigned long int stacktop=(unsigned long int)stackp_v;
	t->sp=(void *)stacktop-S_FRAME_SIZE;
	unsigned long int *stackp=(unsigned long int *)t->sp;
	unsigned long int v_stack;
	int nr_args;
	char **argv_new;
	char *args_storage;
	int  args_storage_size;


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
	v_stack-=args_storage_size;                     // arguments
	v_stack=v_stack&(~7);
	args_storage=(char *)v_stack;

	v_stack-=((nr_args+1)*sizeof(unsigned long int)); // argument pointers

	v_stack=v_stack&(~7);   // stack aligned at 8, missaligned long longs give ugly printouts
	argv_new=(char **)v_stack;



	stackp[0]=(unsigned long int)nr_args;    // x0
	stackp[1]=(unsigned long int)argv_new;    // x1
	stackp[2]=0;	// x2
	stackp[3]=0;	// x3
	stackp[4]=0;	// x4
	stackp[5]=0;	// x5
	stackp[6]=0;	// x6
	stackp[7]=0;	// x7
	stackp[8]=0;	// x8
	stackp[9]=0;	// x9
	stackp[10]=0;	// x10
	stackp[11]=0;	// x11
	stackp[12]=0;	// x12
	stackp[13]=0;	// x13
	stackp[14]=0;	// x14
	stackp[15]=0;	// x15
	stackp[16]=0;	// x16
	stackp[17]=0;	// x17
	stackp[18]=0;	// x18
	stackp[19]=0;	// x19
	stackp[20]=0;	// x20
	stackp[21]=0;	// x21
	stackp[22]=0;	// x22
	stackp[23]=0;	// x23
	stackp[24]=0;	// x24
	stackp[25]=0;	// x25
	stackp[26]=0;	// x26
	stackp[27]=0;	// x27
	stackp[28]=0;	// x28
	stackp[29]=0;	// x29
	stackp[30]=(unsigned long int)ret_fnc;//svc_destroy_self; // x30
	stackp[31]=v_stack;    // user stack
	stackp[32]=(unsigned long int)fnc;    // elr_el1
	stackp[33]=0x00000000;	//user mode           // spsr_el1
	stackp[34] = 0;		// initial irq_lev = 0
	stackp[35] = 0;		// padding för 16-byte alignment

	return 0;
}

void *handle_syscall(unsigned long int *sp);

void handle_trap(void *sp, unsigned long esr, unsigned long elr, unsigned long spsr, unsigned long far, unsigned long sctlr) {
	unsigned int ec=esr>>26; // exception class
	switch(ec) {
		case 0x15:
			handle_syscall(sp);
			break;
		case 0x24: /* Data Abort från EL0 (sbrk / stack / user-access) */
                case 0x25: { /* Data Abort från EL1 (load_init / copy_from_user / copy_to_user) */
			unsigned int dfsc = esr & 0x3f;
//			struct address_space *asp = active_user_asp ? active_user_asp : current->asp;
			struct address_space *asp = current->asp;
			unsigned long brk_page_end = (asp->brk + PAGE_SIZE - 1) & PAGE_MASK;
			int in_brk   = (far >= 0x400000 && far < brk_page_end);
			int in_stack = (far >= 0x7fff0000 && far < 0x80000000);

			/* Translation fault (0x04..0x07) on a valid user address (e.g. during load_init) */
			if (far < VA_START && (dfsc >= 0x04 && dfsc <= 0x07) &&
				asp && asp->pgd && (in_brk||in_stack)) {
				void *kpage = get_page();
				if (kpage) {
//					unsigned long flags = (far < 0x1000000) ? USER_TEXT_RW : USER_DATA_RW;
					memset(kpage, 0, PAGE_SIZE);
					map_page(asp->pgd, far & PAGE_MASK,
						(unsigned long)KV2P(kpage), USER_PAGE_RWX);
					asm volatile("dsb ishst\n isb\n" ::: "memory");
					break;
				}
			}

			io_setpolled(1);
			sys_printf("Data abort: far=%lx esr=%lx elr=%lx\n", far, esr, elr);
			while (1);
		}

		default:
			io_setpolled(1);
			sys_printf("unhandle trap ec=%x\n", ec);
			while(1);
			break;
	}
}

unsigned long int get_stacked_pc(struct task *t) {
	return ((struct trap_frame *)t->sp)->elr_el1;
}

unsigned long int get_usr_pc(struct task *t) {
//	unsigned long int *estackp=(unsigned long int *)t->estack;
	unsigned long int *stackp=t->sp;

	return stackp[32];
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

