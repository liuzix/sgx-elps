#include "enclave_thread.h"
#include "logging.h"

int EnclaveThread::threadCounter = 0;

void EnclaveThread::run() {
    console->info("entering enclave!");
    __eenter(this->tcs);
    console->info("returned from enclave! ret = {}", sharedTLS.enclave_return_val);
    //this->writeToConsole("test1", 5);
    //this->writeToConsole("test2", 5);

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

void EnclaveMainThread::setHeap(vaddr base, size_t len) {
    this->controlStruct.mainArgs.heapBase = base;
    this->controlStruct.mainArgs.heapLength = len;
}

void EnclaveMainThread::setUnsafeHeap(void *base, size_t len) {
    console->info("Set unsafeheap base = 0x{:x}, length = 0x{:x}",
                  (vaddr)base, len);
    this->controlStruct.mainArgs.unsafeHeapBase = base;
    this->controlStruct.mainArgs.unsafeHeapLength = len;
}

void EnclaveMainThread::setBias(size_t len) {
    this->sharedTLS.loadBias = len;
}
