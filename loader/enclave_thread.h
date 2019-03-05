#ifndef ENCLAVE_THREAD_H
#define ENCLAVE_THREAD_H

#include <sgx_arch.h>

struct EnclaveThread {
    vaddr stack;
    vaddr entry;
    vaddr tcs;
    void run();
};

extern "C" int __eenter(vaddr tcs, vaddr stack);

#endif
