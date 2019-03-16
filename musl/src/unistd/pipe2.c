#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include "syscall.h"

int pipe2(int fd[2], int flag)
{
	if (!flag) return pipe(fd);
	int ret = __async_syscall(SYS_pipe2, fd, flag);
	if (ret != -ENOSYS) return __syscall_ret(ret);
	ret = pipe(fd);
	if (ret) return ret;
	if (flag & O_CLOEXEC) {
		__async_syscall(SYS_fcntl, fd[0], F_SETFD, FD_CLOEXEC);
		__async_syscall(SYS_fcntl, fd[1], F_SETFD, FD_CLOEXEC);
	}
	if (flag & O_NONBLOCK) {
		__async_syscall(SYS_fcntl, fd[0], F_SETFL, O_NONBLOCK);
		__async_syscall(SYS_fcntl, fd[1], F_SETFL, O_NONBLOCK);
	}
	return 0;
}
