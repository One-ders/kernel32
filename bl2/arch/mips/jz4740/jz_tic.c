
#include <regdef.h>
#include <frame.h>
#include <irq.h>
#include <devices.h>
#include <sys.h>


extern unsigned int compare_increment;



void config_sys_tic(unsigned int ms) {
	unsigned int compare;

        compare_increment=1680000;
        compare=read_c0_count();
        compare+=compare_increment;
        set_c0_compare(compare);
        set_c0_status(get_c0_status()|CP0_CAUSE_IP7);
}

