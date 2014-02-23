
#include "stm32f4xx_conf.h"
#include "sys.h"
#include "io.h"

#include <string.h>


int main(void) {
//	struct app_data app_data;

	/* initialize the executive */
	init_sys();
	init_io();

	/* start the executive */
	start_sys();
	printf("In main, starting blink tasks\n");

	/* create some jobs */
//	thread_create(app,&app_data,256,"app");
	while (1);
}
