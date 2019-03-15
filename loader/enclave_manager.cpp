#include "enclave_manager.h"

#include <fcntl.h>
#include <spdlog/spdlog.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <libOS_tls.h>
#include "enclave_thread.h"

extern std::shared_ptr<spdlog::logger> console;

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

EnclaveManager::EnclaveManager(vaddr base, size_t len) : siggen(&this->secs) {
    for (this->enclaveMemoryLen = 1; this->enclaveMemoryLen < len;)
        this->enclaveMemoryLen <<= 1;

    this->enclaveBase = (vaddr)mmap(VADDR_VOIDP(base), this->enclaveMemoryLen,
                                    PROT_READ | PROT_WRITE | PROT_EXEC, MAP_SHARED, deviceHandle(), 0);
    if (this->enclaveBase == (vaddr)MAP_FAILED) {
        console->error(
            "Cannot allocate virtual memory. base = 0x{:x}, len = 0x{:x}", base,
            len);
        exit(-1);
    }

    memset(&this->secs, 0, sizeof(this->secs));

    secs.base = (uint64_t)this->enclaveBase;
    secs.size = this->enclaveMemoryLen;
    secs.ssaframesize = NUM_SSAFRAME;
    secs.attributes = SGX_FLAGS_MODE64BIT | SGX_FLAGS_DEBUG;
    secs.xfrm = 3;

    struct sgx_enclave_create param = {.src = (uint64_t)(&this->secs)};

    int ret = ioctl(deviceHandle(), SGX_IOC_ENCLAVE_CREATE, &param);
    if (ret) {
        console->error("Creating enclave ioctl failed. error code = {}.", ret);
        exit(-1);
    }

    siggen.doEcreate(this->enclaveMemoryLen, NUM_SSAFRAME);
    console->info("Creating enclave successful.");
}

bool EnclaveManager::addPages(vaddr dest, void *src, size_t len) {
    return this->addPages(dest, src, len, true, true, false);
}

