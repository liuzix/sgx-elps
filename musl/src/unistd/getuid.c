#include <unistd.h>
#include "syscall.h"

uid_t getuid(void)
{
	return __async_syscall(SYS_getuid);
}
