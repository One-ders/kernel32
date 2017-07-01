
#include <types.h>
#include <sys.h>

size_t copy_to_user(void *dst, const void *src, size_t n) {
	memcpy(dst,src,n);
	return n;
}

size_t copy_from_user(void *dst, const void *src, size_t n) {
	memcpy(dst,src,n);
	return n;
}