bool EnclaveManager::addPages(vaddr dest, void *src, size_t len, bool writable,
                              bool executable, bool isTCS) {
    len = ((len + 4095) >> 12) << 12;
    console->debug("adding len = 0x{:x}", len);
    if (dest < enclaveBase ||
        dest + len > this->enclaveBase + this->enclaveMemoryLen) {
        console->critical("Attempted enclave mapping (addr: 0x{:x}, len: "
                          "0x{:x}) is out of bounds",
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
                      " conflicts with existing mappings",
                      dest, len);
    return false;

good:
    sec_info_t sec_info;
    memset(&sec_info, 0, sizeof(sec_info_t));
    sec_info.flags = ENCLAVE_PAGE_READ;
    sec_info.flags |= (writable ? ENCLAVE_PAGE_WRITE : 0);
    sec_info.flags |= (executable ? ENCLAVE_PAGE_EXECUTE : 0);
    sec_info.flags |= (isTCS ? ENCLAVE_PAGE_THREAD_CONTROL : SI_FLAG_REG);
    if (isTCS) {
        sec_info.flags &= ~ENCLAVE_PAGE_READ;
        sec_info.flags &= ~ENCLAVE_PAGE_WRITE;
        sec_info.flags &= ~ENCLAVE_PAGE_EXECUTE;
    }

    for (size_t i = 0; SGX_PAGE_SIZE * i < len; i++) {
        struct sgx_enclave_add_page addp = {0, 0, 0, 0};

        addp.addr = dest + SGX_PAGE_SIZE * i;
        addp.src = (uint64_t)src + SGX_PAGE_SIZE * i;
        addp.mrmask = 0x0;
        addp.secinfo = (uint64_t)&sec_info;

        int ret = ioctl(deviceHandle(), SGX_IOC_ENCLAVE_ADD_PAGE, &addp);
        if (ret) {
            console->critical(
                "Mapping 0x{:x} to 0x{:x} failed, error code = {}.", addp.src,
                addp.addr, errno);
            return false;
        }

        siggen.doEadd(addp.addr - this->enclaveBase, sec_info);
        console->trace("Mapping 0x{:x} to 0x{:x} succeeded.", addp.src,
                       addp.addr);
    }

    this->mappings[dest] = len;
    return true;
}

vaddr EnclaveManager::allocate(size_t len) {
    if (this->mappings.empty())
        return this->enclaveBase;

    auto it = this->mappings.begin();
    vaddr prev = this->enclaveBase;

    for (; it != this->mappings.end(); it++) {
        if (it->first - prev > len)
            return prev;
        prev = it->first + it->second;
    }

    size_t last_len = this->enclaveBase + this->enclaveMemoryLen - prev;

    if (last_len < len) {
        console->error("Not enough space for 0x{:x}", len);
        exit(-1);
    }
    return prev;
}

template <typename ThreadType>
shared_ptr<ThreadType> EnclaveManager::createThread(vaddr entry_addr) {
    size_t thread_len =
        SGX_PAGE_SIZE * (5 + THREAD_STACK_NUM + secs.ssaframesize * NUM_SSA);
    void *thread_mem = mmap(NULL, thread_len, PROT_READ | PROT_WRITE,
                            MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (thread_mem == MAP_FAILED) {
        console->error("thread_mem mmap failed");
        exit(-1);
    }
    vaddr base = allocate(thread_len);
    vaddr offset = base - enclaveBase;

    tcs_t *tcs =
        (tcs_t *)((char *)thread_mem + SGX_PAGE_SIZE * (2 + THREAD_STACK_NUM));
    memset(tcs, 0, SGX_PAGE_SIZE);
    tcs->ossa = SGX_PAGE_SIZE * (3 + THREAD_STACK_NUM) + offset;
    tcs->nssa = NUM_SSA;
    tcs->oentry = entry_addr - this->enclaveBase;
    tcs->ofsbase = offset + thread_len - SGX_PAGE_SIZE;
    tcs->ogsbase = tcs->ofsbase;
    tcs->fslimit = 0xfff;
    tcs->gslimit = 0xfff;


    enclave_tls *tls =
        (enclave_tls *)((char *)thread_mem + thread_len - SGX_PAGE_SIZE);
    memset(tls, 0, SGX_PAGE_SIZE);
    tls->enclave_size = enclaveMemoryLen;
    tls->tcs_offset = offset + SGX_PAGE_SIZE * (2 + THREAD_STACK_NUM);
    tls->initial_stack_offset = offset + SGX_PAGE_SIZE;
    tls->ssa = (void *)(base + SGX_PAGE_SIZE * (3 + THREAD_STACK_NUM));
    tls->stack = (void *)(base + SGX_PAGE_SIZE);
    
    size_t tmp = SGX_PAGE_SIZE * (2 + THREAD_STACK_NUM);
    vaddr stack = (vaddr)tls->stack + THREAD_STACK_NUM * SGX_PAGE_SIZE;
    vaddr tcs_enclave = base + tmp;
    auto ret = make_shared<ThreadType>(stack, tcs_enclave);
    tls->libOS_data = ret->getSharedTLS();

    if (!addPages(base, thread_mem, tmp)) {
        console->error("Adding pages before TCS failed");
        exit(-1);
    }
    if (!addPages(base + tmp, (char *)thread_mem + tmp, SGX_PAGE_SIZE, true,
                  true, true)) {
        console->error("Adding TCS page failed");
        exit(-1);
    }

    tmp += SGX_PAGE_SIZE;
    if (!addPages(base + tmp, (char *)thread_mem + tmp, thread_len - tmp)) {
        console->error("Adding pages after TCS failed");
        exit(-1);
    }

    munmap(thread_mem, thread_len);
    console->trace("Adding tcs succeed!");

    return ret;
}

void EnclaveManager::prepareLaunch() {
    siggen.digestFinal();
    auto sigstruct = siggen.getSigstruct();

    TokenGetter getter("/var/run/aesmd/aesm.socket");
    auto token = getter.getToken(sigstruct);

    struct sgx_enclave_init initp = {0, 0, 0};
    initp.addr = this->enclaveBase;
    initp.sigstruct = (vaddr)sigstruct;
    initp.einittoken = (vaddr)token;

    int ret = ioctl(deviceHandle(), SGX_IOC_ENCLAVE_INIT, &initp);
    if (ret < 0) {
        console->error("Enclave Init failed: {}", strerror(errno));
        exit(-1);
    } else if (ret > 0) {
        console->error("Enclave Init failed: {}", ret);
        exit(-1);
    }

    console->info("Enclave Init successful!");
}

vaddr EnclaveManager::makeHeap(size_t len) {
    vaddr heapBase = this->allocate(len);
    void *zeroPage = mmap(NULL, 4096, PROT_READ | PROT_WRITE,
                          MAP_ANONYMOUS | MAP_PRIVATE, -1, 0 );
    if (zeroPage == MAP_FAILED) {
        console->error("makeHeap: mmap failed. len = {}", len);
        exit(-1);
    }
    memset(zeroPage, 0, 4096);

    for (size_t offset = 0; offset < len; offset += 4096) {
        if (!this->addPages(heapBase + offset, zeroPage, 4096))
            return 0;
    }

    return heapBase;
}

template
shared_ptr<EnclaveMainThread> EnclaveManager::createThread(vaddr entry_addr);

//template
//shared_ptr<EnclaveSlaveThread> EnclaveManager::createThread(vaddr entry_addr);
