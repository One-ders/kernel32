
#include <io.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

void *memmove(void *dst, const void *src, size_t n) {
	if (n<=0) return dst;
	if (((unsigned char *)src)<((unsigned char *)dst)) {
		int i;
		for(i=n-1;i>=0;i--) {
			((unsigned char *)dst)[i]=
				((unsigned char *)src)[i];
		}
	} else if (((unsigned char *)src)>((unsigned char *)dst)) {
		int i;
		for(i=0;i<n;i++) {
			((unsigned char *)dst)[i]=
				((unsigned char *)src)[i];
		}
	}
	return dst;
}

void *memcpy(void *dst, const void *src, size_t n) {
	return memmove(dst,src,n);
}

void *memset(void *s, int c, size_t n) {
	unsigned char *p=(unsigned char *)s;
	while(n--) {
		*p++=c;
	}
	return s;
}

int memcmp(const void *s1, const void *s2, size_t n) {
	int i;
	for(i=0;i<n;i++) {
		if ((*((int *)s1))!=(*((int *)s2))) break;
		s1++;s2++;
	}
	return (*((int *)s1)-*((int *)s2));
}



char *strchr(const char *s, int c) {
	c&=0xff;
	do  {
		if (*s==c) return (char *)s;
			else s++;
	} while (*s);
	return 0;
}

size_t strlen(const char *s) {
	const char *start=s;
	while(*s) {s++;}
	return s-start;
}

char *strcpy(char *d, const char *s) {
	do {
		*d=*s;
		d++;
	} while(*s++); 
	return d;
}

int strcmp(const char *s1, const char *s2) {
	while((*s1)&&(*s2)&&((*s1)==(*s2))) {
		s1=s1+1;
		s2=s2+1;
	}
	return (*s1)-(*s2);
}

int strncmp(const char *s1, const char *s2, size_t n) {
	int i;
	for(i=0;i<n;i++) {
		if((*s1)&&(*s2)&&(*s1==*s2)){
			s1++;s2++;
		} else {
			break;
		}
	}
	return (*s1)-(*s2);
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
		assert(i<bz);
		buf[i++]=cmap[0];
	} else {
		while(val) {
			assert(i<bz);
			buf[i]=cmap[(val%10)];
			i++;
			val=val/10;
		}
	}
	to=prepend_num-i;
	if (to<0)to=0;
	if (p_neg) to++;
	assert(i<bz);
	for(j=0;j<i/2;j++) {
		char tmp=buf[j];
		buf[j]=buf[i-j-1];
		buf[i-j-1]=tmp;
	}
	assert((i+to)<bz);
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
		assert(i<bz);
		buf[i++]=cmap[(val%10)];
	} else {
		while(val) {
			assert(i<bz);
			buf[i]=cmap[(val%16)];
			i++;
			val=val>>4;
		}
	}
	to=prepend_num-i;
	if (to<0)to=0;
	assert(i<bz);
	for(j=0;j<i/2;j++) {
		char tmp=buf[j];
		buf[j]=buf[i-1-j];
		buf[i-1-j]=tmp;
	}
	assert((i+to)<bz);
	__builtin_memmove(&buf[to],buf,i);
	__builtin_memset(buf,p_char,to);
	buf[i+to]=0;
	
	return buf;
}

unsigned long int strtoul(char *str, char **endp, int base) {
	char *p=str;
	int num=0;
	int mode=0;
	unsigned int val=0;
	int conv=0;

	if (endp) *endp=str;
	while (isspace(*p)) {
		p++;
		if (!*p) return 0;
	}

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
		switch (__builtin_tolower(*p)) {
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
			default:
				num=20000;   // Signal to break while loop;
				break;
		}
		if (num>=mode) {
			break;
		}
		conv=1;
		val=(val*mode)+num;
		p++;
	}

	if (conv) {
		if (endp) *endp=p;
	}
	return val;
}
