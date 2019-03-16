#include <unistd.h>
#include "syscall.h"

pid_t getpgrp(void)
{
	return __async_syscall(SYS_getpgid, 0);
}
