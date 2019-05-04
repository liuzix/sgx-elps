#ifndef SYS_FORMAT_H
#define SYS_FORMAT_H

#define NON_PTR 0
#define CHAR_PTR 1
#define EVENT_PTR 2
#define SOCKADDR_PTR 3
#define INT_PTR 4
#define IOVEC_PTR 5
#define IOC_PTR 6
#define SOKADDR_PTR 7
#define MSGHDR_PTR 8
#define SIGSET_PTR 9
#define TIMESPEC_PTR 10
#define OLD_UTSNAME_PTR 11
#define STAT_PTR 12
#define TIMEVAL_PTR 13
#define TIMEZONE_PTR 14
#define ADDR_PTR 15
#define FD_PAIR_PTR 16

#define SYS_RT_SIGACTION 13
#define SYS_RT_SIGPROCMASK 14
#define SYS_CLONE 56
#define SYS_ARCH_PRCTL 158
#define SYS_SET_TID_ADDRESS 218
#define SYS_EXIT_GROUP 231
#define SYS_SET_ROBUST_LIST 273
#define SYS_BRK 12
#define SYS_GETEGID 108
#define SYS_PIPE2 293
#define SYS_PIPE 22

#define SYS_READ 0
#define SYS_WRITE 1
#define SYS_OPEN 2
#define SYS_CLOSE 3
#define SYS_FSTAT 5
#define SYS_MPROTECT 10
#define SYS_IOCTL 16
#define SYS_WRITEV 20
#define SYS_ACCESS 21
#define SYS_DUP 32
#define SYS_SOCKET 41
#define SYS_CONNECT 42
#define SYS_ACCEPT 43
#define SYS_SENDTO 44
#define SYS_RECVFROM 45
#define SYS_SENDMSG 46
#define SYS_RECVMSG 47
#define SYS_BIND 49
#define SYS_LISTEN 50
#define SYS_GETSOCKNAME 51
#define SYS_GETPEERNAME 52
#define SYS_SETSOCKOPT 54
#define SYS_EXIT 60
#define SYS_UNAME 63
#define SYS_FCNTL 72
#define SYS_GETTIMEOFDAY 96
#define SYS_GETGID 104
#define SYS_EPOLL_CREATE 213
#define SYS_CLOCK_GETTIME 228
#define SYS_EPOLL_CTL 233
#define SYS_EPOLL_WAIT 232
#define SYS_OPENAT 257
#define SYS_EPOLL_PWAIT 281
#define SYS_ACCEPT4 288
#define SYS_EPOLL_CREATE1 291
#define SYS_GETUID 102
#define SYS_GETEUID 107

#define add_syscall0(n) syscall_table->emplace(n, vector<unsigned int>({}))
#define add_syscall1(n, a) syscall_table->emplace(n, vector<unsigned int>({a}))
#define add_syscall2(n, a, b) syscall_table->emplace(n, vector<unsigned int>({a, b}))
#define add_syscall3(n, a, b, c) syscall_table->emplace(n, vector<unsigned int>({a, b, c}))
#define add_syscall4(n, a, b, c, d) syscall_table->emplace(n, \
        vector<unsigned int>({a, b, c, d}))
#define add_syscall5(n, a, b, c, d, e) syscall_table->emplace(n, \
        vector<unsigned int>({a, b, c, d, e}))
#define add_syscall6(n, a, b, c, d, e, f) syscall_table->emplace(n, \
        vector<unsigned int>({a, b, c, d, e, f}))
#define add_type_size(n, s) type_table->emplace(n, s)

struct format_t {
    unsigned int syscall_num;
    bool isIO;
    unsigned int args_num;
    unsigned int types[6];
    unsigned int sizes[6];

    format_t() {}
};

struct syscall_arg_t {
    long arg;
    char *data;
    syscall_arg_t(): arg(0), data(nullptr) {}
};

void initSyscallTable();
bool interpretSyscall(format_t& fm_l, unsigned int index);

#endif
