#include "thread_local.h"
#include <libOS_tls.h>

uint64_t readQWordFromGS(size_t offset) {
    uint64_t ret;
    asm volatile ("movq %%gs:(%1), %0"
            : "=r" (ret): "r"(offset));
    return ret;
}

bool disableInterrupt() {
    return getSharedTLS()->inInterrupt->exchange(true);
}

void enableInterrupt() {
    return getSharedTLS()->inInterrupt->store(false);
}

libOS_shared_tls *getSharedTLS() {
    return (libOS_shared_tls *)readTLSField(libOS_data);
}


