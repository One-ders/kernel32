/**
 * common exception handler
 */

extern void serial_puts(const char *);
extern void serial_put_hexint(unsigned int);
void exc_handler(unsigned long type, unsigned long esr, unsigned long elr, unsigned long spsr, unsigned long far, unsigned long sctlr)
{
    // print out interruption type
    switch(type) {
        case 0: serial_puts("Synchronous"); break;
        case 1: serial_puts("IRQ"); break;
        case 2: serial_puts("FIQ"); break;
        case 3: serial_puts("SError"); break;
    }
    serial_puts(": ");
    // decode exception type (some, not all. See ARM DDI0487B_b chapter D10.2.28)
    switch(esr>>26) {
        case 0b000000: serial_puts("Unknown"); break;
        case 0b000001: serial_puts("Trapped WFI/WFE"); break;
        case 0b001110: serial_puts("Illegal execution"); break;
        case 0b010101: serial_puts("System call"); break;
        case 0b100000: serial_puts("Instruction abort, lower EL"); break;
        case 0b100001: serial_puts("Instruction abort, same EL"); break;
        case 0b100010: serial_puts("Instruction alignment fault"); break;
        case 0b100100: serial_puts("Data abort, lower EL"); break;
        case 0b100101: serial_puts("Data abort, same EL"); break;
        case 0b100110: serial_puts("Stack alignment fault"); break;
        case 0b101100: serial_puts("Floating point"); break;
        default: serial_puts("Unknown"); break;
    }
    // decode data abort cause
    if(esr>>26==0b100100 || esr>>26==0b100101) {
        serial_puts(", ");
        switch((esr>>2)&0x3) {
            case 0: serial_puts("Address size fault"); break;
            case 1: serial_puts("Translation fault"); break;
            case 2: serial_puts("Access flag fault"); break;
            case 3: serial_puts("Permission fault"); break;
        }
        switch(esr&0x3) {
            case 0: serial_puts(" at level 0"); break;
            case 1: serial_puts(" at level 1"); break;
            case 2: serial_puts(" at level 2"); break;
            case 3: serial_puts(" at level 3"); break;
        }
    }
    // dump registers
    serial_puts(":\n  ESR_EL1 ");
    serial_put_hexint(esr>>32);
    serial_put_hexint(esr);
    serial_puts(" ELR_EL1 ");
    serial_put_hexint(elr>>32);
    serial_put_hexint(elr);
    serial_puts("\n SPSR_EL1 ");
    serial_put_hexint(spsr>>32);
    serial_put_hexint(spsr);
    serial_puts(" FAR_EL1 ");
    serial_put_hexint(far>>32);
    serial_put_hexint(far);
    serial_puts(" SCTLR_EL1 ");
    serial_put_hexint(sctlr>>32);
    serial_put_hexint(sctlr);
    serial_puts("\n");
    // no return from exception for now
    while(1);
}


#if 0

