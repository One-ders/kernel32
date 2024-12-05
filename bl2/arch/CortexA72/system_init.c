
#include <sys.h>
#include <string.h>
#include <stm32f407.h>
#include <devices.h>

void _exit(int status) {
}

struct task main_task = {
.name	=	"init_main",
.sp	=	(void *)(0x20020000),
.id	=	256,
.next	=	0,
.next2	=	0,
.state	=	1,
.prio_flags=	3,
.estack	=	(void *)(0x20020000-0x800)
};

extern int __usr_main(int argc, char **argv);
void *usr_init=__usr_main;

void build_free_page_list(void);
static unsigned long free_page_map[SDRAM_SIZE/(PAGE_SIZE*sizeof(unsigned long))];

extern unsigned long __bss_start__;
extern unsigned long __bss_end__;

#define SDRAM_START 0x20000000
#define PHYS2PAGE(a)    ((((unsigned long int)(a))&(SDRAM_START-1))>>12)
#define set_in_use(a,b) ((a)[((unsigned long int)(b))/32]&=~(1<<(((unsigned long int)(b))%32)))
#define set_free(a,b) ((a)[((unsigned long int)(b))/32]|=(1<<(((unsigned long int)(b))%32)))


void init_sys_arch(void) {
//	unsigned long int psize;
	build_free_page_list();
//	psize=(SDRAM_SIZE+SDRAM_START)-__bss_end__;
//	sys_printf("psize=%x\n",psize);
}

void build_free_page_list() {
	unsigned long int mptr=SDRAM_START;
	int i;

	for(i=0;i<(sizeof(free_page_map)/sizeof(unsigned long int));i++) {
		free_page_map[i]=~0;
	}

	while(mptr<((unsigned long int)&__bss_end__)) {
		set_in_use(free_page_map,PHYS2PAGE(mptr));
#if 0
		sys_printf("build_free_page_list: set %x non-free, map %x\n",mptr, free_page_map[0]);
		sys_printf("map index %x, mapmask %x\n",PHYS2PAGE(mptr)/32,
				~(1<<((PHYS2PAGE(mptr))%32)));
#endif
		mptr+=4096;
	}
}

void *get_page(void) {
	int i;
	for(i=0;i<(sizeof(free_page_map)/sizeof(unsigned long int));i++) {
		if (free_page_map[i]) {
			int j=ffsl(free_page_map[i]);
			if (!j) return 0;
			sys_printf("found free page att index %x, bit %x\n",
					i,j);
			j--;
			free_page_map[i]&=~(1<<j);
			return (void *)(SDRAM_START + (i*32*4096) + (j*4096));
		}
	}
	return 0;
}

void put_page(void *p) {
	set_free(free_page_map,PHYS2PAGE(p));
}

unsigned int  SystemCoreClock = SYS_CLK_LO;

void config_sys_tic(unsigned int ms) {
	unsigned int perSec=1000/ms;
	SysTick_Config(SystemCoreClock/perSec);
}

void board_reboot(void) {
	NVIC_SystemReset();
}
