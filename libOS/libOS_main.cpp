#include <control_struct.h>
#include <functional>
#include <string.h>
#include "panic.h"
#include "libos.h"
#include "allocator.h"
#include "sched.h"
#include "mmap.h"
#include "thread_local.h"
#include "user_thread.h"
#include "sys_format.h"

#include <vector>
#include <list>
Queue<RequestBase*> *requestQueue = nullptr;
extern "C" void __temp_libc_start_init(void);
extern "C" void __eexit(int ret);
extern "C" void __interrupt_exit();

int idleThread() {
    for (;;) {
        __asm__("pause");
        scheduler->schedule();
    }
}

int newThread(int argc, char **argv) {
    libos_print("We are in a new thread!");
    libos_print("Enabling interrupt");
    getSharedTLS()->inInterrupt->store(false);
    for (size_t i = 0; i < 100; i++) {
        libos_print("%d", i);
    }
    int ret = main(argc, argv); 
    __eexit(ret);
    return 0;
}

extern "C" int __libOS_start(libOS_control_struct *ctrl_struct) {
    if (!ctrl_struct)
        return -1;
    if (ctrl_struct->magic != CONTROL_STRUCT_MAGIC)
        return -1;
    if (!ctrl_struct->isMain)
        return -1;
    requestQueue = ctrl_struct->requestQueue;
    initPanic(ctrl_struct->panic);
    libos_print("We are inside LibOS!");

    initUnsafeMalloc(ctrl_struct->mainArgs.unsafeHeapBase, ctrl_struct->mainArgs.unsafeHeapLength);
    writeToConsole("UnsafeMalloc intialization successful.");
    libos_print("UnsafeHeap base = 0x%lx, length = 0x%lx", ctrl_struct->mainArgs.unsafeHeapBase,
                ctrl_struct->mainArgs.unsafeHeapLength);

    mmap_init(ctrl_struct->mainArgs.heapBase, ctrl_struct->mainArgs.heapLength);
    initSafeMalloc(10 * 4096);
    libos_print("Safe malloc initialization successful");

    initSyscallTable();
    scheduler_init();
    scheduler->setIdle((new UserThread(idleThread))->se);
    UserThread initThread(std::bind(newThread, ctrl_struct->mainArgs.argc, ctrl_struct->mainArgs.argv)); 
    scheduler->schedule(); 
    libos_panic("Shouldn't have reached here!");
    __asm__("ud2");
    return 0;
}

extern "C" void do_interrupt() {
    libos_print("do_interrupt!");
    __interrupt_exit();
    __asm__("ud2");
}
