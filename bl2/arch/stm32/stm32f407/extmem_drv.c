
#include <sys.h>
#include <string.h>
#include <stm32f407.h>
#include <devices.h>

#include <gpio_drv.h>

#ifdef MB1075B

static struct device_handle my_dh;

extern struct driver_ops gpio_drv_ops;

static int fmc_sram_open(void *instance, DRV_CBH cb_handler, void *dum) {
	return -1;
} 

static int fmc_sram_control(struct device_handle *dh, int cmd, void *arg1, int arg2) {
	return -1;
}

static int fmc_sram_close(struct device_handle *dh) {
	return 0;
}

static int fmc_sram_start(void *inst) {
	return 0;
}

// Mem at 90 Mhz = (180/2)
// Timings
// SDTR.TMRD: 1-16,  2 cc
#define LoadToActiveDelay	2
// SDTR.TXSR: min 70ns (7x11.11ns) 1-16 (v-1)<<4
#define ExitSelfRefreshDelay	7
// SDTR.TRAS: min 42, max 120000 ns (4x11.11ns), 1-16 -> (v-1)<<8
#define SelfRefreshTime		4
// SDTR.TRC: min 70 (7x11.11ns), 1-16 -> (v-1)<<12
#define RowCycleDelay		7
// SDTR.TWR: min 1+7ns	(1+1x11.11ns), 1-16 -> (v-1)<<16
#define WriteRecoveryTime	2
// SDTR.TRP:  20ns (2x11.11ns), 1-16 -> (v-1)<<20
#define RPDelay			2
// SDTR.TRCD: 20ns (2x11.11ns), 1-16 -> (v-1)<<24
#define RCDelay			2

// Controll config
// SDRAM Bank 0 or 1
#define Bank			0
// SDRC1,2 Col addressing: [7:0], value 8-11 is col-8
#define ColumnBits		8
// SDRC1,2 Row addressing: [11:0], value is 11-13 -> (v-11)<<2
#define RowBits			12
// SDRC1,2 MWID 8,16,32 -> (v>>4)<<4)
#define MemDataWidth		16
// SDRC1,2 NB 2 or 4 -> (v>>2)<<6
#define InternalBank		4
// SDRC1,2 0,1,2,3 cycles,  v<<7
#define CASLatency		3
// SDRC1,2 WP  0 or 1 v<<9
#define WriteProtect		0
// SDRC1 00=disable, 2=2xhclk or 3=3xhclk
#define SDClockPeriod		2
// SDRC1 readburst disable 0 or 1  -> v<<12
#define ReadBurst		0
// SDRC1 0,1 or 2 -> v<<13
#define ReadPipeDelay		1

