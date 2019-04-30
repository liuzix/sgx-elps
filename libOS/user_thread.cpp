#include "user_thread.h"

#include "panic.h"
#include "thread_local.h"
#include "mmap.h"
#include <atomic>

/* copied from futex.h. */
extern "C" uint64_t libos_futex(uint32_t *addr, int op, uint32_t val,
                                uint64_t utime, uint32_t *addr2, uint32_t val3);
#define FUTEX_WAKE      1
/* we do not have a seperate futex_impl.h, so including futex.h causes circular dependency */

std::atomic_int threadCounter;
void *tlsBase;
size_t tlsLength;
extern uint64_t *pjiffies;

uint64_t cw_jiffies; 

struct transfer_data {
    UserThread *prev;
    UserThread *cur;
};

extern "C" void __entry_helper(transfer_t transfer) {
    //libos_print("jump_fcontext took %d cycles", *pjiffies - cw_jiffies);
    transfer_data *data = (transfer_data *)transfer.data;
    auto prev = data->prev;
    auto cur = data->cur;
    if (prev) {
        //libos_print("saving context %lx to thread %d", transfer.fctx, data->prev->id);
        prev->fcxt = transfer.fctx;
    }
    getSharedTLS()->preempt_injection_stack = cur->preempt_stack;
    if (prev) {
        prev->contextLock.unlock();
    }

    cur->contextLock.unlock();
    enableInterrupt();
    int val = cur->entry();
    cur->terminate(val);
}

/* this is just to make the debugger happy */
__attribute__((naked)) static void __clear_rbp(transfer_t) {
   __asm__("xor %rbp, %rbp;"
           "sub $8, %rsp;"
           "call __entry_helper;");
}

UserThread::UserThread(function<int(void)> _entry)
    : se(this) {

    this->id = threadCounter.fetch_add(1);
    this->entry = _entry;

    char *stack = (char *)libos_mmap(NULL, STACK_SIZE);
    stack += STACK_SIZE;

    this->fcxt = make_fcontext(stack, STACK_SIZE, __clear_rbp);
    this->preempt_stack = (uint64_t)libos_mmap(NULL, 4096);
    this->preempt_stack += 4096 - 16;
    //scheduler->enqueueTask(this->se); 
    request_obj = unsafeMalloc(sizeof(SwapRequest));
    
    // this is just to make libOS runnable before libc initializes pthread 
    this->fs_base = (uint64_t)allocateTCB();
    //pt_local->tid = 0xbeefbeef;
}

UserThread::UserThread() : se(this) {
    this->id = threadCounter.fetch_add(1);
}


void UserThread::terminate(int val) {
    libos_print("Thread %d: returned %d",
            this->id, val);

    if (this->clear_child_tid) {
        *this->clear_child_tid = 0;
        libos_futex((uint32_t *)this->clear_child_tid, FUTEX_WAKE, 1, 0, 0, 0); 
    }

    disableInterrupt();
    scheduler->dequeueTask(*scheduler->current.get());
    scheduler->schedule();
}

static inline void setFSReg(uint64_t val) {
    __asm__("wrfsbase %0":: "r"(val));
}

static inline uint64_t getFSReg() {
    uint64_t ret;
    __asm__("rdfsbase %0": "=r"(ret));
    return ret;
}

void UserThread::jumpTo(UserThread *from) {
    //libos_print("switching to thread %d, from thread %d", this->id, from ? from->id: -1); 
    if (from) from->fs_base = getFSReg();
    setFSReg(this->fs_base);
    transfer_data t{ .prev = from, .cur = this };
    //libos_print("loading context %lx", this->fcxt);
    //cw_jiffies = *pjiffies;
    transfer_t ret_t = jump_fcontext(this->fcxt, (void *)&t);
    //libos_print("jump_fcontext took %d cycles", *pjiffies - cw_jiffies);
    transfer_data *data = (transfer_data *)ret_t.data;
    auto prev = data->prev;
    auto cur = data->cur;
    if (prev) {
       // libos_print("saving context %lx to thread %d", ret_t.fctx, data->prev->id);
        prev->fcxt = ret_t.fctx;
        prev->contextLock.unlock();
    }
    cur->contextLock.unlock();
    getSharedTLS()->preempt_injection_stack = data->cur->preempt_stack;
    enableInterrupt();
}

extern "C" pthread* libos_pthread_self(void) {
    pthread *ret;
    __asm__("movq %%fs:0, %0": "=r"(ret));
    return ret;

}

pthread *allocateTCB () {
    char *TCB = (char *)malloc(tlsLength + sizeof(pthread));
    auto pthreadPtr = new (TCB + tlsLength) pthread;
    pthreadPtr->self = pthreadPtr;
    libos_print("tlbBase = 0x%lx", tlsBase);
    memcpy(TCB, tlsBase, tlsLength);
    return pthreadPtr;
}

extern "C" long __set_tid_address(int *tidptr) {
    UserThread *cur = scheduler->getCurrent()->get()->thread;
    cur->clear_child_tid = tidptr;
    *tidptr = cur->id;
    return cur->id;
}

extern "C" int libos_clone(int (*fn)(void *), void *stack, void *arg, void *newtls, int *detach) {
    libos_print("libos_clone!");
    auto userThread = new UserThread;
    userThread->fcxt = make_fcontext((char *)stack, STACK_SIZE, __clear_rbp);
    userThread->preempt_stack = (uint64_t)libos_mmap(NULL, 4096);
    userThread->preempt_stack += 4096 - 16;
    
    pthread *pt = (pthread *)newtls;
    pt->tid = userThread->id;
    //userThread->pt_local = pt;
    userThread->fs_base = (uint64_t) pt; 
    auto entry = std::bind(fn, arg);
    userThread->entry = entry; 
    userThread->clear_child_tid = detach;

    scheduler->enqueueTask(userThread->se);
    return 0;
}

extern "C" void libos_exit_thread(int val) {
    auto cur = scheduler->getCurrent()->get()->thread; 
    cur->terminate(val);
    __asm__("ud2");
}
