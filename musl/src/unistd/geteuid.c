#include <unistd.h>
#include "syscall.h"

uid_t geteuid(void)
{
	return __async_syscall(SYS_geteuid);
}
