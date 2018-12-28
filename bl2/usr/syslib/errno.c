
#include <errno.h>

static int my_errno;

int *___errno_location(void)
{
        return &my_errno;
}
