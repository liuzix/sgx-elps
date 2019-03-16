#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>

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

int __async_syscall() {
    libos_panic("We do not support system call!");
    return 0;
}

void *__dso_handle = 0;
}
