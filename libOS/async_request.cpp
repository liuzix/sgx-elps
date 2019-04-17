#include <syscall_format.h>
#include <request.h>
#include "panic.h"
#include "allocator.h"
#include "thread_local.h"
#include "user_thread.h"
#include "sched.h"
#include "libos.h"
#include "sched.h"

std::atomic_int32_t ticket;

struct ticket_is_key
{
   typedef int type;

   const type & operator()(const SchedEntity& v) const
   {  return v.ticket;  }
}; 

typedef set<SchedEntity, key_of_value<ticket_is_key> > OrderedMap;

static inline uint32_t getTicket() {
    return ticket.fetch_add(1);
}

extern Scheduler *scheduler;

extern "C" int __async_swap(void* addr) {
    //uint64_t jiffies = *pjiffies;
    //libOS_shared_tls *tls = getSharedTLS();
    //SwapRequest *req = scheduler->current->thread->request_obj;
    //SwapRequest *req = (SwapRequest *)tls->request_obj;
    /*
     if (!tls->request_obj) {
        req = (SwapRequest *)unsafeMalloc(sizeof(SwapRequest));
        tls->request_obj = (uint64_t)req;
        if (!req) {
            libos_print("allocate unsafe obj failed.\n");
            return -1;
        }
    }*/

    SwapRequest *req = new ((SwapRequest *)(*scheduler->current)->thread->request_obj)
                       SwapRequest((unsigned long)addr);
    req->addr = (unsigned long)addr;
    requestQueue->push(req);
    //libos_printb("__async_swap: get and push reqobj CPU cycles: %ld\n", *pjiffies - jiffies);
    req->waitOnDone(1000000000);
    int ret = (int)req->addr;
    return ret;
}

extern "C" int __async_syscall(unsigned int n, ...) {
    libos_print("Async call [%u]", n);
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
    long enclave_args[6];
    va_list vl;
    va_start(vl, n);
    for (unsigned int i = 0; i < fm_l.args_num; i++) {
        val = va_arg(vl, long);
        req->args[i].arg = val;
        enclave_args[i] = val;
    }
    va_end(vl);
    /* populate arguments in request */
    if (!req->fillArgs()) {
        req->~SyscallRequest();
        unsafeFree(req);
        return 0;
    }

    requestQueue->push(req);
    req->waitOnDone(1000000000);
    libos_print("return val: %ld", req->sys_ret);
    int ret = (int)req->sys_ret;
    req->fillEnclave(enclave_args); 
    req->~SyscallRequest();
    unsafeFree(req);
    return ret;
}

