#ifndef ENCLAVE_THREAD_H
#define ENCLAVE_THREAD_H

#include <sgx_arch.h>
#include <swapper_interface.h>
#include <control_struct.h>
#include <spdlog/spdlog.h>
#include <csignal>
#include <libOS_tls.h>
#include <atomic>
#include <map>

#include "ssa_dump.h"

extern "C" void (*__back)(void);

class EnclaveThread {
private:
    //vaddr stack;
    vaddr tcs;

    libOS_shared_tls sharedTLS;

protected:
    libOS_control_struct controlStruct;
public:
    EnclaveThread(vaddr _stack, vaddr _tcs)
        : tcs(_tcs)
    {
        sharedTLS = {};
        sharedTLS.next_exit = (uint64_t)&__back;
        sharedTLS.enclave_stack = _stack;
        sharedTLS.controlStruct = &this->controlStruct;
        set_flag((uint64_t)_tcs, 0);
    }
    void setSwapper(SwapperManager &swapperManager); 
    //void writeToConsole(const char *msg, size_t n);
    libOS_shared_tls *getSharedTLS() {
        return &this->sharedTLS;
    }
    void run();
    vaddr getTcs() {return this->tcs;}
};

class EnclaveMainThread: public EnclaveThread {
public:
    EnclaveMainThread(vaddr _stack, vaddr _tcs);
   
    void setArgs(int argc, char **argv);
    void setHeap(vaddr base, size_t len);
};

extern "C" int __eenter(vaddr tcs);


#endif
