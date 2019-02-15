#ifndef ENCLAVE_MANAGER_H
#define ENCLAVE_MANAGER_H

#include <stddef.h>
#include <sgx_arch.h>
#include <sgx_user.h>

class EnclaveManager {
private:
    void *enclaveBase;
    size_t enclaveMemoryLen;
public:
    EnclaveManager(void *base, size_t len);
};


struct EnclaveSegment {
    size_t fileOffset;
    void *data;
};

#endif
