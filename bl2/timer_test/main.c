
#include <sys.h>
#include <io.h>

int main(void) {
        /* initialize the executive */
        init_sys();
        init_io();

        /* start the executive */
        start_sys();
        printf("In main, starting tasks\n");

        /* create some jobs */
	io_open("test_driver");
        while (1);
}

