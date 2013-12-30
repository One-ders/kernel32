
#include "stm32f4xx_conf.h"

#include <string.h>

int debug=0;
/*************************  St-term driver ***************************/
#define ST_BSIZE 128
struct st_term {
	unsigned int  magic;
	unsigned char bsize;
	unsigned char tx_len;
	unsigned char rx_ready;
	unsigned char st_term_xx;
	unsigned char tx_buf[ST_BSIZE];
	unsigned char rx_buf[ST_BSIZE];
} st_term = { 0xdeadf00d, ST_BSIZE, 0x00, 0x00, 0x00, };


int st_puts(char *str) {
	int len=strlen(str);
	unsigned char *tb;
	if (st_term.tx_len+len>ST_BSIZE) return 0;
	if (st_term.tx_len) {
		tb=&st_term.tx_buf[st_term.tx_len];
	} else {
		tb=st_term.tx_buf;
	}
	memcpy(tb,str,len);
	st_term.tx_len+=len;
	return len;
}

int sprintf(char *str, const char *format, ...);

void *_sbrk(int bub) {
	return 0;
}

struct task {
	char 		*name;            /* 0-3 */
	unsigned int 	sp;               /* 4-7 */
	
	struct task 	*next;            /* 8-11 */
	int		stack_sz;         /* 12-15 */
	unsigned int	stack[];
};

struct task *ready;
struct task *ready_last;


struct task main_task = { "init_main", 0x20020000, 0, };
struct task *current = &main_task;


struct tq {
	struct task *tq_out_first;
	struct task *tq_out_last;
} tq[1024];

volatile unsigned int tq_tic;


void *SysTick_Handler_c(unsigned int *sp);

void __attribute__ (( naked )) SysTick_Handler(void) {
 /*
  * Get the pointer to the stack frame which was saved before the SVC
  * call and use it as first parameter for the C-function (r0)
  * All relevant registers (r0 to r3, r12 (scratch register), r14 or lr
  * (link register), r15 or pc (programm counter) and xPSR (program
  * status register) are saved by hardware.
  */
  
	asm volatile ( "tst lr,#4\n\t"
			"ite eq\n\t"
			"mrseq r0,msp\n\t"
			"mrsne r0,psp\n\t"
			"mov r1,lr\n\t"
			"mov r2,r0\n\t"
			"push {r1-r2}\n\t"
			"bl %[SysTick_Handler_c]\n\t"
			"pop {r1-r2}\n\t"
			"mov lr,r1\n\t"
			"cmp	r0,#0\n\t"
			"bne.n	1f\n\t"
			"bx lr\n\t"
			"1:\n\t"
			"stmfd r2!,{r4-r11}\n\t"
			"ldr r1,=current\n\t"
			"ldr r1,[r1]\n\t"
			"str r2,[r1,#4]\n\t"
			"ldr r1,=current\n\t"
			"str r0,[r1,#0]\n\t"
			"ldr r0,[r0,#4]\n\t"
			"mov sp,r0\n\t"
			"ldmfd sp!,{r4-r11}\n\t"
			"bx lr\n\t"
			:
			: [SysTick_Handler_c] "i" (SysTick_Handler_c)
			: "r0"
	);
}

void *SysTick_Handler_c(unsigned int *sp) {
	struct tq *tqp;
	tq_tic++;
	tqp=&tq[tq_tic%1024];

	if (tqp->tq_out_first) {
		struct task *t=tqp->tq_out_first;
		if (!ready) {
			ready=t;
		} else {
			ready_last->next=t;
		}
//		while(t->next) t=t->next;
		ready_last=tqp->tq_out_last;
		tqp->tq_out_first=tqp->tq_out_last=0;
	}
	if (ready) {
		struct task *n=0;
			ready_last->next=current;
			ready_last=current;

			n=ready;
			ready=ready->next;
			n->next=0;

			if (debug) {
			if ((st_term.tx_len+80)<ST_BSIZE) {
				int rc;
				char *tb;
				if (st_term.tx_len) {
					tb=(char *)&st_term.tx_buf[st_term.tx_len];	
				} else {
					tb=(char *)st_term.tx_buf;
				}
				rc=sprintf(tb,"time slice: switch out %s, switch in %s\n", current->name, n->name);
				st_term.tx_len+=rc;
			}
			}

			return n;
	}
	return 0;
}

/*
 * Define the interrupt handler for the service calls,
 * It will call the C-function after retrieving the
 * stack of the svc caller.
 */

void *SVC_Handler_c(unsigned int *sp);

