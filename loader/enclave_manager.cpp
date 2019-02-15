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

}
