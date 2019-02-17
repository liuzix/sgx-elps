#ifndef ENCLAVE_MANAGER_H
#define ENCLAVE_MANAGER_H

#include <stddef.h>
#include <sgx_arch.h>
#include <sgx_user.h>

#define NUM_SSAFRAME 4

struct EnclaveThreadHandle {
    void *tcs;
};

class EnclaveManager {
private:
    void *enclaveBase;
    size_t enclaveMemoryLen;

    secs_t secs;
public:
    EnclaveManager(void *base, size_t len);
    void addPages(void *dest, void *src, size_t len);
    EnclaveThreadHandle *createThread(void *entry);
    void makeHeap(void *base, size_t len);
};


struct EnclaveSegment {
    size_t fileOffset;
    void *data;
};

#endif
