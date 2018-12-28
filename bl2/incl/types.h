#ifndef __TYPES_H__
#define __TYPES_H__

typedef unsigned long int size_t;
typedef long int ssize_t;

typedef unsigned long int off_t;
//typedef unsigned long long int loff_t;
typedef unsigned long long int loff_t;

typedef unsigned long long int dev_t;
typedef unsigned long long int ino_t;
typedef unsigned int mode_t;
typedef unsigned int nlink_t;
typedef unsigned int uid_t;
typedef unsigned int gid_t;
typedef unsigned int blksize_t;
typedef unsigned long long int blkcnt_t;

typedef unsigned int time_t;

struct timespec { time_t tv_sec; long tv_nsec;};

#define NULL (0)
#define BITS_PER_LONG (32)

#define BUILD_BUG_ON_ZERO(e) (sizeof(char[1 - 2 * !!(e)]) - 1)
#define __must_be_array(a) \
  BUILD_BUG_ON_ZERO(__builtin_types_compatible_p(typeof(a), typeof(&a[0])))
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]) + __must_be_array(arr))

#endif
