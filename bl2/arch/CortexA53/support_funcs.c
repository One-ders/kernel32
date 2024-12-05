
#include <sys.h>
#include <frame.h>
#include <string.h>

//#include <system_params.h>

unsigned int get_svc_number(void *sp) {
	return (((char *)((unsigned long int *)sp)[6])[-2]);
}

unsigned long int get_svc_arg(void *svc_sp, int arg) {
	return ((unsigned long int *)svc_sp)[arg];
}

void set_svc_ret(void *svc_sp, long int val) {
//	((unsigned long int *)svc_sp)[0]=val;
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
	unsigned long int *stackp=(unsigned long int *)stackp_v;

	*(--stackp)=0x01000000;                  // xPSR
	*(--stackp)=(unsigned long int)fnc;    // r15
	*(--stackp)=(unsigned long int)ret_fnc;//svc_destroy_self;     // r14
	*(--stackp)=0;     // x12
	*(--stackp)=0;     // x3
	*(--stackp)=0;     // x2
	*(--stackp)=(unsigned long int)arg1;    // x1
	*(--stackp)=(unsigned long int)arg0;    // x0
////
	*(--stackp)=0;     // x4
	*(--stackp)=0;     // x5
	*(--stackp)=0;     // x6
	*(--stackp)=0;     // x7
	*(--stackp)=0;     // x8
	*(--stackp)=0;     // x9
	*(--stackp)=0;     // x10
	*(--stackp)=0;     // x11

	t->sp=stackp;

	return 0;
}

unsigned long int get_stacked_pc(struct task *t) {
	return ((struct fullframe *)t->sp)->x15;
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

