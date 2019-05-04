#include <unistd.h>
#include "syscall.h"

int dup(int fd)
{
	return __async_syscall(SYS_dup, fd);
}
