#include <unistd.h>
#include "syscall.h"

pid_t getpid(void)
{
	return __async_syscall(SYS_getpid);
}
