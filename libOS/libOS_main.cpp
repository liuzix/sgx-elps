#include <control_struct.h>
#include <functional>
#include <string.h>
#include "panic.h"
#include "libos.h"
#include "allocator.h"
#include "mmap.h"
#include "thread_local.h"
#include "user_thread.h"
#include "sys_format.h"

#include <vector>
#include <list>
Queue<RequestBase*> *requestQueue = nullptr;
extern "C" void __temp_libc_start_init(void);
extern "C" void __eexit(int ret);

int newThread(int argc, char **argv) {
    libos_print("We are in a new thread!");
    for (int i = 0; i < 10; i++)
        libos_print("%d", i);
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
    //testSafeMalloc();
    
    //libos_print("trying to call libc_start_init");
    //__temp_libc_start_init();
   
    //int *p = 0x0;
    //*p = 5;
    //int ret = main(ctrl_struct->mainArgs.argc, ctrl_struct->mainArgs.argv);

    UserThread initThread(std::bind(newThread, ctrl_struct->mainArgs.argc, ctrl_struct->mainArgs.argv)); 
    initThread.jumpTo();

    libos_panic("Shouldn't have reached here!");
    return 0;
}
