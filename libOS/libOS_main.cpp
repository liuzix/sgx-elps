#include <control_struct.h>
#include "panic.h"
#include "libos.h"
#include "allocator.h"

Queue<RequestBase*> *requestQueue = nullptr;
Allocator *unsafeAllocator = nullptr;

extern "C" int __libOS_start(libOS_control_struct *ctrl_struct) {
    if (!ctrl_struct)
        return -1;
    if (ctrl_struct->magic != CONTROL_STRUCT_MAGIC)
        return -1;
    if (!ctrl_struct->isMain)
        return -1;
    requestQueue = ctrl_struct->requestQueue;
    initPanic(ctrl_struct->panic);
    writeToConsole("We are inside libOS!", 255);

    
    size_t allocatorSize = (sizeof(Allocator) + 15) & (0xF);
    unsafeAllocator = new (ctrl_struct->mainArgs.unsafeHeapBase) Allocator
        (ctrl_struct->mainArgs.unsafeHeapLength - allocatorSize,
         (vaddr)ctrl_struct->mainArgs.unsafeHeapBase + allocatorSize);

    int ret = main(ctrl_struct->mainArgs.argc, ctrl_struct->mainArgs.argv);
    return ret;
}
