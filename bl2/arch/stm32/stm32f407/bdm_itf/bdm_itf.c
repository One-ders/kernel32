/* $Esos: , v1.1 2014/04/07 21:44:00 anders Exp $ */

/*
 * Copyright (c) 2014, Anders Franzen.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * @(#)bdm_itf.c
 */
#include <stm32f407.h>
#include <devices.h>
#include <gpio_drv.h>
#include <sys.h>
#include <io.h>

#include "bdm_itf.h"

static struct device_handle	*t1ch1_pin_dh;
static struct device_handle	*t1ch2_pin_dh;
static struct device_handle	*t8ch1_pin_dh;
static struct driver		*pindrv;

#if 0
#define PERIPH_BASE	0x40000000
#define APB2PERIPH	(PERIPH_BASE+0x00010000)
#define TIMER9_ADDR 	(APB2PERIPH+0x4000)

struct Timer {
/*	0	*/	volatile unsigned int cr1;
/*	4	*/	volatile unsigned int res1;
/*	8	*/	volatile unsigned int smcr;
/*	c	*/	volatile unsigned int dier;
/*	10	*/	volatile unsigned int sr;
/*	14	*/	volatile unsigned int egr;
/*	18	*/	volatile unsigned int ccmr1;
/*	1c	*/	volatile unsigned int res2;
/*	20	*/	volatile unsigned int ccer;
/*	24	*/	volatile unsigned int cnt;
/*	28	*/	volatile unsigned int psc;
/*	2c	*/	volatile unsigned int arr;
/*	30	*/	volatile unsigned int res3;
/*	34	*/	volatile unsigned int ccr1;
/*	38	*/	volatile unsigned int ccr2;
};

static struct Timer *timer9=(struct Timer *)TIMER9_ADDR;
#endif

struct bdm_itf_user {
	struct device_handle dh;
	DRV_CBH callback;
	void *userdata;
	int userfd;
	int inuse;
};


static struct bdm_itf_user biu;


/*'''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''*/

static int bdm_itf_sync(struct bdm_itf_user *u, int arg) {
        unsigned long int cpu_flags;
	unsigned int last_val=0;

	sys_printf("on enter\n");
	sys_printf("TIM1->SR 0x%x\n", TIM1->SR);
	sys_printf("TIM8->SR 0x%x\n", TIM8->SR);
	TIM8->CR1&=~TIM_CR1_CEN;
	TIM1->CR1&=~TIM_CR1_CEN;
	TIM1->BDTR&=~TIM_BDTR_MOE;
	TIM1->CCR1=128;
	TIM1->CCR2=129;
	TIM1->ARR=130;
	TIM1->CR2=(4<<TIM_CR2_MMS_SHIFT)| TIM_CR2_OIS1; // oc1ref -> trg0
//	TIM8->SMCR=0;
//	TIM8->SMCR=((0<<TIM_SMCR_TS_SHIFT)|(6<<TIM_SMCR_SMS_SHIFT));

	// setup timer 8 channel 1 and channel 2 for input
	TIM8->CCMR1=((1<<TIM_CCMR1_CC1S_SHIFT)|(2<<TIM_CCMR1_CC2S_SHIFT));

	TIM8->SMCR=((5<<TIM_SMCR_TS_SHIFT)|(4<<TIM_SMCR_SMS_SHIFT));
	TIM8->CCER=TIM_CCER_CC1E|TIM_CCER_CC2E|TIM_CCER_CC2P|TIM_CCER_CC2NP;

	TIM1->CNT=0;
	TIM1->SR=0;

	TIM8->ARR=256;
	TIM8->CNT=0;
	TIM8->SR=0;
	TIM8->EGR=TIM_EGR_UG;

	TIM1->EGR=TIM_EGR_UG;

	cpu_flags=disable_interrupts();

	TIM1->CCMR1=((4<<TIM_CCMR1_OC1M_SHIFT) |
			(4<<TIM_CCMR1_OC2M_SHIFT));
	TIM1->CR1|=TIM_CR1_CEN;

	TIM1->CCMR1=((1<<TIM_CCMR1_OC1M_SHIFT) |
			(7<<TIM_CCMR1_OC2M_SHIFT));

	TIM8->CR1=TIM_CR1_CEN;

	while(!(TIM8->SR&0x400)) {
		if (TIM1->CNT!=last_val) {
			last_val=TIM1->CNT;
			if (last_val>0x7a) {
				sys_printf("tim1 count 0x%x, status 0x%x\n",
					last_val,TIM1->SR);
				sys_printf("tim8 count 0x%x, status 0x%x\n",
					 TIM8->CNT, TIM8->SR);
			}
		}
	}
	sys_printf("on exit\n");
	sys_printf("TIM1->SR 0x%x, CNT 0x%x, CR1 0x%x\n",
			TIM1->SR, TIM1->CNT, TIM1->CR1);
	sys_printf("TIM8->SR 0x%x, CNT 0x%x, CR1 0x%x, CCR1 0x%x, CCR2 0x%x\n",
			TIM8->SR, TIM8->CNT, TIM8->CR1, TIM8->CCR1, TIM8->CCR2);
	restore_cpu_flags(cpu_flags);

	return 1;
}

