

int cec_init_cec(void);
int cec_set_rec(int enable);
int cec_send(unsigned char *b, int len);
int cec_set_ackmask(int ackmask);

int wakeup_usb_dev(void);
