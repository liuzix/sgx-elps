#ifndef ENCLAVE_MANAGER_H
#define ENCLAVE_MANAGER_H

#include <cstdint>
#include <map>
#include <memory>
#include <sgx_arch.h>
#include <sgx_user.h>
#include <stddef.h>
#include <string>

#include "signature.h"
using namespace std;

#define NUM_SSAFRAME 4
#define NUM_SSA 2

struct EnclaveThread {
    vaddr entry;
    vaddr stack;

    void run();
};

class EnclaveManager {
  private:
    vaddr enclaveBase;
    size_t enclaveMemoryLen;
    map<vaddr, size_t> mappings;

    SigstructGenerator siggen;

    secs_t secs;

    /*allocate will find a hole in mappings that can accommodate len bytes*/
    vaddr allocate(size_t len);

  public:
    /* `base` is only a hint. In case of mmap conflicts, base might be altered
     */
    EnclaveManager(vaddr base, size_t len);

    /* You must use getBase() to calculate the dest for addPages() */
    vaddr getBase() const { return this->enclaveBase; }

    bool addPages(vaddr dest, void *src, size_t len);
    bool addPages(vaddr dest, void *src, size_t len, bool writable,
                  bool executable, bool isTCS);
    unique_ptr<EnclaveThread> createThread(vaddr entry);

    void prepareLaunch();
    void makeHeap(vaddr base, size_t len);
};

#endif