void __attribute__ (( naked )) SVC_Handler(void) {
 /*
  * Get the pointer to the stack frame which was saved before the SVC
  * call and use it as first parameter for the C-function (r0)
  * All relevant registers (r0 to r3, r12 (scratch register), r14 or lr
  * (link register), r15 or pc (programm counter) and xPSR (program
  * status register) are saved by hardware.
  */
  
	asm volatile ( "tst lr,#4\n\t"
			"ite eq\n\t"
			"mrseq r0,msp\n\t"
			"mrsne r0,psp\n\t"
			"mov r1,lr\n\t"
			"mov r2,r0\n\t"
			"push {r1-r2}\n\t"
			"bl %[SVC_Handler_c]\n\t"
			"pop {r1-r2}\n\t"
			"mov lr,r1\n\t"
			"cmp	r0,#0\n\t"
			"bne.n	1f\n\t"
			"bx lr\n\t"
			"1:\n\t"
			"stmfd r2!,{r4-r11}\n\t"
			"ldr r1,=current\n\t"
			"ldr r1,[r1]\n\t"
			"str r2,[r1,#4]\n\t"
			"ldr r1,=current\n\t"
			"str r0,[r1,#0]\n\t"
			"ldr r0,[r0,#4]\n\t"
			"mov sp,r0\n\t"
			"ldmfd sp!,{r4-r11}\n\t"
			"bx lr\n\t"
			:
			: [SVC_Handler_c] "i" (SVC_Handler_c)
			: "r0"
	);
}


#define SVC_SWITCH_TO	1
#define SVC_DELAY	2

void *SVC_Handler_c(unsigned int *svc_args) {
	unsigned int svc_number;
	/*
	 * Stack contains:
	 * r0, r1, r2, r3, r12, r14, the return address and xPSR
	 * First argument (r0) is svc_args[0]
	 */

	svc_number=((char *)svc_args[6])[-2];
	switch(svc_number) {
		case SVC_SWITCH_TO: {
			struct task *n=(struct task *)svc_args[0];

			if(!ready) {
				ready_last=current;
				ready=current;
			} else {
				ready_last->next=current;
				ready_last=current;
			}

			if (debug) {
			if ((st_term.tx_len+80)<ST_BSIZE) {
				int rc;
				char *tb;
				if (st_term.tx_len) {
					tb=(char *)&st_term.tx_buf[st_term.tx_len];	
				} else {
					tb=(char *)st_term.tx_buf;
				}
				rc=sprintf(tb,"Create task: name %s, switch out %s\n", n->name, current->name);
				st_term.tx_len+=rc;
			}
			}
			n->next=0;
			
			return n;
			break;
		}
		case SVC_DELAY: {
			struct task *n=0;
			struct tq *tout=&tq[(svc_args[0]+tq_tic)%1024];
			if (!tout->tq_out_first) {
				tout->tq_out_first=current;
				tout->tq_out_last=current;
			} else {
				tout->tq_out_last->next=current;
				tout->tq_out_last=current;
			}

			n=ready;
			ready=ready->next;
			n->next=0;

			if (debug) {
			if ((st_term.tx_len+80)<ST_BSIZE) {
				int rc;
				char *tb;
				if (st_term.tx_len) {
					tb=(char *)&st_term.tx_buf[st_term.tx_len];	
				} else {
					tb=(char *)st_term.tx_buf;
				}
				rc=sprintf(tb,"sleep task: name %s, switch in %s\n", current->name, n->name);
				st_term.tx_len+=rc;
			}
			}

			return n;
			break;
		}
		default:
			break;
	}
	return 0;
}



/*
 *    Task side library calls
 */

/*
 * Inline assembler helper directive: call SVC with the given immediate
 */
#define svc(code) asm volatile ("svc %[immediate]"::[immediate] "I" (code))
 
/*
 * Use a normal C function, the compiler will make sure that this is going
 * to be called using the standard C ABI which ends in a correct stack
 * frame for our SVC call
 */

void svc_switch_to(void *);
void svc_delay(unsigned int);

__attribute__ ((noinline)) void svc_switch_to(void *task) {
	svc(SVC_SWITCH_TO);
}
 

__attribute__ ((noinline)) void svc_delay(unsigned int ms) {
	svc(SVC_DELAY);
}


struct Page {
	unsigned char mem[4096];
};

struct Page pages[20];
int freePage=0;

void *getPage(void) {
	return &pages[freePage++];
}


#if 0
void TIM2_IRQHandler(void) {
	// flash on update event
	if (TIM2->SR & TIM_SR_UIF) tout=1;
	TIM2->SR = 0x0; // reset the status register
}


