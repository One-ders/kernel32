
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