static int bdm_itf_ct(struct bdm_itf_user *u, int arg) {
        unsigned long int cpu_flags;
	unsigned int last_val=0;
	unsigned int max_same_cnt=0x0;
	unsigned int min_same_cnt=0xffffffff;
	unsigned int same_cnt=0;
	unsigned int curr_cnt;
	unsigned int clk_miss=0;

	sys_printf("on enter\n");
	sys_printf("TIM1->SR 0x%x\n", TIM1->SR);
	sys_printf("TIM8->SR 0x%x\n", TIM8->SR);
	TIM8->CR1&=~TIM_CR1_CEN;
	TIM1->CR1&=~TIM_CR1_CEN;
	TIM1->BDTR&=~TIM_BDTR_MOE;
	TIM1->CCR1=128;
	TIM1->CCR2=129;
	TIM1->ARR=130;
	TIM1->CR2=(4<<TIM_CR2_MMS_SHIFT)| TIM_CR2_OIS1; // oc1ref -> trg0
//	TIM8->SMCR=0;
//	TIM8->SMCR=((0<<TIM_SMCR_TS_SHIFT)|(6<<TIM_SMCR_SMS_SHIFT));

	// setup timer 8 channel 1 and channel 2 for input
	TIM8->CCMR1=((1<<TIM_CCMR1_CC1S_SHIFT)|(2<<TIM_CCMR1_CC2S_SHIFT));

	TIM8->SMCR=((5<<TIM_SMCR_TS_SHIFT)|(4<<TIM_SMCR_SMS_SHIFT));
	TIM8->CCER=TIM_CCER_CC1E|TIM_CCER_CC2E|TIM_CCER_CC2P|TIM_CCER_CC2NP;

	TIM1->CNT=0;
	TIM1->SR=0;

	TIM8->ARR=256;
	TIM8->CNT=0;
	TIM8->SR=0;
	TIM8->EGR=TIM_EGR_UG;

	TIM1->EGR=TIM_EGR_UG;

	cpu_flags=disable_interrupts();

	TIM1->CCMR1=((4<<TIM_CCMR1_OC1M_SHIFT) |
			(4<<TIM_CCMR1_OC2M_SHIFT));
	TIM1->CR1|=TIM_CR1_CEN;

	TIM1->CCMR1=((1<<TIM_CCMR1_OC1M_SHIFT) |
			(7<<TIM_CCMR1_OC2M_SHIFT));

	TIM8->CR1=TIM_CR1_CEN;

	while((curr_cnt=TIM1->CNT)>=last_val) {
		if (curr_cnt==last_val) {
			same_cnt++;
		} else {
			if (same_cnt>max_same_cnt) {
				max_same_cnt=same_cnt;
			}
			if (same_cnt<min_same_cnt) {
				min_same_cnt=same_cnt;
			}

			same_cnt=0;
			if ((last_val+1)!=curr_cnt) {
				clk_miss++;
			}
			last_val=curr_cnt;
		}
	}

	sys_printf("on exit\n");
	sys_printf("max_cnt %x, max_same_cnt %x min_same_cnt %x, clk_miss %x\n",
		last_val, max_same_cnt, min_same_cnt, clk_miss);
	sys_printf("TIM1->SR 0x%x, CNT 0x%x, CR1 0x%x\n",
			TIM1->SR, TIM1->CNT, TIM1->CR1);
	sys_printf("TIM8->SR 0x%x, CNT 0x%x, CR1 0x%x, CCR1 0x%x, CCR2 0x%x\n",
			TIM8->SR, TIM8->CNT, TIM8->CR1, TIM8->CCR1, TIM8->CCR2);
	restore_cpu_flags(cpu_flags);

	return 1;
}



