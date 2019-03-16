#include <sys/times.h>
#include "syscall.h"

clock_t times(struct tms *tms)
{
	return __async_syscall(SYS_times, tms);
}
