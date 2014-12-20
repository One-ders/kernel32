/* $FrameWorks: , v1.1 2014/04/07 21:44:00 anders Exp $ */

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
 * @(#)hr_tiemr.c
 */
#include <stm32f407.h>
#include <devices.h>
#include <sys.h>
#include <io.h>

#include "hr_timer.h"

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

struct timer_user {
	struct device_handle dh;
	DRV_CBH callback;
	void *userdata;
	int userfd;
	unsigned int out_tic;
	int inuse;
	struct timer_user *next;
};


#define TIMERS	8

static unsigned int current_tic;
static unsigned int tic_step;
static int in_irq;

static struct timer_user tu[TIMERS];
static struct timer_user *tout;

extern unsigned int sys_irqs;

int get_user(struct timer_user **ttu) {
	int i;
	for(i=0;i<TIMERS;i++) {
		if (!tu[i].inuse) {
			break;
		}
	}

	if (i<TIMERS) {
		tu[i].inuse=1;
		(*ttu)=&tu[i];
		return i;
	}
	return -1;
}

static unsigned int get_current_tic() {
	return current_tic+TIM10->CNT;
}

static int adj_hwtimer() {
	if (tout) {
		int ct=get_current_tic();
		int n_tics_to_out=((int)tout->out_tic)-((int)ct);
		if (n_tics_to_out<=0) {
			return ct;
		}
		if (n_tics_to_out<60000) {
			tic_step=n_tics_to_out;
			TIM10->ARR=tic_step;
			TIM10->CNT=0;
			current_tic=ct;
		}
	}
	return 0;
}

void TIM1_UP_TIM10_IRQHandler(void) {
	struct timer_user *t=tout;
	struct timer_user **tprev=&tout;
	int rc;
	int i;

	enable_interrupts();
	if (TIM10->CNT>0) {
		sys_printf("took %d to get irq\n", TIM10->CNT);
	}
	sys_irqs++;

	current_tic+=tic_step;
	TIM10->SR=0;
	if (TIM10->CNT>0) {
		sys_printf("took %d to get irq step 2\n", TIM10->CNT);
	}

	if (!t) {
		if (tic_step!=60000) {
			tic_step=60000;
			TIM10->ARR=60000;
			TIM10->CNT=0;
		}
		return;
	}

	in_irq=1;
	if ((((int)current_tic)-((int)tout->out_tic))<0) {
		if ((rc=adj_hwtimer())) {
			current_tic=rc;
			TIM10->CNT=0;
			sys_printf("next timeout too close, running now\n");
			goto run_tout;
		}
		goto out;
	}

	if (TIM10->CNT>2) {
		sys_printf("took %d to get irq step 3\n", TIM10->CNT); /* ~20uS  for print */
	}

	/* gard against, cnt wrap */
	tic_step=60000;
	TIM10->ARR=60000;

run_tout:
	t=tout;
	tprev=&tout;
	i=0;
	while(t) {
		if ((((int)t->out_tic)-((int)current_tic))>0)  {
			break;
		}
		tprev=&t->next;
		t=t->next;
	}
	t=tout;
	tout=(*tprev);
	(*tprev)=0;

	if (tout==t) {
		sys_printf("baaad timeout list\n");
	}
#if 0
	if (tout) {
		if (tout->out_tic-current_tic<10) {
			sys_printf("close next timeout %d, current %d\n", tout->out_tic,current_tic);
		}
	}
#endif

	while(t) {
		struct timer_user *tnext=t->next;
		if (t->out_tic>current_tic) {
			sys_printf("bad timeout; %d current_tic %d, tout_tic %d\n", i, current_tic,t->out_tic);
		}
		i++;
		t->out_tic=0;
		t->next=0;
		t->callback(&t->dh, EV_STATE, t->userdata);
		t=tnext;
	}

	if (!tout) {
		TIM10->CNT=0;
	} else {
		if ((rc=adj_hwtimer())) {
			current_tic=rc;
			TIM10->CNT=0;
//			sys_printf("rerunning timeouts, current tic %d, tout->tic %d\n", current_tic,tout->out_tic);
			goto run_tout;
		}
	}
out:
	in_irq=0;
}


/*'''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''*/


static int timer_link_in(struct timer_user *u) {
	struct timer_user *p=tout;
	struct timer_user **pprev=&tout;
	while(p) {
		if ((((int)u->out_tic)-((int)p->out_tic))<0) {
			u->next=p;
			break;
		}
		pprev=&p->next;
		p=p->next;
	}
	(*pprev)=u;
	return 0;
}

static int timer_link_out(struct timer_user *u) {
	struct timer_user *p=tout;
	struct timer_user **pprev=&tout;
	while(p) {
		if (u==p) {
			break;
		}
		pprev=&p->next;
		p=p->next;
	}
	(*pprev)=u->next;
	u->next=0;
	return 0;
}


static int hr_timer_set(struct timer_user *u, int val) {
	unsigned int ct;
	unsigned long int cpu_flags;

	if ((val>10000000)||(val<=5)) {
		return -1;
	}
	if (u->out_tic) {
		sys_printf("setting active timer\n");
		return -1;
	}

	cpu_flags=disable_interrupts();
	ct=get_current_tic();
	u->out_tic=ct+(val-1);
	u->next=0;
	timer_link_in(u);

	if (in_irq) {
		restore_cpu_flags(cpu_flags);
		return 0;
	}
	if (tout==u) {
		if (adj_hwtimer()) {
			sys_printf("hr_timer_set: should have timeout immideately\n");
		}
	}
	restore_cpu_flags(cpu_flags);
	return 0;
}

static int hr_timer_clr(struct timer_user *u) {
	unsigned int left;
	unsigned long int cpu_flags;
	if (!u->out_tic) return 0;
	cpu_flags=disable_interrupts();
	left=u->out_tic-get_current_tic();
	u->out_tic=0;
	timer_link_out(u);
	if ((!tout)&&(tic_step!=60000)) {
		tic_step=60000;
		TIM10->ARR=tic_step;
		TIM10->CNT=0;
	}
	restore_cpu_flags(cpu_flags);
	return left;
}


/**************** Driver API ********************************************/

static struct device_handle *hr_timer_open(void *inst, DRV_CBH callback, void *uref) {
	struct timer_user *tu=0;
	int ix=get_user(&tu);
	if (ix<0) return 0;
	tu->callback=callback;
	tu->userdata=uref;
	return &tu->dh;
}

static int hr_timer_close(struct device_handle *dh) {
	struct timer_user *tu=(struct timer_user *)dh;
	tu->inuse=0;
	return 0;
}

static int hr_timer_control(struct device_handle *dh, int cmd, void *arg, int len) {
	struct timer_user *u=(struct timer_user *)dh;

	switch(cmd) {
		case HR_TIMER_SET:
			return hr_timer_set(u,*((int *)arg));
			break;
		case HR_TIMER_CANCEL:
			return hr_timer_clr(u);
			break;
		case HR_TIMER_GET_TIC: {
			unsigned int *ttic=((unsigned int *)arg);
			*ttic=get_current_tic();
			return 0;
		}
		default:
			return -1;
	}
	return 0;
}

static int hr_timer_init(void *inst) {

	RCC->APB2ENR |= RCC_APB2ENR_TIM10EN;
	/* at 168Mhz, prescaler count of 167 give 1uS timer tics */
	TIM10->PSC=167;   /* prescaling 4Mhz to 1 Mhz should probably be 3...*/
	TIM10->CNT=0;
	TIM10->ARR=60000; /* 60000 gives 60 mS */
	tic_step=60000;
	TIM10->DIER=TIM_DIER_UIE;
//	TIM10->CR1= TIM_CR1_CEN|TIM_CR1_ARPE;
	TIM10->CR1= TIM_CR1_CEN;
	NVIC_SetPriority(TIM1_UP_TIM10_IRQn,0x2);
	NVIC_EnableIRQ(TIM1_UP_TIM10_IRQn);
	return 0;
}


static int hr_timer_start(void *inst) {

	return 0;
}



static struct driver_ops hr_timer_ops = {
	hr_timer_open,
	hr_timer_close,
	hr_timer_control,
	hr_timer_init,
	hr_timer_start
};


static struct driver hr_timer = {
	HR_TIMER,
	0,
	&hr_timer_ops,
};


void init_hr_timer() {
	sys_printf("init_hr_timer");
	driver_publish(&hr_timer);
}



INIT_FUNC(init_hr_timer);
