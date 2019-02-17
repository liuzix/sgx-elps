#include "enclave_manager.h"

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <spdlog/spdlog.h>

extern std::shared_ptr<spdlog::logger> console; 

static int deviceHandle()
{
    static int fd = -1;
    
    if (fd >= 0) return fd;
    
    fd = open("/dev/isgx", O_RDWR);
    if (fd < 0) {
        console->error("Opening /dev/isgx failed: {}.", strerror(errno));
        console->info("Make sure you have the Intel SGX driver installed.");
        exit(-1);
    }

    return fd;
}

EnclaveManager::EnclaveManager(void *base, size_t len)
{
    for (this->enclaveMemoryLen = 1; this->enclaveMemoryLen < len;)
        this->enclaveMemoryLen <<= 1;

    this->enclaveBase = mmap(base, this->enclaveMemoryLen, PROT_NONE, MAP_SHARED, deviceHandle(), 0); 
    if (this->enclaveBase == MAP_FAILED) {
        console->error("Cannot allocate virtual memory. base = 0x{:x}, len = 0x{:x}",
            base, len);
        exit(-1);
    }

    memset(&this->secs, 0, sizeof(this->secs));

    secs.base = (uint64_t)this->enclaveBase;
    secs.size = this->enclaveMemoryLen;
    secs.ssaframesize = NUM_SSAFRAME;
    secs.attributes = SGX_FLAGS_MODE64BIT | SGX_FLAGS_DEBUG;
    secs.xfrm = 3;

    struct sgx_enclave_create param = { .src = (uint64_t)(&this->secs) };

    int ret = ioctl(deviceHandle(), SGX_IOC_ENCLAVE_CREATE, &param);
    if (ret) {
        console->error("Creating enclave ioctl failed. error code = {}.", ret);
        exit(-1);
    }

    console->info("Creating enclave successful.");
}
