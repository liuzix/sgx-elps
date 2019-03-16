#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include "syscall.h"

int remove(const char *path)
{
#ifdef SYS_unlink
	int r = __async_syscall(SYS_unlink, path);
#else
	int r = __async_syscall(SYS_unlinkat, AT_FDCWD, path, 0);
#endif
#ifdef SYS_rmdir
	if (r==-EISDIR) r = __async_syscall(SYS_rmdir, path);
#else
	if (r==-EISDIR) r = __async_syscall(SYS_unlinkat, AT_FDCWD, path, AT_REMOVEDIR);
#endif
	return __syscall_ret(r);
}
