#ifndef ENCLAVE_MANAGER_H
#define ENCLAVE_MANAGER_H

#include <map>
#include <stddef.h>
#include <sgx_arch.h>
#include <sgx_user.h>

using namespace std;

using vaddr = uint64_t;
#define VADDR_VOIDP(x) ((void *)x)
#define VADDR_PTR(x, t) ((t *)x)
#define PTR_VADDR(x) ((vaddr)x)

#define NUM_SSAFRAME 4

struct EnclaveThreadHandle {
    vaddr tcs;
};

class EnclaveManager {
private:
    vaddr enclaveBase;
    size_t enclaveMemoryLen;
    map<vaddr, size_t> mappings;

    secs_t secs;
public:
    EnclaveManager(vaddr base, size_t len);
    bool addPages(vaddr dest, void *src, size_t len);
    bool addPages(vaddr dest, void *src, size_t len, bool writable, bool executable, bool isTCS);
    EnclaveThreadHandle *createThread(vaddr entry);
    void makeHeap(vaddr base, size_t len);
};


struct EnclaveSegment {
    size_t fileOffset;
    void *data;
};

#endif
