
#include "../mini_uart.h"

void startup_test(void) {

	uart_init();
	uart_send_string("kalle kalle\r\n");

	while(1);
}
