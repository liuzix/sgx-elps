#include <control_struct.h>
#include "panic.h"
#include "libos.h"

Queue<RequestBase*> *requestQueue = nullptr;

extern "C" int __libOS_start(libOS_control_struct *ctrl_struct) {
    if (!ctrl_struct)
        return -1;
    if (ctrl_struct->magic != CONTROL_STRUCT_MAGIC)
        return -1;
    if (!ctrl_struct->isMain)
        return -1;

    requestQueue = ctrl_struct->requestQueue;
    initPanic(ctrl_struct->panic);
    libos_panic("panic test!");
	libos_panic("second");
	__asm__ ("ud2");   //commit suicide
    int ret = main(ctrl_struct->mainArgs.argc, ctrl_struct->mainArgs.argv);
    char buf[256];
    sprintf(buf, "Return value of main: %d", ret);
    libos_panic(buf);
    return ret;
}
