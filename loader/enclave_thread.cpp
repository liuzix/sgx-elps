#include "enclave_thread.h"
#include "logging.h"

/*
void EnclaveThread::writeToConsole(const char *msg, size_t n) {
    if (n >= 256) n =  255;

    this->controlStruct.panic->lock.lock();
    char *ptr = this->controlStruct.panic->panicBuf;
    size_t i = 0;
    while (*msg && i < n) {
        *ptr = *msg;
        msg++;
        ptr++;
        i++;
    }
    *ptr = 0;
    
    auto req = new (this->controlStruct.panic->requestBuf) DebugRequest;
    this->controlStruct.requestQueue->push(req); 
    req->waitOnDone(1000000000);
    this->controlStruct.panic->lock.unlock();
}
*/

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
