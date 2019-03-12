#ifndef ENCLAVE_THREAD_H
#define ENCLAVE_THREAD_H

#include <sgx_arch.h>
#include <swapper_interface.h>
#include <control_struct.h>
#include <atomic>

class EnclaveThread {
private:
    vaddr stack;
    vaddr entry;
    vaddr tcs;
protected:
    libOS_control_struct controlStruct;
public:
    EnclaveThread(vaddr _stack, vaddr _entry, vaddr _tcs)
        : stack(_stack), entry(_entry), tcs(_tcs) 
    {}
    void setSwapper(SwapperManager &swapperManager); 
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
