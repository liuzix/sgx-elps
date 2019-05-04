#include <time.h>
#include "syscall.h"
void __async_sleep(unsigned long ns);

int nanosleep(const struct timespec *req, struct timespec *rem)
{
    rem->tv_nsec = 0;
    rem->tv_sec = 0;
	__async_sleep(req->tv_sec * 1000000000 + req->tv_nsec);
    return 0;
}
