#ifndef ENCLAVE_MANAGER_H
#define ENCLAVE_MANAGER_H

#include <map>
#include <memory>
#include <stddef.h>
#include <sgx_arch.h>
#include <sgx_user.h>

using namespace std;

using vaddr = uint64_t;
#define VADDR_VOIDP(x) ((void *)x)
#define VADDR_PTR(x, t) ((t *)x)
#define PTR_VADDR(x) ((vaddr)x)

#define NUM_SSAFRAME 4
#define NUM_SSA 2

struct EnclaveThread {
private:
    vaddr tcs;
    vaddr stacktop;
public:
   void run();
};

class EnclaveManager {
private:
    vaddr enclaveBase;
    size_t enclaveMemoryLen;
    map<vaddr, size_t> mappings;

    secs_t secs;

    /*allocate will find a hole in mappings that can accommodate len bytes*/
    vaddr allocate(size_t len);
public:
    /* `base` is only a hint. In case of mmap conflicts, base might be altered */
    EnclaveManager(vaddr base, size_t len);
    
    /* You must use getBase() to calculate the dest for addPages() */
    vaddr getBase() const {
        return this->enclaveBase;
    }

    bool addPages(vaddr dest, void *src, size_t len);
    bool addPages(vaddr dest, void *src, size_t len, bool writable, bool executable, bool isTCS);
    unique_ptr<EnclaveThread> createThread(vaddr entry);
    void makeHeap(vaddr base, size_t len);
    bool addTCS(vaddr entry_addr);
};

#endif
