
#include <stdarg.h>

#include "sys.h"
#include "io.h"

#include "usart_drv.h"


#define MIN(A,B) (A<B?A:B)

#define TX_BUF_SIZE 1024
#define TXI_MASK (TX_BUF_SIZE-1)

static char tx_buf[TX_BUF_SIZE];
static unsigned int tx_out;
static unsigned int tx_in;

static struct driver *iodrv;
static int iodrvix;

int io_cb_handler(void) {
	return 0;
}

#if 0
void io_push() {

	if (!iodrv) return;
	while (tx_in-tx_out>0) {
		int rc =
		iodrv->control(iodrvix, WR_CHAR, &tx_buf[tx_out%TXI_MASK],1);
		if (rc==1) {
			tx_out++;
		}
	}
}

int io_add_c(const char c) {
	tx_buf[tx_in%TXI_MASK]=c;
	tx_in++;
	io_push();
	return 1;
}

int io_add_str(const char *str) {
	unsigned int len=__builtin_strlen(str);
	if (len>1023) return 0;
	if (((tx_in%TXI_MASK)+len)>1023) {
		int l1=len-(1024-(tx_in%TXI_MASK));
		__builtin_memcpy(&tx_buf[tx_in%TXI_MASK],str,l1);
		__builtin_memcpy(tx_buf,&str[l1],len-l1);
	} else {
		__builtin_memcpy(&tx_buf[tx_in%TXI_MASK],str,len);
	}
	tx_in+=len;
	io_push();
	return len;
}

int io_puts(const char *str) {
	return io_add_str(str);
}

#endif

void io_push() {
}

int io_puts(const char *str) {
	if (!iodrv) return 0;
	return iodrv->control(iodrvix, WR_CHAR, str, __builtin_strlen(str));
}

int io_add_c(const char c) {
	if (!iodrv) return 0;
	return iodrv->control(iodrvix, WR_CHAR, &c,1);
}

int io_add_str(const char *str) {
	if (!iodrv) return 0;
	return iodrv->control(iodrvix, WR_CHAR, str, __builtin_strlen(str));
}


char cmap[]={'0','1','2','3','4','5','6','7','8','9',
		'a','b','c','d','e','f'};

char *itoa(int val, char *buf, int prepend_zero, int prepend_num) {
	int i=0;
	int j;
	int to;
	char p_char=prepend_zero?'0':' ';	

	if (!val) {
		buf[i++]=cmap[(val%10)];
	} else {
		while(val) {
			buf[i]=cmap[(val%10)];
			i++;
			val=val/10;
		}
	}
	to=prepend_num-i;
	if (to<0)to=0;
	for(j=0;j<i/2;j++) {
		char tmp=buf[j];
		buf[j]=buf[i-j-1];
		buf[i-j-1]=tmp;
	}
	__builtin_memmove(&buf[to],buf,i);
	__builtin_memset(buf,p_char,to);
	buf[i+to]=0;
	
	return buf;
}

char *xtoa(int val, char *buf, int prepend_zero, int prepend_num) {
	int i=0;
	int j;
	int to;
	char p_char=prepend_zero?'0':' ';

	if (!val) {
		buf[i++]=cmap[(val%10)];
	} else {
		while(val) {
			buf[i]=cmap[(val%16)];
			i++;
			val=val>>4;
		}
	}
	to=prepend_num-i;
	if (to<0)to=0;
	for(j=0;j<i/2;j++) {
		char tmp=buf[j];
		buf[j]=buf[i-1-j];
		buf[i-1-j]=tmp;
	}
	__builtin_memmove(&buf[to],buf,i);
	__builtin_memset(buf,p_char,to);
	buf[i+to]=0;
	
	return buf;
}

int parse_fmt(const char *fmt, int *field_width, int *zero_fill) {
	int i=0;
	int val=0;
	if ((*fmt)=='0') {
		*zero_fill=1;
		i++;
	}
	if ((fmt[i]>='1')&&(fmt[i]<='9')) {
		val=fmt[i]-'0';
		i++;
		while ((fmt[i]>='0') && (fmt[i]<='9')) {
			val=val*10;
			val+=(fmt[i]-'0');
			i++;
		}
	}
	*field_width=val;
	if (i>3) while(1) {;}
	if (i<0) while(1) {;}
	return i;
}

int io_printf(const char *fmt, ...) {
	int i=0;
	va_list ap;
	char numericbuf[16];
	
	va_start(ap,fmt);
	while(1) {
		int c=fmt[i++];
		if (!c) break;
		else if (c=='%') {
			int prepend_zero=0, prepend_num=0;
			i+=parse_fmt(&fmt[i],&prepend_num, &prepend_zero);
			switch((c=fmt[i++])) {
				case 's': {
					char *s=va_arg(ap,char *);
					int len=io_add_str(s);
					if ((prepend_num-len)>0) {
						int j;
						for(j=0;j<(prepend_num-len);j++) {
							io_add_c(' ');
						}
					}
					break;
				}
				case 'c': {
					int gruuk=va_arg(ap, int);
					io_add_c(gruuk);
					break;
				}
				case 'd': {
					char *s=itoa(va_arg(ap,int),numericbuf,
								prepend_zero, prepend_num);
					io_add_str(s);
					break;
				}
				case 'x': {
					char *s=xtoa(va_arg(ap,int),numericbuf,
							prepend_zero, prepend_num);
					io_add_str(s);
					break;
				}
				default: {
					io_add_c('%');
					io_add_c(c);
				}
			}
		} else {
			io_add_c(c);
		}
		
	}
	va_end(ap);
	return 0;
}


void init_io(void) {
	iodrv=driver_lookup(USART_DRV);
	if (iodrv) {
		iodrvix=iodrv->open(iodrv, io_cb_handler);
	}
	io_push();    /* to force out prints, done before open */
#if 0
	thread_create(st_io_r,0,256,"st_io_r");
	thread_create(st_io_t,0,256,"st_io_t");
#endif
}

int fprintf(int fd, const char *fmt, ...) {
	int i=0;
	va_list ap;
	char numericbuf[16];
	
	va_start(ap,fmt);
	while(1) {
		int c=fmt[i++];
		if (!c) break;
		else if (c=='%') {
			int prepend_zero=0, prepend_num=0;
			i+=parse_fmt(&fmt[i],&prepend_num, &prepend_zero);
			switch((c=fmt[i++])) {
				case 's': {
					char *s=va_arg(ap,char *);
					int len=io_write(fd,s,__builtin_strlen(s));
					if ((prepend_num-len)>0) {
						int j;
						for(j=0;j<(prepend_num-len);j++) {
							io_write(fd," ",1);
						}
					}
					break;
				}
				case 'c': {
					int gruuk=va_arg(ap, int);
					io_write(fd,&gruuk,1);
					break;
				}
				case 'd': {
					char *s=itoa(va_arg(ap,int),numericbuf,
								prepend_zero, prepend_num);
					io_write(fd,s,__builtin_strlen(s));
					break;
				}
				case 'x': {
					char *s=xtoa(va_arg(ap,int),numericbuf,
							prepend_zero, prepend_num);
					io_write(fd,s,__builtin_strlen(s));
					break;
				}
				default: {
					io_write(fd,"%",1);
					io_write(fd,&c,1);
				}
			}
		} else {
			io_write(fd,&c,1);
		}
		
	}
	va_end(ap);
	return 0;
}
