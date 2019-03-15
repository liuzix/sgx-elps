#ifndef ENCLAVE_THREAD_H
#define ENCLAVE_THREAD_H

#include <sgx_arch.h>
#include <swapper_interface.h>
#include <control_struct.h>
#include <libOS_tls.h>
#include <atomic>

class EnclaveThread {
private:
    //vaddr stack;
    //vaddr entry;
    vaddr tcs;

    libOS_shared_tls sharedTLS;
protected:
    libOS_control_struct controlStruct;
public:
    EnclaveThread(vaddr _stack, vaddr _entry, vaddr _tcs)
        : tcs(_tcs) 
    {
        sharedTLS = {};
        sharedTLS.next_entry = _entry;
        sharedTLS.enclave_stack = _stack;
    }
    void setSwapper(SwapperManager &swapperManager); 
    void writeToConsole(const char *msg, size_t n);
    void run();
};

class EnclaveMainThread: public EnclaveThread {
public:
    EnclaveMainThread(vaddr _stack, vaddr _entry, vaddr _tcs);
   
    void setArgs(int argc, char **argv);
    void setHeap(vaddr base, size_t len);
};

extern "C" int __eenter(vaddr tcs, vaddr stack, libOS_control_struct *);


#endif
