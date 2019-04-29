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
extern volatile uint64_t *pjiffies;

typedef boost::intrusive::list<RequestBase, member_hook<RequestBase, list_member_hook<>,
                                      &RequestBase::watchListHook>>
    WatchList;

/*
struct ticket_is_key {
    typedef int type;

    const type &operator()(const SchedEntity &v) const { return v.ticket; }
};
*/

/*typedef boost::intrusive::set<
    SchedEntity,
    member_hook<SchedEntity, set_member_hook<>, &SchedEntity::set_member_hook_>,
    key_of_value<ticket_is_key>>
    OrderedMap;
*/
//extern unordered_map<unsigned int, vector<unsigned int>>* syscall_table;
WatchList *watchList;
std::atomic_int32_t ticket;
SpinLock *watchListLock;
//OrderedMap *ticketList;
//extern Singleton<SwapRequest> sg;

void initWatchList() {
    watchListLock = new SpinLock;
    watchList = new WatchList;
    //ticketList = new OrderedMap;
}

/* this is called by the scheduler */
void watchListCheck() {
    //volatile uint64_t jif = *pjiffies;
    //asm("":::"memory");
    watchListLock->lock();
    auto it = watchList->begin();
    while (it != watchList->end()) {
        if (it->waitOnDone(1)) {
            if (!it->owner)
                __asm__("ud2");
            SchedEntity &se = it->owner->se;
            it->owner = nullptr;
            it = watchList->erase(it);
            scheduler->enqueueTask(se);
        } else {
            it++;
        }
    }
    watchListLock->unlock();
    //jif = *pjiffies - jif;
    //asm("":::"memory");
    //libos_print("watchlist CPU cycles: %ld", jif);
}

void sleepWait(RequestBase *req) {
    //req->ticket = ticket.fetch_add(1);
    //scheduler->current.get()->ticket = req->ticket;

    bool intFlag = disableInterrupt();

    watchListLock->lock();
    //ticketList->insert(*scheduler->current.get());
    req->owner = scheduler->getCurrent()->get()->thread;
    watchList->push_back(*req);
    watchListLock->unlock();

    scheduler->dequeueTask(*scheduler->current.get());

    if (!intFlag) enableInterrupt();
    scheduler->schedule();
}


extern volatile uint64_t *pjiffies;
extern "C" int __async_swap(void *addr) {
    SwapRequest *req = Singleton<SwapRequest>::getRequest();
    //SwapRequest *req = createUnsafeObj<SwapRequest>();
    req->addr = (unsigned long)addr;
    requestQueue->push(req);

    //sleepWait(req);
    //req->waitOnDone(0xffffffff);
    req->blockOnDone();
    int ret = (int)req->addr;
    return ret;
}

extern "C" int __async_syscall(unsigned int n, ...) {
    libos_print("Async call [%u]", n);
    /* Interpret correspoding syscall args types */
    format_t fm_l;
    int ret_tmp = interpretSyscall(fm_l, n);
    if (!ret_tmp) {
//        __asm__("ud2");
        libos_print("No support for system call [%u]", n);
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
        libos_print("SYSCALL[%d] fillargs failed", n);
        req->~SyscallRequest();
        //unsafeFree(req);
        return -1;
    }

    requestQueue->push(req);
    req->blockOnDone();
//        sleepWait(req);
//    if (!req->waitOnDone(3000))
//        sleepWait(req);
    libos_print("return val: %ld", req->sys_ret);
    int ret = (int)req->sys_ret;
    req->fillEnclave(enclave_args);
    req->~SyscallRequest();
    //unsafeFree(req);
    libos_print("Async call end");
    return ret;
}