/**************** Driver API ********************************************/

static struct device_handle *bdm_itf_open(void *inst, DRV_CBH callback, void *uref) {
	if (!biu.inuse) {
		biu.callback=callback;
		biu.userdata=uref;
		return &biu.dh;
	} else {
		return 0;
	}
}

static int bdm_itf_close(struct device_handle *dh) {
	struct bdm_itf_user *u=(struct bdm_itf_user *)dh;
	u->inuse=0;
	return 0;
}

static int bdm_itf_control(struct device_handle *dh, int cmd, void *arg, int len) {
	struct bdm_itf_user *u=(struct bdm_itf_user *)dh;

	switch(cmd) {
		case BDM_ITF_SYNC:
			return bdm_itf_sync(u,*((int *)arg));
			break;
		case BDM_ITF_CLOCKTEST:
			return bdm_itf_ct(u,*((int *)arg));
			break;
		default:
			return -1;
	}
	return 0;
}

static int bdm_itf_init(void *inst) {

	RCC->APB2ENR |= (RCC_APB2ENR_TIM1EN|RCC_APB2ENR_TIM8EN);
	/* APB2 bus clock 84Mhz */
	TIM1->PSC=0x1a;
	TIM8->PSC=0x1a;
	TIM1->CNT=0;
	TIM8->CNT=0;
	TIM1->CR1 = TIM_CR1_OPM;
//	TIM8->CR1 = TIM_CR1_OPM;

	TIM1->CR2=(4<<TIM_CR2_MMS_SHIFT)| TIM_CR2_OIS1; // oc1ref -> trg0

			// timer 8 triggers from timer1_trg0 (ITR0)
			// timer 8 slave trigger

//	TIM8->SMCR=0;
//	TIM8->SMCR=((0<<TIM_SMCR_TS_SHIFT)|(6<<TIM_SMCR_SMS_SHIFT));

			// timer 1 ch 1 output active on match
			// timer 1 ccr1 is loaded from preload reg
			// timer 1 ch 2 output active on match
			// timer 1 ccr2 is loaded from preload reg
	TIM1->CCMR1=((1<<TIM_CCMR1_OC1M_SHIFT) |
			(7<<TIM_CCMR1_OC2M_SHIFT));
	TIM1->BDTR=TIM_BDTR_AOE | TIM_BDTR_OSSR | TIM_BDTR_OSSI;

			// timer 8 channel 1 input on TI1
//	TIM8->CCMR1=((1<<TIM_CCMR1_CC1S_SHIFT));

	TIM1->CCER=TIM_CCER_CC1E | TIM_CCER_CC2E;

//	TIM8->CCER=TIM_CCER_CC1E;

	TIM1->DIER=0xffff;
	TIM8->DIER=0xffff;
	return 0;
}


