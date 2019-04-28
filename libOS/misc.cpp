#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>
#include "libos.h"
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

#define weak __attribute__((__weak__))
#define hidden __attribute__((__visibility__("hidden")))
#define weak_alias(old, new) \
    extern __typeof(old) new __attribute__((__weak__, __alias__(#old)))


static void dummy(void) {} weak_alias(dummy, _init);

extern weak hidden void (*const __init_array_start)(void), (*const __init_array_end)(void);

void __temp_libc_start_init(void)
{
    //_init();

    uintptr_t a = (uintptr_t)&__init_array_start;
    for (; a<(uintptr_t)&__init_array_end; a+=sizeof(void(*)())) {
        uintptr_t cur_func = *(uintptr_t *)a;
        libos_print("calling function: 0x%lx", cur_func - getSharedTLS()->loadBias);
        ((void (*)(void))cur_func)();
    }
}

void raise(int sig) {
    libos_print("libc trying to raise signal %d", sig);
    return;
}

}

