#include <control_struct.h>
#include <string.h>
#include "panic.h"
#include "libos.h"
#include "allocator.h"
#include "mmap.h"
#include "thread_local.h"
#include "sys_format.h"

#include <vector>
#include <list>
Queue<RequestBase*> *requestQueue = nullptr;
extern "C" void __temp_libc_start_init(void);

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

    libos_print("testVal p: %p", (uint64_t *)(0x3555e8 + getSharedTLS()->loadBias));
    uint64_t testVal = *(uint64_t *)(0x3555e8 + getSharedTLS()->loadBias);
    libos_print("testVal: %p", testVal);
    initUnsafeMalloc(ctrl_struct->mainArgs.unsafeHeapBase, ctrl_struct->mainArgs.unsafeHeapLength);    
    writeToConsole("UnsafeMalloc intialization successful.");
    libos_print("UnsafeHeap base = 0x%lx, length = 0x%lx", ctrl_struct->mainArgs.unsafeHeapBase,
                ctrl_struct->mainArgs.unsafeHeapLength);

    mmap_init(ctrl_struct->mainArgs.heapBase, ctrl_struct->mainArgs.heapLength);
    initSafeMalloc(10 * 4096);
    libos_print("Safe malloc initialization successful");

    initSyscallTable();
    //testSafeMalloc();
    
    libos_print("trying to call libc_start_init");
    __temp_libc_start_init();
   
    int *p = 0x0;
    *p = 5;
    int ret = main(ctrl_struct->mainArgs.argc, ctrl_struct->mainArgs.argv);
    return ret;
}
