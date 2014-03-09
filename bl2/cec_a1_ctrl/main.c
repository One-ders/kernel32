
#include "stm32f4xx_conf.h"
#include "sys.h"
#include "io.h"
#include "hr_timer.h"
#include "cec_drv.h"

#include <string.h>

int hr_t2;
int tout=7000000;

void cec_gw(void *dum) {
	unsigned int s=0;
	int fd=io_open(CEC_DRV);
	if (!fd) return;
	while(1) {
		printf("in cec_gw: at sec %d\n", s);
#if 0
		if (((s-2)%10)==0) {
			io_control(hr_t2,HR_TIMER_SET,&tout,sizeof(tout));
		} else if (((s-7)%10)==0) {
			unsigned int left=io_control(hr_t2,HR_TIMER_CLR,0,0);
			if ((left>2050000)||(left<1950000)) {
				io_printf("weird left value %d\n", left);
			}
		}
#endif
		sleep(1000);
		s++;
	}
}

int main(void) {
//	struct app_data app_data;

	/* initialize the executive */
	init_sys();
	init_io();

	/* start the executive */
	start_sys();
	printf("In main, starting tasks\n");

#if 0
	hr_t2=io_open(HR_TIMER);
#endif
	/* create some jobs */
	thread_create(cec_gw,0,1,"cec_gw");
	while (1);
}
