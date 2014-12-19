
#include <sys.h>
#include <frame.h>


extern unsigned long int *curr_pgd;


unsigned int get_svc_number(void *sp)  {
	struct fullframe *ff=(struct fullframe *)sp;
	return ff->v0;
}

unsigned long int get_svc_arg(void *sp,int arg) {
	unsigned long int a=((unsigned long int *)sp)[4+arg];
//	sys_printf("get_svc_arg: arg %x is %x\n", arg, a);
	return a;
}

void set_svc_ret(void *sp, long int val)  {
	struct fullframe *ff=(struct fullframe *)sp;
	ff->v0=val;
}

void setup_registers(struct task *t, char *arg0) {
	unsigned long int *stackp=(unsigned long int *)(((char *)t)+4096);

	sys_printf("t at %x, stackp at %x\n",
			t, stackp);

	curr_pgd=t->pgd;  /* repoint page table directory while loading */
	strcpy(((void *)0x3f000),arg0);
	curr_pgd=current->pgd;
	
	*(--stackp)=0;		// hi
	*(--stackp)=0;		// lo

	*(--stackp)=0x40000;	// epc
	*(--stackp)=0x04008000;	// cause

	*(--stackp)=0xfc13;	// status
	*(--stackp)=0;		// ra
	*(--stackp)=0;		// fp
	*(--stackp)=0x80000000;	// user sp

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
	*(--stackp)=0x80000000;	// a3
	*(--stackp)=0;		// a2
	*(--stackp)=0;		// a1

	*(--stackp)=0x0000ff10; // a0
	*(--stackp)=0x00000000; // v1
	*(--stackp)=0x00040000; // v0

	*(--stackp)=0;		// at
	*(--stackp)=0;		// zero

	t->sp=stackp;	// kernel sp
}
