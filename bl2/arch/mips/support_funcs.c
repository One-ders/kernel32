
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

void setup_return_stack(struct task *t, void *stackp_v,
			unsigned long int fnc,
			unsigned long int ret_fnc,
			void *arg0,
			void *arg1) {
//	unsigned long int *stackp=(unsigned long int *)(((char *)t)+4096);
	unsigned long int *stackp=(unsigned long int *)stackp_v;
	unsigned long int v_stack;

	sys_printf("t at %x, stackp at %x\n",
			t, stackp);
	if (t->asp->ref==1) {
		v_stack = 0x80000000;
	} else {
		unsigned long int brk=(t->asp->brk-1)>>PAGE_SHIFT;
		brk+=2;
		t->asp->brk=brk<<PAGE_SHIFT;
		v_stack = t->asp->brk;
	}

	curr_pgd=t->asp->pgd;  /* repoint page table directory while loading */
//	strcpy(((void *)0x3f000),arg0);
	strcpy(((void *)(v_stack-8)),arg0);
	curr_pgd=current->asp->pgd;
	
	*(--stackp)=0;		// hi
	*(--stackp)=0;		// lo

	*(--stackp)=fnc;	// epc
	*(--stackp)=0x04008000;	// cause

	*(--stackp)=0xfc13;	// status
	*(--stackp)=0;		// ra
	*(--stackp)=0;		// fp
//	*(--stackp)=0x80000000;	// user sp
	*(--stackp)=v_stack-((unsigned int)arg1); // user sp

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
	*(--stackp)=v_stack-((unsigned int)arg1);	// a3
	*(--stackp)=0;		// a2
	*(--stackp)=0;		// a1
//	*(--stackp)=(unsigned long int)arg1; // a1

	*(--stackp)=0x0000ff10; // a0
//	*(--stackp)=(unsigned long int)arg0; // a0
	*(--stackp)=0x00000000; // v1
	*(--stackp)=fnc; // v0

	*(--stackp)=0;		// at
	*(--stackp)=0;		// zero

	t->sp=stackp;	// kernel sp
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
