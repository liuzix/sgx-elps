#include <threads.h>
#include "syscall.h"

void thrd_yield()
{
	__async_syscall(SYS_sched_yield);
}
