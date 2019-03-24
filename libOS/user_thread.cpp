#include "user_thread.h"
#include "panic.h"
#include "thread_local.h"
#include "mmap.h"
#include <atomic>

std::atomic_int counter;

extern "C" void __entry_helper(transfer_t transfer) {
    UserThread *thread = (UserThread *)transfer.data;
    thread->entry();
}

/* this is just to make the debugger happy */
__attribute__((naked)) static void __clear_rbp(transfer_t) {
   __asm__("xor %rbp, %rbp;"
           "call __entry_helper;");
}

UserThread::UserThread(function<int(void)> _entry)
    : se(std::bind(&UserThread::jumpTo, this)) {
    
    this->id = counter.fetch_add(1);
    this->entry = _entry;
    
    char *stack = (char *)libos_mmap(NULL, STACK_SIZE);
    stack += (STACK_SIZE - 16);

    this->fcxt = make_fcontext(stack, STACK_SIZE, __clear_rbp);

    scheduler->enqueueTask(this->se); 
}

void UserThread::jumpTo() {
    libos_print("switching to thread %d", this->id); 
    jump_fcontext(this->fcxt, this);
}
