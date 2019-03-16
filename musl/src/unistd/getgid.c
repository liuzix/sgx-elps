#include <unistd.h>
#include "syscall.h"

gid_t getgid(void)
{
	return __async_syscall(SYS_getgid);
}
