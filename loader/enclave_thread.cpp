#include "enclave_thread.h"
#include "enclave_threadpool.h"
#include "logging.h"
#include <chrono>
#include <x86intrin.h>
#include <iostream>

DEFINE_LOGGER(enclave_thread, spdlog::level::debug)
int EnclaveThread::threadCounter = 0;
extern shared_ptr<EnclaveThreadPool> threadpool;

std::atomic<int> aexCounter = 0;

std::map<uint64_t, atomic<char>> sig_flag_map;

char get_flag(uint64_t tcs) {
    char res = threadpool->sig_flag_map[tcs].exchange(0);
    return res;
}

void set_flag(uint64_t tcs, char flag) {
    console->trace("set_flag tcs: 0x{:x}", tcs);
    threadpool->sig_flag_map[tcs].store(flag);
}

extern "C" bool set_interrupt(uint64_t tcs) {
    auto tls = threadpool->thread_map[tcs]->getSharedTLS();
    bool ret = tls->inInterrupt->exchange(true);
    console->trace("old interrupt flag = {}", ret);
    return ret;
}

extern "C" bool clear_interrupt(uint64_t tcs) {
    console->trace("clear interrupt! 0x{:x}", tcs);
    auto tls = threadpool->thread_map[tcs]->getSharedTLS();
    return tls->inInterrupt->exchange(false);
}

extern "C" uint64_t do_aex(uint64_t tcs) {
    using namespace std::chrono;
    static high_resolution_clock::time_point last_interrupt;
    //console->info("do_aex: tcs = 0x{:x}", tcs);
    char dumpFlag = get_flag(tcs);
    bool intFlag = set_interrupt(tcs);
    int ret = 0;

    aexCounter++;
    if (dumpFlag)
        return 1;
    if (!intFlag) {
        if (dumpFlag) {
            console->debug("We decide to dump!");
            return 1;
        }
        else {
            duration<double> time_span = duration_cast<duration<double>>(high_resolution_clock::now() - last_interrupt);
            if (time_span.count() < 0.001) {
                clear_interrupt(tcs);
                //aexCounter++;
                return 0;
            }
            last_interrupt = high_resolution_clock::now();
            //aexCounter++;
            return 2;
        }
    }
    //aexCounter++;
    return ret;
}

void EnclaveThread::run() {
    using namespace std::chrono;
    static high_resolution_clock::time_point starttime = high_resolution_clock::now(), endtime;

    classLogger->info("entering enclave!");
    uint64_t cc1 = __rdtsc();
    __eenter(this->tcs);
    cc1 = __rdtsc() - cc1;
    endtime =  high_resolution_clock::now();
    classLogger->info("Total aex: {}, time: {}, CPU cycle: {}, jiffies: {}", aexCounter, duration<double>(endtime - starttime).count(), cc1, __jiffies);
    classLogger->info("returned from enclave! ret = {}", sharedTLS.enclave_return_val);
    print_buffer();

}

void EnclaveThread::print_buffer() {
    classLogger->info("---------- printb buffer begin ----------");
    std::cout << this->sharedTLS.buffer;
    classLogger->info("----------- printb buffer end -----------");
}

void EnclaveThread::setSwapper(SwapperManager &swapperManager) {
    this->controlStruct.requestQueue = &swapperManager.queue;
    this->controlStruct.panic = &swapperManager.panic;
}

EnclaveMainThread::EnclaveMainThread(vaddr _stack,  vaddr _tcs)
    : EnclaveThread(_stack, _tcs) 
{
    this->controlStruct.isMain = 1;
    this->sharedTLS.isMain = true;
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

void EnclaveThread::setBias(size_t len) {
    this->sharedTLS.loadBias = len;
}

void EnclaveThread::setJiffies(uint64_t *p) {
    this->sharedTLS.pjiffies = p;
}
