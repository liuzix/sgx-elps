#include <unistd.h>
#include "syscall.h"

gid_t getegid(void)
{
	return __async_syscall(SYS_getegid);
}
