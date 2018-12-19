
#define TASK_STATE_IDLE         0
#define TASK_STATE_RUNNING      1
#define TASK_STATE_READY        2
#define TASK_STATE_TIMER        3
#define TASK_STATE_IO           4
#define TASK_STATE_DEAD         6

struct procdata {
	char name[32];
	unsigned long int id;
	unsigned long int addr;
	unsigned long int sp;
	unsigned long int state;
	unsigned long int prio_flags;
	unsigned long int active_tics;
	unsigned long int pc;
};
