

struct SYS_TIMER {
#define SYS_TIMER_CS_M0 1
#define SYS_TIMER_CS_M1 2
#define SYS_TIMER_CS_M2 4
#define SYS_TIMER_CS_M3 8
	volatile unsigned int cs;
	volatile unsigned int clo;
	volatile unsigned int chi;
	volatile unsigned int c0;
	volatile unsigned int c1;
	volatile unsigned int c2;
	volatile unsigned int c3;
};
