#include "enclave_manager.h"

#include <fcntl.h>
#include <spdlog/spdlog.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <libOS_tls.h>
#include "enclave_thread.h"

#define stack_start(addr) (addr + SGX_PAGE_SIZE)
#define ssa_start(addr) (addr + SGX_PAGE_SIZE)
#define enclave_offset(addr) (addr - enclaveBase)

DEFINE_LOGGER(EnclaveManager, spdlog::level::info)
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

static void *getZeroPage() {
    static void *zeroPage = nullptr;
    if (zeroPage == nullptr) {
        int fd = open("/dev/zero", O_RDWR); 
        zeroPage = mmap (0, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_FILE, fd, 0);
        if (zeroPage == MAP_FAILED) {
            console->error("Cannot map /dev/zero");
            exit(-1);
        }
    }

    return zeroPage;
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
    secs.ssaframesize = SSA_FRAMESIZE_PAGE;
    secs.attributes = SGX_FLAGS_MODE64BIT | SGX_FLAGS_DEBUG;
    secs.xfrm = 3;

    struct sgx_enclave_create param = {.src = (uint64_t)(&this->secs)};

    int ret = ioctl(deviceHandle(), SGX_IOC_ENCLAVE_CREATE, &param);
    if (ret) {
        console->error("Creating enclave ioctl failed. error code = {}.", ret);
        exit(-1);
    }

    siggen.doEcreate(this->enclaveMemoryLen, SSA_FRAMESIZE_PAGE);
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
        console->critical("Enclave begin = 0x{:x}, len = 0x{:x}",
                          this->enclaveBase, this->enclaveMemoryLen);
        exit(-1);
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
    exit(-1);
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
            exit(-1);
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

/*
 * Stack layout | guard page | stack | guard page |
 * TCS+SSA+TLS layout | TCS | SSA 1 page 1 | SSA 1 page 2 | ... | guard page | TLS |
 */
template <typename ThreadType>
shared_ptr<ThreadType> EnclaveManager::createThread(vaddr entry_addr) {

    size_t stack_len = (THREAD_STACK_NUM + 2) * SGX_PAGE_SIZE;
    size_t ssa_len = secs.ssaframesize * NUM_SSA * SGX_PAGE_SIZE + SGX_PAGE_SIZE;
    size_t tcs_len = SGX_PAGE_SIZE;
    size_t tls_len = SGX_PAGE_SIZE;

    void *stack_mem = mmap(NULL, stack_len, PROT_READ | PROT_WRITE,
                            MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    void *joint_mem = mmap(NULL, tcs_len + ssa_len + tls_len, PROT_READ | PROT_WRITE,
                            MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);

    if (stack_mem == MAP_FAILED || joint_mem == MAP_FAILED) {
        console->error("thread_mem mmap failed");
        exit(-1);
    }

    vaddr stack_enclave = allocate(stack_len);
    if (!addPages(stack_enclave, stack_mem, stack_len)) {
        console->error("Adding pages of stack failed");
        exit(-1);
    }

    vaddr joint_enclave = allocate(tcs_len + ssa_len + tls_len);
    vaddr tcs_enclave = joint_enclave;
    vaddr ssa_enclave = joint_enclave + tcs_len;
    vaddr tls_enclave = ssa_enclave + ssa_len;

    void *tcs_mem = joint_mem;
    void *ssa_mem = (void *)((char *)tcs_mem + tcs_len);
    void *tls_mem = (void *)((char *)ssa_mem + ssa_len);

    tcs_t *tcs = (tcs_t *)((char *)tcs_mem);
    memset(tcs, 0, tcs_len);
    tcs->ossa = enclave_offset(ssa_enclave);
    tcs->nssa = NUM_SSA;
    tcs->oentry = enclave_offset(entry_addr);
    tcs->ofsbase = enclave_offset(tls_enclave);
    tcs->ogsbase = tcs->ofsbase;
    tcs->fslimit = 0xfff;
    tcs->gslimit = 0xfff;

    enclave_tls *tls = (enclave_tls *)((char *)tls_mem);
    memset(tls, 0, tls_len);
    tls->enclave_size = enclaveMemoryLen;
    tls->tcs_offset = enclave_offset(tcs_enclave);
    tls->initial_stack_offset = enclave_offset(stack_start(stack_enclave));
    tls->ssa = (void *)ssa_enclave;
    tls->stack = (void *)(stack_start(stack_enclave));

    vaddr stack_high = (vaddr)tls->stack + THREAD_STACK_NUM * SGX_PAGE_SIZE;

    auto ret = make_shared<ThreadType>(stack_high, tcs_enclave);
    tls->libOS_data = ret->getSharedTLS();


    if (!addPages(tcs_enclave, tcs_mem, tcs_len, true, true, true)) {
        console->error("Adding pages of TCS failed");
        exit(-1);
    }

    if (!addPages(ssa_enclave, ssa_mem, ssa_len + tls_len)) {
        console->error("Adding pages of ssa & tls failed");
        exit(-1);
    }

    munmap(stack_mem, stack_len);
    munmap(joint_mem, tcs_len + ssa_len + tls_len);
    console->trace("Adding tcs succeed!");

    vaddr injectionStack = this->allocate(4096);
    if (!addPages(injectionStack, getZeroPage(), 4096)) {
        console->error("Adding preemtion injection stack failed");
        exit(-1);
    }

    ret->getSharedTLS()->preempt_injection_stack = injectionStack;
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

    for (size_t offset = 0; offset < len; offset += 4096) {
        if (!this->addPages(heapBase + offset, getZeroPage(), 4096))
            return 0;
    }

    return heapBase;
}

template
shared_ptr<EnclaveMainThread> EnclaveManager::createThread(vaddr entry_addr);

template
shared_ptr<EnclaveThread> EnclaveManager::createThread(vaddr entry_addr);
