#include <stdlib.h>
#include "syscall.h"

_Noreturn void _Exit(int ec)
{
	__async_syscall(SYS_exit_group, ec);
	for (;;) __async_syscall(SYS_exit, ec);
}
