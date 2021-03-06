#include "allocator.h"
#include "libos.h"
#include "panic.h"
#include "singleton.h"
#include "sched.h"
#include "thread_local.h"
#include "user_thread.h"
#include "spin_lock.h"
#include <atomic>
#include <boost/intrusive/list.hpp>
#include <map>
#include <request.h>
#include <syscall_format.h>

using namespace std;

//typedef boost::intrusive::list<RequestBase, member_hook<RequestBase, list_member_hook<>,
//                                      &RequestBase::watchListHook>, link_mode<normal_link>>
//    WatchList;

//WatchList *watchList;
std::atomic_int32_t ticket;
//SpinLock *watchListLock;

extern uint64_t *pjiffies;

static atomic_int pendingCount;

void initWatchList() {
//    watchListLock = new SpinLock;
//    watchList = new WatchList;
}
/*
void dumpWatchList() {
    libos_print("dumping watchList");
    for (auto &req: scheduler->eachWatchList.get()) {
        libos_print("Pending request type %d", req.requestType);
        if (req.requestType == SyscallRequest::typeTag) {
            auto sysReq = (SyscallRequest *)&req;
            libos_print("Pending syscall: %d", sysReq->fm_list.syscall_num);
        }
    }
}
*/

/* this is called by the scheduler */
void watchListCheck() {
    int cpu = get_cpu();
    auto it = scheduler->eachWatchList[cpu].begin();
    while (it != scheduler->eachWatchList[cpu].end()) {
        auto rq = scheduler->eachWatchList[cpu].listEntry(it);
        if (rq->waitOnDone(1)) {
            if (!rq->owner)
                __asm__("ud2");
            SchedEntity &se = rq->owner->se;
            se.refDec();
            rq->owner = nullptr;
            it = scheduler->eachWatchList[cpu].next(it);
            scheduler->eachWatchList[cpu].listDelete(&rq->watchListHook);
            __sync_synchronize();
            scheduler->enqueueTask(se);
        } else {
            it = scheduler->eachWatchList[cpu].next(it);
        }
    }
}


void sleepWait(RequestBase *req) {
    bool intFlag = disableInterrupt();
    int cpu = get_cpu();

    req->owner = scheduler->getCurrent()->get()->thread;
    req->owner->se.refInc();
    scheduler->dequeueTask(*scheduler->current.get());
    scheduler->eachWatchList[cpu].listAddHead(&req->watchListHook);
    if (!intFlag) enableInterrupt();
    scheduler->schedule();
}

extern "C" void __async_sleep(unsigned long ns) {
    //unsigned long count = ns;
    //libos_print("sleep request for %ld ns", ns);
    auto req = Singleton<SleepRequest>::getRequest(ns); 
    requestQueue->push(req);
    sleepWait(req);
    //req->blockOnDone();
    //while (count--);
    //libos_print("sleep request for %ld ns finished", ns);
}

extern "C" int __async_swap(void *addr) {
    SwapRequest *req = Singleton<SwapRequest>::getRequest();
    //SwapRequest *req = createUnsafeObj<SwapRequest>();
    req->addr = (unsigned long)addr;
    requestQueue->push(req);
    //if (!req->waitOnDone(1))
    sleepWait(req);
    //uint64_t jif = *pjiffies;
    //req->done.store(false);
    //req->blockOnDone();
    //jif = *pjiffies - jif;
    asm volatile("": : :"memory");
    __sync_synchronize();
    int ret = (int)req->addr;
    return ret;
}

extern "C" unsigned long __async_syscall(unsigned int n, ...) {
    //libos_print("[%d] Async call [%u]", scheduler->getCurrent()->get()->thread->id,
    //        n);
    /* Interpret correspoding syscall args types */
    format_t fm_l;
    int ret_tmp = interpretSyscall(fm_l, n);
    if (!ret_tmp) {
        libos_print("No support for system call [%u]", n);
        if (13 <= n && n <= 15) return 0;  // signal related calls
        if (n == 160 || n == 97 || n == 302) return 0; // trlimit related calls
        if (n == 12) return -1; /* We do not support brk() */
        __asm__("ud2");
        return -1;
    }

    /* Create SyscallReq */

    //SyscallRequest *req = createUnsafeObj<SyscallRequest>();

    SyscallRequest *req = Singleton<SyscallRequest>::getRequest();
    if (req == nullptr)
        return -1;
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
        //libos_print("SYSCALL[%d] fillargs failed", n);
        req->~SyscallRequest();
        //unsafeFree(req);
        return -1;
    }
    requestQueue->push(req);
    //libos_print("[%d] pushed request",
    //        scheduler->getCurrent()->get()->thread->id);
//    req->blockOnDone();
//    if (!req->waitOnDone(3000))

    if (n == SYS_READ || n == SYS_WRITE || n == SYS_WRITEV || n == SYS_CLOSE || n == SYS_GETPID) {
        if (!req->waitOnDone(0x50000)) {
            libos_print("Syscall request %d timed out. Sleep wait now.", n);
	    sleepWait(req);
        }
    } else
        sleepWait(req);
    //libos_print("return val: %ld", req->sys_ret);
    unsigned long ret = (unsigned long)req->sys_ret;
    req->fillEnclave(enclave_args);
    req->~SyscallRequest();
    //unsafeFree(req);
    //libos_print("Async call end, %d pending", --pendingCount);
    //if (ret == (unsigned long)-1 && n!= 16)
    //    __asm__("ud2");
    return ret;
}

