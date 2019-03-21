#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>

#include "sys_format.h"
#include "request.h"
#include "thread_local.h"
#include "allocator.h"
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
	_init();

	uintptr_t a = (uintptr_t)&__init_array_start;
    libos_print("init_array: %p", a);
	for (; a<(uintptr_t)&__init_array_end; a+=sizeof(void(*)())) {
        libos_print("init function: %p", *(void **)a);
        uintptr_t cur_func = *(uintptr_t *)a;
        libos_print("calling function: 0x%lx", cur_func);
        ((void (*)(void))cur_func)();
    }
}

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
    
    SyscallRequest *req = createUnsafeObj<SyscallRequest>();
    if (req == nullptr)
        return 0;
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
    /* populate arguments in request */
    if (!req->fillArgs()) {
        req->~SyscallRequest();
        unsafeFree(req);
        return 0;
    }

    libos_print("-----SYSCALL(%u)-----", fm_l.syscall_num);
    for (unsigned int i = 0; i < fm_l.args_num; i++) {
        libos_print("arg[%d] type[%u] arg_in[0x%x]", i+1, req->fm_list.types[i], req->args[i].arg);
        if (req->fm_list.sizes[i])
            libos_print("buffer addr[0x%x] : %s", &(req->args[i].data), req->args[i].data);
    }

    requestQueue->push(req);
    req->waitOnDone(1000000000);
//   req->~SyscallRequest();
//   unsafeFree(req);
    return 0;
}

