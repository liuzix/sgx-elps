#ifndef ENCLAVE_MANAGER_H
#define ENCLAVE_MANAGER_H

#include <cstdint>
#include <map>
#include <memory>
#include <sgx_arch.h>
#include <sgx_user.h>
#include <stddef.h>
#include <string>
#include <signal.h>

#include "enclave_thread.h"
#include "enclave_threadpool.h"
#include "signature.h"

using namespace std;

#define SSA_FRAMESIZE_PAGE 4
#define NUM_SSA 4

extern uint64_t enclave_base, enclave_end;
extern "C" void (__aex_handler)(void)  __attribute__ ((visibility ("hidden")));

class EnclaveManager {
  private:
    vaddr enclaveBase;
    size_t enclaveMemoryLen;
    map<vaddr, size_t> mappings;

    SigstructGenerator siggen;

    secs_t secs;

    /* allocate will find a hole in mappings that can accommodate len bytes */
    vaddr allocate(size_t len);

    shared_ptr<EnclaveThreadPool> threadpool;

  public:
    /*
     * base is only a hint. In case of mmap conflicts, base might be altered
     */
    EnclaveManager(vaddr base, size_t len);

    /* You must use getBase() to calculate the dest for addPages() */
    vaddr getBase() const { return this->enclaveBase; }
    size_t getLen() const { return this->enclaveMemoryLen; }
    bool addPages(vaddr dest, void *src, size_t len);
    bool addPages(vaddr dest, void *src, size_t len, bool writable,
                  bool executable, bool isTCS);
    template <typename ThreadType>
    shared_ptr<ThreadType> createThread(vaddr entry);
    void prepareLaunch();
    vaddr makeHeap(size_t len);
    void setThreadpool(shared_ptr<EnclaveThreadPool> p) { this->threadpool = p; }
    shared_ptr<EnclaveThreadPool> getThreadpool() { return this->threadpool; }
    static void __sigaction(int n, siginfo_t *siginfo, void *ucontext);
    void dump_sigaction();
};

extern shared_ptr<EnclaveManager> manager;
#endif
