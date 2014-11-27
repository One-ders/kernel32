#include <stm32f407.h>
#include <core_cm4.h>
#include <core_cmInstr.h>
#include <sys.h>
#include <io.h>

#define MPU_CTRL_ENABLE		1
#define MPU_CTRL_HFNMIENA	2
#define MPU_CTRL_PRIVDEFENA	4

#define MPU_RBAR_VALID		0x10

#define AP_FULL	(0x3<<24)
#define AP_NONE (0)
#define CACHE_MEM (0xd<<16)

struct MPU {
	unsigned int TYPE;
	volatile unsigned int CTRL;
	volatile unsigned int RNR;
	volatile unsigned int RBAR;
	volatile unsigned int RASR;
	struct ALIAS {
		volatile unsigned int RBAR;
		volatile unsigned int RASR;
	} ALIAS[3];
};

#define MPU	((struct MPU *)0xe000ed90)

int init_memory_protection(void) {
	sys_printf("mpu type: %x\n",MPU->TYPE);	
	return 0;
}

int map_stack_page(unsigned long int addr, unsigned int size) {
	MPU->RBAR=addr|MPU_RBAR_VALID|1;
	MPU->RASR=AP_FULL|CACHE_MEM|(size<<1)|1;
	__DMB();
	return 0;
}

int map_tmp_stack_page(unsigned long int addr, unsigned int size) {
	MPU->RBAR=addr|MPU_RBAR_VALID|2;
	MPU->RASR=AP_FULL|CACHE_MEM|(size<<1)|1;
	__DMB();
	return 0;
}

int map_next_stack_page(unsigned long int new_addr, unsigned long int old_addr) {
	/* got an interrupt between the two RBAR updates */
	/* new address valid map, old address not valid until */
	/* RASR is updated, gives exception stacking error */
	/* current stack is still on old addr */
	disable_interrupts();
	MPU->RBAR=new_addr|MPU_RBAR_VALID|1;
	MPU->RBAR=old_addr|MPU_RBAR_VALID|2;
	MPU->RASR=AP_FULL|CACHE_MEM|(9<<1)|1;
	enable_interrupts();
	__DMB();
	return 0;
}

int unmap_tmp_stack_page() {
//	MPU->RBAR=addr|MPU_RBAR_VALID|2;
	MPU->RNR=2;
	MPU->RASR=AP_NONE|CACHE_MEM|(9<<1)|1;
	__DMB();
	return 0;
}



int unmap_stack_memory(unsigned long int addr) {
	MPU->RBAR=addr|(MPU_RBAR_VALID|0);
	MPU->RASR=0x1d;  /* map 16K mem non access */
	__DMB();
	return 0;
}

int activate_memory_protection(void) {
	MPU->CTRL=MPU_CTRL_ENABLE|MPU_CTRL_PRIVDEFENA;	
	__DMB();
	return 0;
}
