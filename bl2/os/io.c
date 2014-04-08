/* $FrameWorks: , v1.1 2014/04/07 21:44:00 anders Exp $ */

/*
 * Copyright (c) 2014, Anders Franzen.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * @(#)io.c
 */
#include <stdarg.h>

#include "sys.h"
#include "io.h"

static struct driver *iodrv;
static struct device_handle *dh;

int io_cb_handler(struct device_handle *dh, int ev, void *dum) {
	return 0;
}

void io_push() {
}

int io_add_c(const char c) {
	if (!iodrv) return 0;
	return iodrv->ops->control(dh, WR_CHAR, (char *)&c,1);
}

int io_add_str(const char *str) {
	if (!iodrv) return 0;
	return iodrv->ops->control(dh, WR_CHAR, (char *)str, __builtin_strlen(str));
}

int io_add_strn(const char *bytes, int len) {
	if (!iodrv) return 0;
	return iodrv->ops->control(dh, WR_CHAR, (char *)bytes, len);
}


int in_print;
int pulled;
int io_setpolled(int enabled) {
	if (!iodrv) return 0;
	in_print=0;
	if (enabled) pulled++;
	else pulled--;
	return iodrv->ops->control(dh, WR_POLLED_MODE, &pulled, sizeof(pulled));
}


char cmap[]={'0','1','2','3','4','5','6','7','8','9',
		'a','b','c','d','e','f'};

char *itoa(unsigned int val, char *buf, int bz, int prepend_zero, int prepend_num) {
	int i=0;
	int j;
	int to;
	char p_char=prepend_zero?'0':' ';	
	char p_neg=0;

	if (val&0x80000000) {
		val&=~0x80000000;
		val=0x80000000-val;
		p_neg=1;
	}
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
	if (p_neg) to++;
	ASSERT(i<bz);
	for(j=0;j<i/2;j++) {
		char tmp=buf[j];
		buf[j]=buf[i-j-1];
		buf[i-j-1]=tmp;
	}
	ASSERT((i+to)<bz);
	__builtin_memmove(&buf[to],buf,i);
	__builtin_memset(buf,p_char,to);
	if (p_neg) {
		buf[0]='-';
	}
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

unsigned long int strtoul(char *str, char *endp, int base) {
	char *p=str;
	int num=0;
	int mode=0;
	unsigned int val=0;

	if (base) {
		mode=base;
	} else {
		if ((p[0]=='0')&&(__builtin_tolower(p[1])=='x')) {
			mode=16;
			p=&p[2];
		} else if (p[0]=='0') {
			mode=8;
			p=&p[1];
		} else {
			mode=10;
		}
	}

	while(*p) {
		switch (*p) {
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				num=(*p)-'0';
				break;
			case 'a':
			case 'b':
			case 'c':
			case 'd':
			case 'e':
			case 'f':
				num=__builtin_tolower(*p)-'a';
				num+=10;
				break;
		}
		val=(val*mode)+num;
		p++;
	}
	return val;
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


int sys_printf(const char *fmt, ...) {
	int i=0;
	va_list ap;
	char numericbuf[16];
	const char *ppos=fmt;

	/* protect against recursion */
	if (__sync_fetch_and_or(&in_print,1)) {
		return -1;
	}
	
	va_start(ap,fmt);
	while(1) {
		char *cppos;
		int c;
		if (!(*ppos)) break;
		cppos=__builtin_strchr(ppos,'%');
		if (cppos) {
			int len=cppos-ppos;
			if (len) io_add_strn(ppos,len);
			ppos=cppos;
			i=0;
		} else {
			io_add_str(ppos);
			break;
		}
		c=ppos[i++];
		if (c=='%') {
			int prepend_zero=0, prepend_num=0;
			i+=parse_fmt(&ppos[i],&prepend_num, &prepend_zero);
			switch((c=ppos[i++])) {
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
		}
		ppos+=i;
		
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
	dh=iodrv->ops->open(iodrv->instance, io_cb_handler, 0);
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
