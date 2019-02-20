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

EnclaveManager::EnclaveManager(vaddr base, size_t len)
{
    for (this->enclaveMemoryLen = 1; this->enclaveMemoryLen < len;)
        this->enclaveMemoryLen <<= 1;

    this->enclaveBase = (vaddr)mmap(VADDR_VOIDP(base), this->enclaveMemoryLen,
            PROT_NONE, MAP_SHARED, deviceHandle(), 0); 
    if (this->enclaveBase == (vaddr)MAP_FAILED) {
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

bool EnclaveManager::addPages(vaddr dest, void *src, size_t len)
{
    return this->addPages(dest, src, len, true, true, false);
}

bool EnclaveManager::addPages(vaddr dest, void *src, size_t len,
        bool writable, bool executable, bool isTCS)
{
    if (dest < enclaveBase || dest + len > this->enclaveBase + this->enclaveMemoryLen) {
        console->critical("Attempted enclave mapping (addr: 0x{:x}, len: 0x{:x}) is out of bounds",
                dest, len);
        return false;
    }

    auto it = this->mappings.upper_bound(dest);
    // make sure there's no conflict with the following mapping
    if (it != this->mappings.end() && dest + len > it->first)
        goto conflict;
    // make sure there's no conflict with the previous mapping
    if (it != this->mappings.begin()) {
        auto prev = --it;
        if (prev->first + prev->second > dest)
            goto conflict;
    }
    goto good;

conflict:
    console->critical("Attempted enclave mapping (addr: 0x{:x}, len: 0x{:x})"
        " conflicts with existing mappings", dest, len);
    return false;

good:
    sec_info_t sec_info;
    memset(&sec_info, 0, sizeof(sec_info_t));
    sec_info.flags = ENCLAVE_PAGE_READ;
    sec_info.flags |= (writable ? ENCLAVE_PAGE_WRITE : 0);
    sec_info.flags |= (executable ? ENCLAVE_PAGE_EXECUTE : 0);
    sec_info.flags |= (isTCS ? ENCLAVE_PAGE_THREAD_CONTROL : SI_FLAG_REG); 

    for (size_t i = 0; SGX_PAGE_SIZE * i < len; i++) {
        struct sgx_enclave_add_page addp = {0, 0, 0, 0};
        
        addp.addr = dest + SGX_PAGE_SIZE * i;
        addp.src = (uint64_t)src + SGX_PAGE_SIZE * i;
        addp.mrmask = 0xFFFF;
        addp.secinfo = (uint64_t)&sec_info;

        int ret = ioctl(deviceHandle(), SGX_IOC_ENCLAVE_ADD_PAGE, &addp);
        if (ret) {
            console->critical("Mapping 0x{:x} to 0x{:x} failed, error code = {}.",
                    addp.src, addp.addr, errno);
            return false;
        }

        console->trace("Mapping 0x{:x} to 0x{:x} succeeded.", addp.src, addp.addr);
    }

    this->mappings[dest] = len;
    return true;

}

