
#include <sys.h>
#include <frame.h>

unsigned int get_svc_number(void *sp) {
	return (((char *)((unsigned long int *)sp)[6])[-2]);
}

unsigned long int get_svc_arg(void *svc_sp, int arg) {
	return ((unsigned long int *)svc_sp)[arg];
}

void set_svc_ret(void *svc_sp, long int val) {
	((unsigned long int *)svc_sp)[0]=val;
}

void setup_return_stack(struct task *t, void *stackp_v,
					unsigned long int fnc,
					unsigned long int ret_fnc,
					void *arg0,
					void *arg1) {
	unsigned long int *stackp=(unsigned long int *)stackp_v;

	*(--stackp)=0x01000000;                  // xPSR
	*(--stackp)=(unsigned long int)fnc;    // r15
	*(--stackp)=(unsigned long int)ret_fnc;//svc_kill_self;     // r14
	*(--stackp)=0;     // r12
	*(--stackp)=0;     // r3
	*(--stackp)=0;     // r2
	*(--stackp)=(unsigned long int)arg1;    // r1
	*(--stackp)=(unsigned long int)arg0;    // r0
////
	*(--stackp)=0;     // r4
	*(--stackp)=0;     // r5
	*(--stackp)=0;     // r6
	*(--stackp)=0;     // r7
	*(--stackp)=0;     // r8
	*(--stackp)=0;     // r9
	*(--stackp)=0;     // r10
	*(--stackp)=0;     // r11

	t->sp=stackp;

}

unsigned long int get_stacked_pc(struct task *t) {
	return ((struct fullframe *)t->sp)->r15;
}