static int fmc_sram_init(void *inst) {
	struct device_handle *gpio_dh;
//	struct driver *gpio_drv;
	struct pin_spec ps[38];
	int rc;
	unsigned int tmpreg;
	unsigned int tout;
	unsigned int flags;

//	gpio_drv=driver_lookup(GPIO_DRV);
//	if (!gpio_drv) return 0;

	gpio_drv_ops.init(0);

	gpio_dh=gpio_drv_ops.open(0,0,0);

	flags=GPIO_DIR(0,GPIO_ALTFN_PIN);
//	flags=GPIO_SPEED(flags,GPIO_SPEED_HIGH);
	flags=GPIO_SPEED(flags,GPIO_SPEED_FAST);
	flags=GPIO_DRIVE(flags,GPIO_PUSHPULL);
	flags=GPIO_ALTFN(flags,0xc);

	ps[0].pin=FMC_A0;
	ps[0].flags=flags;
	ps[1].pin=FMC_A1;
	ps[1].flags=flags;
	ps[2].pin=FMC_A2;
	ps[2].flags=flags;
	ps[3].pin=FMC_A3;
	ps[3].flags=flags;
	ps[4].pin=FMC_A4;
	ps[4].flags=flags;
	ps[5].pin=FMC_A5;
	ps[5].flags=flags;
	ps[6].pin=FMC_A6;
	ps[6].flags=flags;
	ps[7].pin=FMC_A7;
	ps[7].flags=flags;
	ps[8].pin=FMC_A8;
	ps[8].flags=flags;
	ps[9].pin=FMC_A9;
	ps[9].flags=flags;
	ps[10].pin=FMC_A10;
	ps[10].flags=flags;
	ps[11].pin=FMC_A11;
	ps[11].flags=flags;
	/*********************/
	ps[12].pin=FMC_D0;
	ps[12].flags=flags;
	ps[13].pin=FMC_D1;
	ps[13].flags=flags;
	ps[14].pin=FMC_D2;
	ps[14].flags=flags;
	ps[15].pin=FMC_D3;
	ps[15].flags=flags;
	ps[16].pin=FMC_D4;
	ps[16].flags=flags;
	ps[17].pin=FMC_D5;
	ps[17].flags=flags;
	ps[18].pin=FMC_D6;
	ps[18].flags=flags;
	ps[19].pin=FMC_D7;
	ps[19].flags=flags;
	ps[20].pin=FMC_D8;
	ps[20].flags=flags;
	ps[21].pin=FMC_D9;
	ps[21].flags=flags;
	ps[22].pin=FMC_D10;
	ps[22].flags=flags;
	ps[23].pin=FMC_D11;
	ps[23].flags=flags;
	ps[24].pin=FMC_D12;
	ps[24].flags=flags;
	ps[25].pin=FMC_D13;
	ps[25].flags=flags;
	ps[26].pin=FMC_D14;
	ps[26].flags=flags;
	ps[27].pin=FMC_D15;
	ps[27].flags=flags;
	/**************************/
	ps[28].pin=FMC_BA0;
	ps[28].flags=flags;
	ps[29].pin=FMC_BA1;
	ps[29].flags=flags;
	ps[30].pin=FMC_NBL0;
	ps[30].flags=flags;
	ps[31].pin=FMC_NBL1;
	ps[31].flags=flags;
	ps[32].pin=FMC_SDCLK;
	ps[32].flags=flags;
	ps[33].pin=FMC_SDNWE;
	ps[33].flags=flags;
	ps[34].pin=FMC_SDNRAS;
	ps[34].flags=flags;
	ps[35].pin=FMC_SDNCAS;
	ps[35].flags=flags;
	ps[36].pin=FMC_SDNE1;
	ps[36].flags=flags;
	ps[37].pin=FMC_SDCKE1;
	ps[37].flags=flags;

	rc=gpio_drv_ops.control(gpio_dh,GPIO_BUS_ASSIGN_PINS,ps,sizeof(ps));
	if (rc<0) return -1;

	/* */
	RCC->AHB3ENR |= RCC_AHB3ENR_FMCEN;	
	sys_udelay(100);

	/* Config and enable SDRAM bank1 */
#if 0
	FMC->SDCR_1=(1<<FMC_SDCR_RPIPE_SHIFT)|(2<<FMC_SDCR_SDCLK_SHIFT)|
			(3<<FMC_SDCR_CAS_SHIFT)|FMC_SDCR_NB|
			(1<<FMC_SDCR_MWID_SHIFT);
#endif
	FMC->SDCR_1=(ReadPipeDelay<<FMC_SDCR_RPIPE_SHIFT)|
			(SDClockPeriod<<FMC_SDCR_SDCLK_SHIFT)|
			(CASLatency<<FMC_SDCR_CAS_SHIFT)|
			((InternalBank>>2)<<FMC_SDCR_NB_SHIFT)|
			((MemDataWidth>>4)<<FMC_SDCR_MWID_SHIFT)|
			((RowBits-11)<<FMC_SDCR_NR_SHIFT)|
			(ColumnBits-8);

	FMC->SDCR_2=(ReadPipeDelay<<FMC_SDCR_RPIPE_SHIFT)|
			(SDClockPeriod<<FMC_SDCR_SDCLK_SHIFT)|
			(CASLatency<<FMC_SDCR_CAS_SHIFT)|
			((InternalBank>>2)<<FMC_SDCR_NB_SHIFT)|
			((MemDataWidth>>4)<<FMC_SDCR_MWID_SHIFT)|
			((RowBits-11)<<FMC_SDCR_NR_SHIFT)|
			(ColumnBits-8);


#if 0
	FMC->SDTR1=(1<<FMC_SDTR_TRCD_SHIFT)|(1<<FMC_SDTR_TRP_SHIFT)|
		(1<<FMC_SDTR_TWR_SHIFT)|(5<<FMC_SDTR_TRC_SHIFT)|
		(3<<FMC_SDTR_TRAS_SHIFT)|(5<<FMC_SDTR_TXSR_SHIFT)|
		(1<<FMC_SDTR_TMRD_SHIFT);
#endif
	FMC->SDTR1=((RCDelay-1)<<FMC_SDTR_TRCD_SHIFT)|
		((RPDelay-1)<<FMC_SDTR_TRP_SHIFT)|
		((WriteRecoveryTime-1)<<FMC_SDTR_TWR_SHIFT)|
		((RowCycleDelay-1)<<FMC_SDTR_TRC_SHIFT)|
		((SelfRefreshTime-1)<<FMC_SDTR_TRAS_SHIFT)|
		((ExitSelfRefreshDelay-1)<<FMC_SDTR_TXSR_SHIFT)|
		((LoadToActiveDelay-1)<<FMC_SDTR_TMRD_SHIFT);

	FMC->SDTR2=((RCDelay-1)<<FMC_SDTR_TRCD_SHIFT)|
		((RPDelay-1)<<FMC_SDTR_TRP_SHIFT)|
		((WriteRecoveryTime-1)<<FMC_SDTR_TWR_SHIFT)|
		((RowCycleDelay-1)<<FMC_SDTR_TRC_SHIFT)|
		((SelfRefreshTime-1)<<FMC_SDTR_TRAS_SHIFT)|
		((ExitSelfRefreshDelay-1)<<FMC_SDTR_TXSR_SHIFT)|
		((LoadToActiveDelay-1)<<FMC_SDTR_TMRD_SHIFT);


	FMC->SDCMR=FMC_SDCMR_CTB2|(1<<FMC_SDCMR_MODE_SHIFT); // Clock conf ena

	tout=0xffff;
	tmpreg=FMC->SDSR&FMC_SDSR_BUSY;
	while(tmpreg&&(tout--)) {
		tmpreg=FMC->SDSR&FMC_SDSR_BUSY;
	}

	for(tout=0;tout<1000;tout++);

	FMC->SDCMR=FMC_SDCMR_CTB2|(2<<FMC_SDCMR_MODE_SHIFT); // PALL
	tout=0xffff;
	tmpreg=FMC->SDSR&FMC_SDSR_BUSY;
	while(tmpreg&&(tout--)) {
		tmpreg=FMC->SDSR&FMC_SDSR_BUSY;
	}

	FMC->SDCMR=(3<<FMC_SDCMR_NRFS_SHIFT)|FMC_SDCMR_CTB2|
			(3<<FMC_SDCMR_MODE_SHIFT); // Auto refresh
	tout=0xffff;
	tmpreg=FMC->SDSR&FMC_SDSR_BUSY;
	while(tmpreg&&(tout--)) {
		tmpreg=FMC->SDSR&FMC_SDSR_BUSY;
	}

	FMC->SDCMR=(3<<FMC_SDCMR_NRFS_SHIFT)|FMC_SDCMR_CTB2|
			(3<<FMC_SDCMR_MODE_SHIFT); // Auto refresh
	tout=0xffff;
	tmpreg=FMC->SDSR&FMC_SDSR_BUSY;
	while(tmpreg&&(tout--)) {
		tmpreg=FMC->SDSR&FMC_SDSR_BUSY;
	}


	FMC->SDCMR=(0x231<<FMC_SDCMR_MRD_SHIFT)|FMC_SDCMR_CTB2|
			(4<<FMC_SDCMR_MODE_SHIFT); // load mode reg
	tout=0xffff;
	tmpreg=FMC->SDSR&FMC_SDSR_BUSY;
	while(tmpreg&&(tout--)) {
		tmpreg=FMC->SDSR&FMC_SDSR_BUSY;
	}
	
//	FMC->SDRTR|=(0x27c<<FMC_SDRTR_COUNT_SHIFT);
	FMC->SDRTR=(0x56a<<FMC_SDRTR_COUNT_SHIFT);

	FMC->SDCR_2&=~FMC_SDCR_WP;

	return 0;
}

static struct driver_ops fmc_sram_ops = {
	fmc_sram_open,
	fmc_sram_close,
	fmc_sram_control,
	fmc_sram_init,
	fmc_sram_start,
};

static struct driver fmc_sram_drv = {
	"fmc_sram",
	0,
	&fmc_sram_ops,
};

void init_fmc_sram(void) {
	driver_publish(&fmc_sram_drv);
}

INIT_FUNC(init_fmc_sram);

#endif
