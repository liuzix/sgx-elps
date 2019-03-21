#include "user_thread.h"
#include "thread_local.h"
#include "mmap.h"

UserThread::UserThread(function<int(void)> _entry) {
    this->entry = _entry;
    
    char *stack = (char *)libos_mmap(NULL, STACK_SIZE);
    stack += (STACK_SIZE - 16);
    this->sp = stack;
}

UserThread::UserThread() {
     
}
