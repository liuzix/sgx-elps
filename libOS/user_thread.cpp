#include "user_thread.h"
#include "panic.h"
#include "thread_local.h"
#include "mmap.h"
#include <atomic>

std::atomic_int counter;
void *tlsBase;
size_t tlsLength;


struct transfer_data {
    UserThread *prev;
    UserThread *cur;
};

extern "C" void __entry_helper(transfer_t transfer) {
    transfer_data *data = (transfer_data *)transfer.data;
    if (data->prev) {
        libos_print("saving context %lx to thread %d", transfer.fctx, data->prev->id);
        data->prev->fcxt = transfer.fctx;
    }
    getSharedTLS()->preempt_injection_stack = data->cur->preempt_stack;
    data->cur->entry();
    enableInterrupt();
}

/* this is just to make the debugger happy */
__attribute__((naked)) static void __clear_rbp(transfer_t) {
   __asm__("xor %rbp, %rbp;"
           "call __entry_helper;");
}

UserThread::UserThread(function<int(void)> _entry)
    : se(this) {

    this->id = counter.fetch_add(1);
    this->entry = _entry;

    char *stack = (char *)libos_mmap(NULL, STACK_SIZE);
    stack += (STACK_SIZE - 16);

    this->fcxt = make_fcontext(stack, STACK_SIZE, __clear_rbp);
    this->preempt_stack = (uint64_t)libos_mmap(NULL, 4096);
    this->preempt_stack += 4096 - 16;
    //scheduler->enqueueTask(this->se); 
    request_obj = unsafeMalloc(sizeof(SwapRequest));
    
    pt_local = allocateTCB();
}

UserThread::UserThread(int tid)
    : se(this) {
    pt_local = allocateTCB();
    this->pt_local->tid = tid;
}

static inline void setFSReg(uint64_t val) {
    __asm__("wrfsbase %0":: "r"(val));
}

void UserThread::jumpTo(UserThread *from) {
    libos_print("switching to thread %d, from thread %d", this->id, from ? from->id: -1); 
    setFSReg((uint64_t)this->pt_local);
    transfer_data t = { .prev = from, .cur = this };
    libos_print("loading context %lx", this->fcxt);
    transfer_t ret_t = jump_fcontext(this->fcxt, (void *)&t);
    transfer_data *data = (transfer_data *)ret_t.data;
    if (data->prev) {
        libos_print("saving context %lx to thread %d", ret_t.fctx, data->prev->id);
        data->prev->fcxt = ret_t.fctx;
    }
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

