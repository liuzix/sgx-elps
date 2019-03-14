#include "enclave_thread.h"
#include "logging.h"

void EnclaveThread::run() {
    console->info("entering enclave!");
    int ret = __eenter(this->tcs, this->stack, &this->controlStruct);
    console->info("returned from enclave! ret = {}", ret);
}

void EnclaveThread::setSwapper(SwapperManager &swapperManager) {
    this->controlStruct.requestQueue = &swapperManager.queue;
    this->controlStruct.panic = &swapperManager.panic;
}

EnclaveMainThread::EnclaveMainThread(vaddr _stack, vaddr _entry, vaddr _tcs)
    : EnclaveThread(_stack, _entry, _tcs) 
{
    this->controlStruct.isMain = true;
}
    
void EnclaveMainThread::setArgs(int argc, char **argv) {
    this->controlStruct.mainArgs.argc = argc;
    this->controlStruct.mainArgs.argv = argv;
}

void EnclaveMainThread::setHeap(vaddr base, size_t len) {
    this->controlStruct.mainArgs.heapBase = base;
    this->controlStruct.mainArgs.heapLength = len;
}
