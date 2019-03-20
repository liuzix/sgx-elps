#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>

#include "sys_format.h"
#include "request.h"

#include "panic.h"
extern "C" {

int __sprintf_chk(
        char *dest,
        int flags __attribute__((unused)),
        size_t dest_len_from_compiler __attribute__((unused)),
        const char *format, ...)
{
    va_list va;
    int retval;
    va_start(va, format);
    retval = vsprintf(dest, format, va);
    va_end(va);
    return retval;
}

void *__dso_handle = 0;
}

extern "C" int __async_syscall(unsigned int n, ...) {
//    libos_print("We do not support system call!");
//    return 0;

    libos_print("async_syscall: %u", n);
    /* Interpret correspoding syscall args types */
    format_t fm_l;
    if (!interpretSyscall(fm_l, n)) {
        libos_print("No support for system call [%u]", n);
        return 0;
    }
    
    /* Create SyscallReq */
    SyscallRequest *req = new SyscallRequest;    
    /* Give this format info to req  */
    req->fm_list = fm_l;

    long val;
    va_list vl;
    va_start(vl, n);
    for (unsigned int i = 0; i < fm_l.args_num; i++) {
        val = va_arg(vl, long);
        req->args[i].arg = val;
    }
    va_end(vl);

    req->fillArgs();

    libos_print("-----SYSCALL(%u)-----", fm_l.syscall_num);
    for (unsigned int i = 0; i < fm_l.args_num; i++) {
        libos_print("arg[%d] type[%u] arg_in[0x%x]", i+1, req->fm_list.types[i], req->args[i].arg);
        if (req->fm_list.sizes[i])
            libos_print("buffer addr[0x%x] : %s", &(req->args[i].data), req->args[i].data);
    }
    return 0;
}

