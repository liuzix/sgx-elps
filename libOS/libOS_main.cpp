#include <control_struct.h>

Queue<RequestBase *> *requestQueue = nullptr;
extern "C" int main(int argc, char **argv);

extern "C" int __libOS_start(libOS_control_struct *ctrl_struct) {
    int ret = main(0, nullptr);
    //if (ctrl_struct->magic != CONTROL_STRUCT_MAGIC)
    //    return -1;

    //requestQueue = ctrl_struct->requestQueue;
    return ret;
}
