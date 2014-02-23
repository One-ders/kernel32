
#include <stdarg.h>

#include "sys.h"
#include "io.h"

#define ASSERT(a) {while(!(a)) ; }



static struct driver *iodrv;
static int kfd;

int io_cb_handler(void *dum) {
	return 0;
}

void io_push() {
}

int io_puts(const char *str) {
	if (!iodrv) return 0;
	return iodrv->ops->control(kfd, WR_CHAR, (char *)str, __builtin_strlen(str));
}

int io_add_c(const char c) {
	if (!iodrv) return 0;
	return iodrv->ops->control(kfd, WR_CHAR, (char *)&c,1);
}

int io_add_str(const char *str) {
	if (!iodrv) return 0;
	return iodrv->ops->control(kfd, WR_CHAR, (char *)str, __builtin_strlen(str));
}

int in_print;
int pulled;
int io_setpolled(int enabled) {
	if (!iodrv) return 0;
	in_print=0;
	if (enabled) pulled++;
	else pulled--;
	return iodrv->ops->control(kfd, WR_POLLED_MODE, &pulled, sizeof(pulled));
}


char cmap[]={'0','1','2','3','4','5','6','7','8','9',
		'a','b','c','d','e','f'};

char *itoa(unsigned int val, char *buf, int bz, int prepend_zero, int prepend_num) {
	int i=0;
	int j;
	int to;
	char p_char=prepend_zero?'0':' ';	

	if (!val) {
		ASSERT(i<bz);
		buf[i++]=cmap[0];
	} else {
		while(val) {
			ASSERT(i<bz);
			buf[i]=cmap[(val%10)];
			i++;
			val=val/10;
		}
	}
	to=prepend_num-i;
	if (to<0)to=0;
	ASSERT(i<bz);
	for(j=0;j<i/2;j++) {
		char tmp=buf[j];
		buf[j]=buf[i-j-1];
		buf[i-j-1]=tmp;
	}
	ASSERT((i+to)<bz);
	__builtin_memmove(&buf[to],buf,i);
	__builtin_memset(buf,p_char,to);
	buf[i+to]=0;
	
	return buf;
}

char *xtoa(unsigned int val, char *buf, int bz, int prepend_zero, int prepend_num) {
	int i=0;
	int j;
	int to;
	char p_char=prepend_zero?'0':' ';

	if (!val) {
		ASSERT(i<bz);
		buf[i++]=cmap[(val%10)];
	} else {
		while(val) {
			ASSERT(i<bz);
			buf[i]=cmap[(val%16)];
			i++;
			val=val>>4;
		}
	}
	to=prepend_num-i;
	if (to<0)to=0;
	ASSERT(i<bz);
	for(j=0;j<i/2;j++) {
		char tmp=buf[j];
		buf[j]=buf[i-1-j];
		buf[i-1-j]=tmp;
	}
	ASSERT((i+to)<bz);
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

	/* protect against recursion */
	if (__sync_fetch_and_or(&in_print,1)) {
		return -1;
	}
	
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
					char *s=itoa(va_arg(ap,unsigned int),numericbuf, 16,
								prepend_zero, prepend_num);
					io_add_str(s);
					break;
				}
				case 'x': {
					char *s=xtoa(va_arg(ap,unsigned int),numericbuf, 16,
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
	in_print=0;
	return 0;
}


void init_io(void) {
	iodrv=driver_lookup("usart0");
	if (!iodrv) {
		return;
	}
	kfd=iodrv->ops->open(iodrv->instance, io_cb_handler, 0);
	io_push();    /* to force out prints, done before open */
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
					io_write(fd,(char *)&gruuk,1);
					break;
				}
				case 'd': {
					char *s=itoa(va_arg(ap,unsigned int),numericbuf, 16,
								prepend_zero, prepend_num);
					io_write(fd,s,__builtin_strlen(s));
					break;
				}
				case 'x': {
					char *s=xtoa(va_arg(ap,unsigned int),numericbuf, 16,
							prepend_zero, prepend_num);
					io_write(fd,s,__builtin_strlen(s));
					break;
				}
				default: {
					io_write(fd,"%",1);
					io_write(fd,(char *)&c,1);
				}
			}
		} else {
			io_write(fd,(char *)&c,1);
		}
		
	}
	va_end(ap);
	return 0;
}
