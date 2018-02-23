#include <sys.h>
#include <io.h>

int init_memory_protection(void) {
	return 0;
}

int map_stack_page(unsigned long int addr, unsigned int size) {
	return 0;
}

int map_tmp_stack_page(unsigned long int addr, unsigned int size) {
	return 0;
}

int map_next_stack_page(unsigned long int new_addr, unsigned long int old_addr) {
	return 0;
}

int unmap_tmp_stack_page() {
	return 0;
}



int unmap_stack_memory(unsigned long int addr) {
	return 0;
}

int activate_memory_protection(void) {
	return 0;
}