void *memset(void *s, int c, int n) {
	int i;
	for(i=0;i<n;i++) {
		*((unsigned char *)s)=c;
	}
	return s;	
}

#endif


int thread_create(void *fnc, void *val, int stacksz, char *name) {
	struct task *t=(struct task *)getPage();
	unsigned int *stackp;
	memset(t,0,4096);
	t->sp=((unsigned int)((unsigned char *)t)+4096);
	stackp=(unsigned int *)t->sp;
	*(--stackp)=0x01000000; 		 // xPSR
	*(--stackp)=(unsigned int)fnc;    // r15
	*(--stackp)=0;     // r14
	*(--stackp)=0;     // r12
	*(--stackp)=0;     // r3
	*(--stackp)=0;     // r2
	*(--stackp)=0;     // r1
	*(--stackp)=(unsigned int)val;    // r0
////
	*(--stackp)=0;     // r4
	*(--stackp)=0;     // r5
	*(--stackp)=0;     // r6
	*(--stackp)=0;     // r7
	*(--stackp)=0;     // r8
	*(--stackp)=0;     // r9
	*(--stackp)=0;     // r10
	*(--stackp)=0;     // r11

	t->sp=(unsigned int) stackp;
	t->name=name;
	svc_switch_to(t);
	return 1;
}


void sleep(unsigned int ms) {
	unsigned int tics=ms/10;
	svc_delay(tics);
}

void tic_wait(unsigned int tic) {
	while(1) {
		if ((((int)tic)-((int)tq_tic))<=0)  {
			return;
		}	
	}
}


void st_io_r(void *dum) {
	char buf[256];
	int ix=0;
	while(1) {
		if (st_term.rx_ready) {
			st_term.rx_buf[st_term.rx_ready]=0;
			st_puts((char *)st_term.rx_buf);
			if ((ix+st_term.rx_ready)<256) {
				memcpy(&buf[ix],st_term.rx_buf,st_term.rx_ready);
				ix+=st_term.rx_ready;
				st_term.rx_ready=0;
			}
			if (buf[ix-1]=='\n') {
				if (strstr(buf,"debug")) {
					debug=1;
				} else debug=0;
				ix=0;
			}
		}
		sleep(100);
	}
}

struct blink_data {
	unsigned int pin;
	unsigned int delay;
};


void blink(struct blink_data *bd) {
	while(1) {
//		st_puts("blink\n");
		GPIOD->ODR ^= (1 << bd->pin);
		sleep(bd->delay);
	}
}

void blink_loop(struct blink_data *bd) {
	while(1) {
//		st_puts("blink_loop\n");
		GPIOD->ODR ^= (1 << bd->pin);
		tic_wait((bd->delay/10)+tq_tic);
	}
}


int main(void) {
	struct blink_data green={12,1000};
	struct blink_data amber={13,500};
	struct blink_data red={14,750};
	struct blink_data blue={15,200};

	SysTick_Config(SystemCoreClock/ 100); // 10 mS tic is 100/Sec
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIODEN; // enable the clock to GPIOD
	GPIOD->MODER |= (1 << 24); // set pin 12 to be general purpose output
	GPIOD->MODER |= (1 << 26); // set pin 13 to be general purpose output
	GPIOD->MODER |= (1 << 28); // set pin 14 to be general purpose output
	GPIOD->MODER |= (1 << 30); // set pin 15 to be general purpose output

#if 0
	RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;  // enable TIM2 clock
#endif

//	NVIC->ISER[0] |= 1<< (TIM2_IRQn); // enable the TIM2 IRQ
     
//	TIM2->PSC = 0x0; // no prescaler, timer counts up in sync with the peripheral clock
//	TIM2->PSC = 0x0400; // no prescaler, timer counts up in sync with the peripheral clock
//	TIM2->DIER |= TIM_DIER_UIE; // enable update interrupt
//	TIM2->ARR = 0x01; // count to 1 (autoreload value 1)
//	TIM2->ARR = 0xffff; // count to 1 (autoreload value 1)
//	TIM2->CR1 |= TIM_CR1_ARPE | TIM_CR1_CEN; // autoreload on, counter enabled
//	TIM2->EGR = 1; // trigger update event to reload timer registers
     
	st_puts("hej hop tralla la la\n");
	thread_create(st_io_r,NULL,256,"st_io");
	thread_create(blink,&green,256,"green");
	thread_create(blink,&amber,256,"amber");
	thread_create(blink_loop,&red,256,"red_looping");
	thread_create(blink,&blue,256,"blue");
	while (1);
}
