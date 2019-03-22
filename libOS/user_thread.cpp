#include "user_thread.h"
#include "thread_local.h"
#include "mmap.h"

void __entry_helper(transfer_t transfer) {
    UserThread *thread = (UserThread *)transfer.data;
    thread->entry();
}

UserThread::UserThread(function<int(void)> _entry) {
    this->entry = _entry;
    
    char *stack = (char *)libos_mmap(NULL, STACK_SIZE);
    stack += (STACK_SIZE - 16);

    this->fcxt = make_fcontext(stack, STACK_SIZE, __entry_helper);
}

void UserThread::jumpTo() {
    jump_fcontext(this->fcxt, this);
}
