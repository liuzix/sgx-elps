#include "enclave_thread.h"
#include "enclave_threadpool.h"
#include "enclave_manager.h"
#include "logging.h"
#include <chrono>
#include <x86intrin.h>
#include <iostream>
#include <sys/ioctl.h>

DEFINE_LOGGER(enclave_thread, spdlog::level::debug)
int EnclaveThread::threadCounter = 0;

std::atomic<int> aexCounter = 0;

std::map<uint64_t, atomic<char>> sig_flag_map;
extern std::atomic_flag __in_enclave;

/*
 * We keep these functions outside the class because we want it to
 * in assembly.
 */
char get_flag(uint64_t tcs) {
    char res = manager->getThreadpool()->sig_flag_map[tcs].exchange(0);
    return res;
}

void set_flag(uint64_t tcs, char flag) {
    console->trace("set_flag tcs: 0x{:x}", tcs);
    manager->getThreadpool()->sig_flag_map[tcs].store(flag);
}

extern "C" bool set_interrupt(uint64_t tcs) {
    auto tls = manager->getThreadpool()->thread_map[tcs]->getSharedTLS();
    bool ret = tls->inInterrupt->exchange(true);
    console->trace("old interrupt flag = {}", ret);
    return ret;
}

extern "C" bool clear_interrupt(uint64_t tcs) {
    console->trace("clear interrupt! 0x{:x}", tcs);
    auto tls = manager->getThreadpool()->thread_map[tcs]->getSharedTLS();
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

static int deviceHandle() {
    static int fd = -1;
    if (fd >= 0)
        return fd;
    fd = open("/dev/isgx", O_RDWR);
    if (fd < 0) {
        console->error("Opening /dev/isgx failed: {}.", strerror(errno));
        console->info("Make sure you have the Intel SGX driver installed.");
        exit(-1);
    }

    return fd;
}

struct sgx_user_data {
	unsigned long load_bias;
	unsigned long tcs_addr;
};

#define SGX_IOC_ENCLAVE_SET_USER_DATA \
	_IOW(SGX_MAGIC, 0X04, struct sgx_user_data)

void EnclaveThread::run() {
    using namespace std::chrono;
    static high_resolution_clock::time_point starttime = high_resolution_clock::now(), endtime;

    uint64_t cc1 = __rdtsc();
    //sgx_user_data u_data = {.load_bias = this->sharedTLS.loadBias, .tcs_addr = this->tcs};
    sgx_user_data u_data = {.load_bias = 0, .tcs_addr = 0};
    ioctl(deviceHandle(), SGX_IOC_ENCLAVE_SET_USER_DATA, &u_data);

    for (;;) {
        classLogger->info("entering enclave!");
        __eenter(this->tcs);
        classLogger->info("exiting enclave!");
        this->controlStruct.isMain = false;
        if (sharedTLS.enclave_return_val != 0x1000)
            break;
        this->threadPool->idleBlock();
    };

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
    this->controlStruct.timeStamp = &swapperManager.timeStamp;
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

void EnclaveMainThread::setUserTLS(vaddr base, size_t len) {
    this->controlStruct.mainArgs.tlsBase = (void *)base;
    this->controlStruct.mainArgs.tlsSize = len;
}
