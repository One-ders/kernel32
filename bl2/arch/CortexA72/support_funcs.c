
#include <sys.h>
#include <frame.h>
#include <system_params.h>

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
	*(--stackp)=(unsigned long int)ret_fnc;//svc_destroy_self;     // r14
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

unsigned long int get_usr_pc(struct task *t) {
	unsigned long int *estackp=(unsigned long int *)t->estack;
	unsigned long int *stackp=t->sp;
	unsigned long int *bstackp=estackp+512;
	unsigned long int rval;
	unsigned long int cpu_flags;

	// We need to map in the stack page of the process we want to read.
	// It is mpu protected while inactive.
	// Also shut off irqs to prevent rescheduling, if we would
	// get switched out, the aux stack page will not be present
	// at switch in.
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
	return 0;
}

