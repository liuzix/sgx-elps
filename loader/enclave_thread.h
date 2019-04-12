#ifndef ENCLAVE_THREAD_H

#define ENCLAVE_THREAD_H

#include <sgx_arch.h>
#include <swapper.h>
#include <control_struct.h>
#include <csignal>
#include <logging.h>
#include <libOS_tls.h>
#include <atomic>
#include <map>
#include <sys/mman.h>

#include <ssa_dump.h>
#include <request.h>
#include "../libOS/allocator.h"

extern "C" void (*__back)(void);
extern "C" void (*__interrupt_back)(void);

extern "C" bool set_interrupt(uint64_t tcs);
extern "C" bool clear_interrupt(uint64_t tcs);

extern std::atomic_int numActiveThread;
class EnclaveThread;
class EnclaveThreadPool;

class EnclaveThread {
private:
    static int threadCounter;
protected:
    //vaddr stack;
    vaddr tcs;
    libOS_shared_tls sharedTLS;
    libOS_control_struct controlStruct;
    EnclaveThreadPool *threadPool;
    DEFINE_MEMBER_LOGGER("EnclaveThread", spdlog::level::trace)
public:
    EnclaveThread(vaddr _stack, vaddr _tcs)
        : tcs(_tcs)
    {
        controlStruct.isMain = false;
        sharedTLS.isMain = 0;
        int threadID = threadCounter++;
        sharedTLS.next_exit = (uint64_t)&__back;
        sharedTLS.interrupt_exit = (uint64_t)&__interrupt_back;
        sharedTLS.enclave_stack = _stack;
        sharedTLS.controlStruct = &this->controlStruct;
        sharedTLS.threadID = threadID;
        sharedTLS.inInterrupt = new std::atomic_bool(true);
        sharedTLS.interrupt_stack = (uint64_t)malloc(4096) + 4096 - 16;
        //sharedTLS.request_obj = (uint64_t)new SwapRequest();
        sharedTLS.request_obj = 0;
        set_flag((uint64_t)_tcs, 0);
        mlock(&this->sharedTLS, 0x1000);
    }
    ~EnclaveThread() {
        //free((SwapRequest *)sharedTLS.request_obj);
    }
    void setSwapper(SwapperManager &swapperManager); 
    //void writeToConsole(const char *msg, size_t n);
    libOS_shared_tls *getSharedTLS() {
        return &this->sharedTLS;
    }
    void run();
    void setBias(size_t bias);
    vaddr getTcs() { return this->tcs; }
    void setJiffies(uint64_t *p);
    void print_buffer();

    friend class EnclaveThreadPool;
};
extern std::map<uint64_t, shared_ptr<EnclaveThread>> thread_map;

class EnclaveMainThread: public EnclaveThread {
public:
    EnclaveMainThread(vaddr _stack, vaddr _tcs);

    void setArgs(int argc, char **argv);
    void setHeap(vaddr base, size_t len);
    void setUnsafeHeap(void *base, size_t len);
    void setEnvs(char **envp);
    void setAux(size_t *auxv);
};

extern "C" int __eenter(vaddr tcs);


#endif