void dataAbortHandler()
{
// to return enable:
//  asm __volatile__ ("push {r0-r12, lr}\n"  );

#define PC_OFFSET 8
#define EXCEPTION_TYPE "DATA ABORT Exception"
   /* static, so stack operations (in assembler), do not willfully destroy these things */
   static struct pt_regs *p;
   static unsigned int *lnk_ptr; /* 32 bit mode - pointer to last executed instruction */
   static unsigned int *stack;   /* 32 bit mode - pointer to "old" stack frame */
   static unsigned int SPSR;
   static unsigned int ccr;
   static unsigned int *usr_stack;
   static unsigned int *fiq_stack;
   static unsigned int *irq_stack;
   static unsigned int *svc_stack;
   static unsigned int *abt_stack;
   static unsigned int *und_stack;
   static unsigned int *sys_stack;
   static int x;

   /* save all registers! */
   /*  save current stack as "pointer" to exception registers */
   asm __volatile__ ( "stmfd	sp, {r0-r14}\n"
		      "sub	sp, sp, #4*15\n"
		      "mov %0,  sp\n"  : "=r" (p) : : "memory" );

   asm __volatile__ ( "mov %0, r14\n"  : "=r" (r14) );  /* save the link, since above statement gets confused, since it saves "over" the stack */
   asm __volatile__ (" mrs %0, SPSR" : "=r" (SPSR) );   /* get the PSR "Before" the exception! */

   /*  On data abort exception the LR points to PC+8  */
   /*  On IRQ exception the LR points to PC+4  */
   /*  get the last executed PC to lnk_ptr */
   /*  see DDI0487G_b_armv8_arm.pdf page 6052 */
   asm __volatile__ ( /*"sub lr, lr, #8\n" */
		      "mov %0, lr" : "=r" (lnk_ptr) ); lnk_ptr -= (PC_OFFSET/4); /* address is 4 byte pointer, so add only offset divided by 4 */


   /*  switch to supervisor mode - our "run mode" */
   /*  switch back and force in ONE assembler statement, otherwise the values are not saved correctly to our OWN stack */
   asm __volatile__ (	"cps	#0x13\n"
			"mov %0, sp\n"
			"cps #0x17\n": "=r" (stack) );
   svc_stack = stack;

  /*  save current stack as "pointer" to access the "user stack" */
  /* asm __volatile__ (	"cps	#0x10\n"  */
  /*			"mov %0, sp\n"  */
  /*			"cps #0x17\n": "=r" (usr_stack) );  */
  /*  save current stack as "pointer" to access the "sys stack" */
  /* asm __volatile__ (	"cps	#0x1f\n"  */
  /*			"mov %0, sp\n"  */
  /*			"cps #0x17\n": "=r" (sys_stack) );  */

   asm __volatile__ (	"cps	#0x11\n"
			"mov %0, sp\n"
			"cps #0x17\n": "=r" (fiq_stack) );

   asm __volatile__ (	"cps	#0x12\n"
			"mov %0, sp\n"
			"cps #0x17\n": "=r" (irq_stack) );

   asm __volatile__ (	"cps	#0x1b\n"
			"mov %0, sp\n"
			"cps #0x17\n": "=r" (und_stack) );

   asm __volatile__ ("mov %0, sp\n"  : "=r" (abt_stack) );

    printf( "\n!!!!!!!! %s !!!!!!!!\n", EXCEPTION_TYPE);
    printf( "at %p:  code (dump): 0x%08lX \r\n", lnk_ptr,  *(lnk_ptr));
    /*  some sanity check  */
    if (((unsigned int )stack) > 0x4000000)
    {
      printf( "Stack  %p: -> INVALID\r\n", stack );
    }
    else
    {
      unsigned int *stackLoop = stack;
      for (int i=0; i<10;i++)
      {printf( "Stack  %p: -> 0x%08lX\r\n", stackLoop, *(stackLoop) );stackLoop++;}
    }

  asm __volatile__ (  "MRC  p15,0,r0,c1,c0,0\n"
		      "mov %0, r0\n"  : "=r" (ccr) );
  unsigned int FSR = 0, FAR = 0, core=0;
  asm volatile ("mrc p15, 0, %0, c5, c0,  0" : "=r" (FSR));
  asm volatile ("mrc p15, 0, %0, c6, c0,  0" : "=r" (FAR));
  asm volatile ("mrc p15, 0, %0, c0, c0,  5" : "=r" (core));
  core = core & 3;

  printf("========================================\r\n");
  printf("R0  (A1): %08X    R8  (V5): %08X\r\n", p->uregs[ PTRACE_R0_idx ] , p->uregs[ PTRACE_R8_idx ]);
  printf("R1  (A2): %08X    R9  (SB): %08X\r\n", p->uregs[ PTRACE_R1_idx ] , p->uregs[ PTRACE_R9_idx ]);
  printf("R2  (A3): %08X    R10 (SL): %08X\r\n", p->uregs[ PTRACE_R2_idx ] , p->uregs[ PTRACE_R10_idx ]);
  printf("R3  (A4): %08X    R11 (FP): %08X\r\n", p->uregs[ PTRACE_R3_idx ] , p->uregs[ PTRACE_R11_idx ]);
  printf("R4  (V1): %08X    R12 (IP): %08X\r\n", p->uregs[ PTRACE_R4_idx ] , p->uregs[ PTRACE_R12_idx ]);
  printf("R5  (V2): %08X    R13 (SP): %08X\r\n", p->uregs[ PTRACE_R5_idx ] , stack );
  printf("R6  (V3): %08X    R14 (LR): %08X\r\n", p->uregs[ PTRACE_R6_idx ] , r14);
  printf("R7  (V4): %08X    R15 (PC): %08X\r\n", p->uregs[ PTRACE_R7_idx ] , lnk_ptr);
  printf("\nusr_SP:   n/a\r\n"/*,usr_stack*/);
  printf("fiq_SP:   %08X\r\n",fiq_stack);
  printf("irq_SP:   %08X\r\n",irq_stack);
  printf("svc_SP:   %08X\r\n",svc_stack);
  printf("abt_SP:   %08X\r\n",abt_stack);
  printf("und_SP:   %08X\r\n",und_stack);
  printf("sys_SP:   n/a\r\n"/*,sys_stack*/);
  printf("\nSPSR:     %08X\r\n", SPSR );

  /*  now decode the flags  */
  x = SPSR;
  printf("    FLAGS     %c", ( x & ARM_CPSR_N_BIT ) ? 'N' : 'n' );
  printf("%c", ( x & ARM_CPSR_Z_BIT ) ? 'Z' : 'z' );
  printf("%c", ( x & ARM_CPSR_C_BIT ) ? 'C' : 'c' );
  printf("%c", ( x & ARM_CPSR_V_BIT ) ? 'V' : 'v' );
  printf("...");
  printf("%c", ( x & ARM_CPSR_F_BIT ) ? 'F' : 'f' );
  printf("%c", ( x & ARM_CPSR_I_BIT ) ? 'I' : 'i' );
  printf("%c\n    ARM-Mode  ", ( x & ARM_CPSR_T_BIT ) ? 'T' : 't' );
  switch( ARM_MODE_MASK & x )
  {
  case ARM_USR_MODE:
    printf("user-mode\r\n");
    break;
  case ARM_FIQ_MODE:
    printf("fiq-mode\r\n");
    break;
  case ARM_IRQ_MODE:
    printf("irq-mode\r\n");
    break;
  case ARM_SVC_MODE:
    printf("svc-mode\r\n");
    break;
  case ARM_ABT_MODE:
    /* these are not implemented yet!  */
    /* if( p->uregs[ PTRACE_FRAMETYPE_idx ] == PTRACE_FRAMETYPE_pfa ) */
    /* {printf("pfa-mode\r\n");} */
    /* else if( p->uregs[ PTRACE_FRAMETYPE_idx ] == PTRACE_FRAMETYPE_da  ) */
    /* {printf("da-mode\r\n");}  */
    /* else  */
    {printf("unknown-abort-mode\r\n");}
    break;
  case ARM_UND_MODE:
    printf("und-mode\r\n");
    break;
  case ARM_SYS_MODE:
    printf("sys-mode\r\n");
    break;
  }
  /* alignment check, see: /DDI0487G_b_armv8_arm.pdf page 4312 */
  printf("\r\n");
  printf("Core:     %01X\r\n", core );
  printf("CCR:      %08X\r\n", ccr ); /* see: DDI0487G_b_armv8_arm.pdf 6810 */
  printf("    MMU                  : "); if (ccr & (1<<0))  printf("(1) enabled");else printf("(0) disabled");
  printf("\n    alignment fault check: "); if (ccr & (1<<1))  printf("(1) enabled");else printf("(0) disabled");
  printf("\n    data cache           : "); if (ccr & (1<<2))  printf("(1) enabled");else printf("(0) disabled");
  printf("\n    instruction cache    : "); if (ccr & (1<<12))  printf("(1) enabled");else printf("(0) disabled");
  printf("\n    endian               : "); if (ccr & (1<<25))  printf("(1) big");else printf("(0) little");
  printf("\n    access permission    : "); if (ccr & (1<<29))  printf("(1) AP[0] is the Access flag");else printf("(0) AP[0] is access permission bit");
  printf("\n========================================\r\n");
  if (PC_OFFSET == 8) /* Data Abort */
  {
    printf("\nAdditional Data Abort information\n");
    printf("FAR:      %08X (Modified Virtual Address (MVA))\r\n", FAR);
    printf("DFSR:     %08X\r\n", FSR ); /* see: https://developer.arm.com/documentation/ddi0211/k/system-control-coprocessor/system-control-coprocessor-register-descriptions/c5--data-fault-status-register--dfsr*/\
    char *fsr_typ[] =
    {
      "No function, reset value",               /*  0 */
      "Alignment fault",			/*  1 */
      "Debug event fault",			/*  2 */
      "Access Flag fault on Section",		/*  3 */
      "Cache maintenance operation fault",	/*  4 */
      "Translation fault on Section",		/*  5 */
      "Access Flag fault on Page",		/*  6 */
      "Translation fault on Page",		/*  7 */
      "Precise External Abort",			/*  8 */
      "Domain fault on Section",		/*  9 */
      "No function",				/* 10 */
      "Domain fault on Page",			/* 11 */
      "External abort on translation, first level",/* 12 */
      "Permission fault on Section",		/* 13 */
      "External abort on translation, second level",/* 14 */
      "Permission fault on Page",		/* 15 */
      "No function",				/* 16 */
      "No function",				/* 17 */
      "No function",				/* 18 */
      "No function",				/* 19 */
      "No function",				/* 20 */
      "No function",				/* 21 */
      "Imprecise External Abort",		/* 22 */
      "No function",				/* 23 */
      "No function",				/* 24 */
      "No function",				/* 25 */
      "No function",				/* 26 */
      "No function",				/* 27 */
      "No function",				/* 28 */
      "No function",				/* 29 */
      "No function",				/* 30 */
      "No function",				/* 31 */
    };
    int typ = FSR & 0xf;
    if (FSR & 1024) typ += 16;
    int domain = FSR >> 4;
    domain = domain & 0xf;

    printf("  typ:    %s\r\n", fsr_typ[typ] );
    printf("  domain: %01X\r\n", domain );
    if (FSR & 2048)
    printf("  on:     WRITE\r\n");
    else
    printf("  on:     READ\r\n");
  }
  asm __volatile__ ("add	sp, sp, #4*15\n");
  for(;;);

  // to return:
  //  asm __volatile__ ("pop {r0-r12, lr}\n"  );
  //  asm __volatile__ ("subs pc, lr, #8\n"  );
}

#endif
