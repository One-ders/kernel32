
#include <sys.h>
#include <frame.h>
#include <string.h>


extern unsigned long int *curr_pgd;


unsigned int get_svc_number(void *sp)  {
	struct fullframe *ff=(struct fullframe *)sp;
	return ff->v0;
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
