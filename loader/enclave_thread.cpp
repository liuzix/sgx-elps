#include "enclave_thread.h"
#include "logging.h"

DEFINE_LOGGER(enclave_thread, spdlog::level::trace)
int EnclaveThread::threadCounter = 0;

std::map<uint64_t, shared_ptr<EnclaveThread>> thread_map;


char get_flag(uint64_t rbx) {
    char res = sig_flag_map[rbx].exchange(0);
    return res;
}

void set_flag(uint64_t rbx, char flag) {
    sig_flag_map[rbx].store(flag);
}

std::map<uint64_t, atomic<char>> sig_flag_map;
extern "C" bool set_interrupt(uint64_t tcs) {
    auto tls = thread_map[tcs]->getSharedTLS();    
    bool ret = tls->inInterrupt->exchange(true);
    console->trace("old interrupt flag = {}", ret);
    return ret;
}

extern "C" bool clear_interrupt(uint64_t tcs) {
    console->trace("clear interrupt! 0x{:x}", tcs);
    auto tls = thread_map[tcs]->getSharedTLS();    
    return tls->inInterrupt->exchange(false);
}

extern "C" uint64_t do_aex(uint64_t tcs) {
    //console->info("do_aex: tcs = 0x{:x}", tcs);
    char dumpFlag = get_flag(tcs);
    bool intFlag = set_interrupt(tcs);
    int ret = 0;
    if (!intFlag) {
        if (dumpFlag) {
            console->debug("We decide to dump!");
            ret =  1;
        }
        else ret = 2;
    }
    return ret;
}

void EnclaveThread::run() {
    classLogger->info("entering enclave!");
    __eenter(this->tcs);
    classLogger->info("returned from enclave! ret = {}", sharedTLS.enclave_return_val);

}

void EnclaveThread::setSwapper(SwapperManager &swapperManager) {
    this->controlStruct.requestQueue = &swapperManager.queue;
    this->controlStruct.panic = &swapperManager.panic;
}

EnclaveMainThread::EnclaveMainThread(vaddr _stack,  vaddr _tcs)
    : EnclaveThread(_stack, _tcs) 
{
    this->controlStruct.isMain = true;
}
    
void EnclaveMainThread::setArgs(int argc, char **argv) {
    this->controlStruct.mainArgs.argc = argc;
    this->controlStruct.mainArgs.argv = argv;
}

void EnclaveMainThread::setEnvs(char **envp) {
    int i = 0;

    this->controlStruct.mainArgs.envp = envp;
    for (i = 1; envp[i]; i++);

    this->controlStruct.mainArgs.envc = i;
}

void EnclaveMainThread::setAux(size_t *auxv) {
    this->controlStruct.mainArgs.auxv = auxv;
}

void EnclaveMainThread::setHeap(vaddr base, size_t len) {
    this->controlStruct.mainArgs.heapBase = base;
    this->controlStruct.mainArgs.heapLength = len;
}

void EnclaveMainThread::setUnsafeHeap(void *base, size_t len) {
    classLogger->info("Set unsafeheap base = 0x{:x}, length = 0x{:x}",
                  (vaddr)base, len);
    this->controlStruct.mainArgs.unsafeHeapBase = base;
    this->controlStruct.mainArgs.unsafeHeapLength = len;
}

void EnclaveMainThread::setBias(size_t len) {
    this->sharedTLS.loadBias = len;
}
