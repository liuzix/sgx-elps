#ifndef ENCLAVE_THREAD_H
#define ENCLAVE_THREAD_H

#include <sgx_arch.h>
#include <control_struct.h>
#include <atomic>

struct EnclaveThread {
    vaddr stack;
    vaddr entry;
    vaddr tcs;
    void run();
};

extern "C" int __eenter(vaddr tcs, vaddr stack, libOS_control_struct *);


#endif
