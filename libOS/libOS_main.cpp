#include <control_struct.h>

Queue<RequestBase *> *requestQueue = nullptr;
extern "C" int main(int argc, char **argv);

extern "C" int __libOS_start(libOS_control_struct *ctrl_struct) {
    if (!ctrl_struct)
        return -1;
    if (ctrl_struct->magic != CONTROL_STRUCT_MAGIC)
        return -1;
    if (!ctrl_struct->isMain)
        return -1;
    requestQueue = ctrl_struct->requestQueue;
    int ret = main(ctrl_struct->mainArgs.argc, ctrl_struct->mainArgs.argv);
    return ret;
}
