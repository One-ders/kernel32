
#include <string.h>

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
}

void *memcpy(void *dst, const void *src, size_t n) {
	return memmove(dst,src,n);
}

int memcmp (const void *str1, const void *str2, size_t count) {
	register const unsigned char *s1 = (const unsigned char*)str1;
	register const unsigned char *s2 = (const unsigned char*)str2;

	while (count-- > 0) {
		if (*s1++ != *s2++) {
			return s1[-1] < s2[-1] ? -1 : 1;
		}
	}
	return 0;
}

void *memset(void *s, int c, size_t n) {
        unsigned char *p=(unsigned char *)s;
        while(n--) {
                *p=c;
		p++;
        }
        return s;
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
	while(*s++);
	return s-start;
}

size_t strnlen(const char *s,size_t n) {
	const char *start=s;
	while((*s++)&&(n--));
	return s-start;
}


int strcmp(const char *s1, const char *s2) {
	while((*s1)&&(*s2)&&(*s1==*s2)) {
		s1++;s2++;
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

char *strcpy(char *d, const char *s) {
	char *r=d;
	unsigned char c;
	do {
		c=*d=*s;
		d++;s++;
	} while (c!=0);
	return r;
}

char *strncpy(char *d, const char *s, size_t n) {
	char *r=d;
	unsigned char c;
	do {
		c=*d=*s;
		d++;s++;n--;
	} while ((c!=0) & (n));
	return r;
}


char *strcat(char *d, const char *s) {
	strcpy(d+strlen(d),s);
	return d;
}

char *strncat(char *d, const char *s, size_t n) {
	strncpy(d+strlen(d),s,n);
	return d;
}
