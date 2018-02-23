
struct fullframe {
	unsigned int zero;	//	0*4
	unsigned int at;	//	1*4
	unsigned int v0;	//	2*4
	unsigned int v1;	//	3*4
	unsigned int a0;	//	4*4
	unsigned int a1;	//	5*4
	unsigned int a2;	//	6*4
	unsigned int a3;	//	7*4
	unsigned int t0;	//	8*4
	unsigned int t1;	//	9*4
	unsigned int t2;	//	10*4
	unsigned int t3;	//	11*4
	unsigned int t4;	//	12*4
	unsigned int t5;	//	13*4
	unsigned int t6;	//	14*4
	unsigned int t7;	//	15*4
	unsigned int s0;	//	16*4
	unsigned int s1;	//	17*4
	unsigned int s2;	//	18*4
	unsigned int s3;	//	19*4
	unsigned int s4;	//	20*4
	unsigned int s5;	//	21*4
	unsigned int s6;	//	22*4
	unsigned int s7;	//	23*4
	unsigned int t8;	//	24*4
	unsigned int t9;	//	25*4
	unsigned int k0;	//	26*4
	unsigned int k1;	//	27*4
	unsigned int gp;	//	28*4
	unsigned int sp;	//	29*4
	unsigned int fp;	//	30*4
	unsigned int ra;	//	31*4

	unsigned int cp0_status;//	32*4
	unsigned int cp0_cause;	//	33*4
	unsigned int cp0_epc;	//	34*4
	unsigned int lo;	//	35*4
	unsigned int hi;	//	36*4
};
