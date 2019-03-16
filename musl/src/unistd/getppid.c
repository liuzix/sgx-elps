#include <unistd.h>
#include "syscall.h"

pid_t getppid(void)
{
	return __async_syscall(SYS_getppid);
}
