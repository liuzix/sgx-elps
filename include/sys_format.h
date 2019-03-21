#ifndef SYS_FORMAT_H
#define SYS_FORMAT_H

#define NON_PTR 0
#define CHAR_PTR 1

#define SYS_READ 0
#define SYS_WRITE 1
#define SYS_OPEN 2
#define SYS_CLOSE 3
#define SYS_EXIT 60

struct format_t {
    unsigned int syscall_num;
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
