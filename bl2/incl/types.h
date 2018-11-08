#ifndef __TYPES_H__
#define __TYPES_H__

typedef unsigned long int size_t;
typedef long int ssize_t;

typedef unsigned long int off_t;
typedef unsigned long long int loff_t;

typedef unsigned int dev_t;

#define NULL (0)
#define BITS_PER_LONG (32)

#define BUILD_BUG_ON_ZERO(e) (sizeof(char[1 - 2 * !!(e)]) - 1)
#define __must_be_array(a) \
  BUILD_BUG_ON_ZERO(__builtin_types_compatible_p(typeof(a), typeof(&a[0])))
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]) + __must_be_array(arr))

#endif