static int bdm_itf_start(void *inst) {
	int flags;
	int rc;
	int t1ch1_pin;
	int t1ch2_pin;
	int t8ch1_pin;

	pindrv=driver_lookup(GPIO_DRV);
	if (!pindrv) return 0;

	t1ch1_pin_dh=pindrv->ops->open(pindrv->instance,0,0);
	if (!t1ch1_pin_dh) return -1;

	t1ch1_pin=TIM1_CH1_PIN;  // from config.h
	rc=pindrv->ops->control(t1ch1_pin_dh,GPIO_BIND_PIN,&t1ch1_pin,sizeof(t1ch1_pin));
	if (rc<0) {
		goto closeGPIO;
	}

	t1ch2_pin_dh=pindrv->ops->open(pindrv->instance,0,0);
	if (!t1ch2_pin_dh) return -1;

	t1ch2_pin=TIM1_CH2_PIN; // from config.h
	rc=pindrv->ops->control(t1ch2_pin_dh,GPIO_BIND_PIN,&t1ch2_pin,sizeof(t1ch2_pin));
	if (rc<0) {
		goto closeGPIO;
	}

	t8ch1_pin_dh=pindrv->ops->open(pindrv->instance,0,0);
	if (!t8ch1_pin_dh) return -1;

	t8ch1_pin=TIM8_CH1_PIN; // from config.h
	rc=pindrv->ops->control(t8ch1_pin_dh,GPIO_BIND_PIN,&t8ch1_pin,sizeof(t8ch1_pin));
	if (rc<0) {
		goto closeGPIO;
	}


	flags=GPIO_DIR(0,GPIO_ALTFN_PIN);
	flags=GPIO_SPEED(flags,GPIO_SPEED_HIGH);
	flags=GPIO_DRIVE(flags,GPIO_OPENDRAIN);
	flags=GPIO_ALTFN(flags,1);

	rc=pindrv->ops->control(t1ch1_pin_dh,GPIO_SET_FLAGS,&flags,sizeof(flags));
	if (rc<0) {
		goto closeGPIO;
	}

	flags=GPIO_DIR(0,GPIO_ALTFN_PIN);
	flags=GPIO_SPEED(flags,GPIO_SPEED_HIGH);
	flags=GPIO_DRIVE(flags,GPIO_PUSHPULL);
	flags=GPIO_ALTFN(flags,1);


	rc=pindrv->ops->control(t1ch2_pin_dh,GPIO_SET_FLAGS,&flags,sizeof(flags));
	if (rc<0) {
		goto closeGPIO;
	}

	flags=GPIO_DIR(0,GPIO_ALTFN_PIN);
	flags=GPIO_SPEED(flags,GPIO_SPEED_HIGH);
	flags=GPIO_DRIVE(flags,GPIO_FLOAT);
	flags=GPIO_ALTFN(flags,3);


	rc=pindrv->ops->control(t8ch1_pin_dh,GPIO_SET_FLAGS,&flags,sizeof(flags));
	if (rc<0) {
		goto closeGPIO;
	}


	return 0;

closeGPIO:
	if (t1ch1_pin_dh) pindrv->ops->close(t1ch1_pin_dh);
	if (t1ch2_pin_dh) pindrv->ops->close(t1ch2_pin_dh);
	if (t8ch1_pin_dh) pindrv->ops->close(t8ch1_pin_dh);
	t1ch1_pin_dh=t1ch2_pin_dh=t8ch1_pin_dh=0;
	return 0;
}



static struct driver_ops bdm_itf_ops = {
	bdm_itf_open,
	bdm_itf_close,
	bdm_itf_control,
	bdm_itf_init,
	bdm_itf_start
};


static struct driver bdm_itf = {
	BDM_ITF,
	0,
	&bdm_itf_ops,
};


void init_bdm_itf() {
	sys_printf("init_bdm_itf");
	driver_publish(&bdm_itf);
}



INIT_FUNC(init_bdm_itf);
