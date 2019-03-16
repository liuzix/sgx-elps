#include <unistd.h>
#include "syscall.h"

void sync(void)
{
	__async_syscall(SYS_sync);
}
