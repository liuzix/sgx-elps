#include <iostream>
#include <memory>
#include <string>
#include <sys/mman.h>
#include <elfio/elfio.hpp>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <atomic>
#include <swapper.h>
#include <logging.h>
#include <x86intrin.h>
#include <ssa_dump.h>
#include <sched.h>
#include <sys/auxv.h>
#include <sys/ioctl.h>
#include "load_elf.h"
#include "enclave_manager.h"
#include "enclave_threadpool.h"
#include "signature.h"

#define UNSAFE_HEAP_LEN 0x10000000
#define SAFE_HEAP_LEN (256 * 1024 * 1024)
#define AUX_CNT 38

using namespace std;

DEFINE_LOGGER(main, spdlog::level::trace);
/* Do we really need these 2 varibles when manager is global? */
uint64_t enclave_base, enclave_end;
shared_ptr<EnclaveManager> manager;

uint64_t __jiffies = 0;

void __timer() {
    while (true) {
        __jiffies = __rdtsc();
        //__jiffies++;
    }
}

bool in_enclave(uint64_t rip) {
    console->error("rip: 0x{:x}, enclave_base: 0x{:x}, enclave_end: 0x{:x}", rip, enclave_base, enclave_end);
    return rip >= enclave_base && rip < enclave_end;
}

void *makeUnsafeHeap(size_t length) {
    void *addr = mmap(NULL, length, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0); 
    if (addr == MAP_FAILED) {
        console->error("Cannot map unsafe heap, len = 0x{:x}", length);
        exit(-1);
    }
    return addr;
}

void sig_exit() {
    exit(-1);
}

void EnclaveManager::__sigaction(int n, siginfo_t *siginfo, void *ucontext) {
    static int sigintCounter = 0;
    ucontext_t *context = (ucontext_t *)ucontext;
    uint64_t rip = context->uc_mcontext.gregs[REG_RIP];

    console->error("rip: 0x{:x}, __aex_handler: 0x{:x}", (uint64_t)rip, (uint64_t)&__aex_handler);
    /* Signal outside the enclave */
    if (n != SIGTSTP && rip != (uint64_t)__aex_handler) {
        console->error("Receive signal {} outside the enclave.", n);
        console->flush();
        exit(-1);
    }
    
    uint64_t rbx = context->uc_mcontext.gregs[REG_RBX];
    if (n == SIGTSTP) {
        for (auto thr: manager->getThreadpool()->thread_map) {
           set_flag(thr.first, 1); 
           uint64_t debugStack = (uint64_t)malloc(4096) + 4096 - 16;
           thr.second->getSharedTLS()->enclave_stack = debugStack;
        }
    } else {
        console->info("rbx in signal handler = 0x{:x}", rbx);
        set_flag(rbx, 1);
        uint64_t debugStack = (uint64_t)malloc(4096) + 4096 - 16;
        manager->getThreadpool()->thread_map[rbx]->getSharedTLS()->enclave_stack = debugStack;
    }
    /* Per-thread flag */

    if (n == SIGSEGV)
        console->error("Segmentation Fault!");
    else if (n == SIGTSTP)
        console->error("SIGTSTP received!");
    else if (n == SIGFPE)
        console->error("Floating point error!");
    else if (n == SIGINT) {
        console->error("SIGINT received");
        if (sigintCounter++ == 1)
            std::abort();
    } else
        console->error("Signal num = {}", n);
    console->flush();
}

void EnclaveManager::dump_sigaction() {
    sigset_t msk = {0};
    struct sigaction sa;

    {
        sa.sa_handler = NULL;
        sa.sa_sigaction = this->__sigaction;
        sa.sa_mask = msk;
        sa.sa_flags = SA_SIGINFO;
        sa.sa_restorer = NULL;
    }

    sigaction(SIGSEGV, &sa, NULL);
    sigaction(7, &sa, NULL);
    sigaction(SIGILL, &sa, NULL);
    sigaction(SIGFPE, &sa, NULL);
    sigaction(SIGTSTP, &sa, NULL);
}

size_t *get_curr_auxv(ELFLoader &loader) {
    /* AUX_CNT is defined in this file */
    size_t *auxv = new size_t[AUX_CNT * 2 + 2];
    int i, j;

    for (i = 1, j = 1; i < AUX_CNT * 2; i += 2, j++) {
        switch (j) {
            case AT_PHDR:
                auxv[i] = (size_t)loader.getAuxPhdr();
                break;
            case AT_ENTRY:
                auxv[i] = (size_t)loader.getAuxEntry();
                break;
            case AT_PHNUM:
                auxv[i] = (size_t)loader.getAuxPhnum();
                break;
            case AT_PHENT:
                auxv[i] = (size_t)loader.getAuxPhent();
                break;
            case AT_EXECFD:
                auxv[i] = (size_t)loader.getAuxFd();
                break;
            case AT_SYSINFO_EHDR:
                goto done;
            default:
                auxv[i] = getauxval(j);
        }
        auxv[i - 1] = j;
    }
done:
    auxv[i] = 0;
    auxv[i - 1] = 0;

    return auxv;
}

/*
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
*/
struct sgx_user_data {
	unsigned long load_bias;
	unsigned long tcs_addr;
};

#define SGX_IOC_ENCLAVE_SET_USER_DATA \
	_IOW(SGX_MAGIC, 0X04, struct sgx_user_data)

int main(int argc, char **argv, char **envp) {
    /*
     * Set up a timer outside the enclave for benchmarking
     * This is a workaround for not being able to use rdtsc inside encalve
     */
    cpu_set_t  mask;
    CPU_ZERO(&mask);
    CPU_SET(0, &mask);
    CPU_SET(1, &mask);
    sched_setaffinity(0, sizeof(mask), &mask);

    std::thread ttimer(__timer);
    ttimer.detach();

    console->set_level(spdlog::level::trace);

    if (argc < 2) {
        console->error("Usage: loader [binary file name]");
        return -1;
    }

    console->info("Welcome to the Loader");
    console->info("Start loading binary file: {}", argv[1]);

    manager = make_shared<EnclaveManager>(0x0, SAFE_HEAP_LEN * 4ULL);

    ELFLoader loader(manager);
    loader.open(argv[1]);
    loader.relocate();

    enclave_base = manager->getBase();
    enclave_end = enclave_base + manager->getLen();

    // TODO: threadpool now is a global varibale but we prefer it a member of manager
    auto threadpool = std::make_shared<EnclaveThreadPool>(&swapperManager);
    manager->setThreadpool(threadpool);
    auto thread = loader.load();
    vaddr heap = manager->makeHeap(SAFE_HEAP_LEN);
    threadpool->addMainThread(thread);


    for (int i = 0; i < 3; i++) {
        threadpool->addWorkerThread(loader.makeWorkerThread());
    }

    manager->prepareLaunch();
    /*
    sgx_user_data u_data = {.load_bias = thread->getSharedTLS()->loadBias, .tcs_addr = thread->getTcs()};
    ioctl(deviceHandle(), SGX_IOC_ENCLAVE_SET_USER_DATA, &u_data);
    */
    //char const *testArgv[] ={"hello", "world", (char *)0};
    thread->setArgs(argc - 1, (char **)(argv + 1));
    //thread->setArgs(2, (char **)testArgv);
    thread->setAux(get_curr_auxv(loader));
    thread->setEnvs(envp);
    thread->setSwapper(swapperManager);
    thread->setHeap(heap, SAFE_HEAP_LEN);
    thread->setJiffies(&__jiffies);

    void *unsafeHeap = makeUnsafeHeap(UNSAFE_HEAP_LEN);
    thread->setUnsafeHeap(unsafeHeap, UNSAFE_HEAP_LEN);

    console->info("tcs: 0x{:x}", thread->getTcs());

    swapperManager.launchWorkers();
    /* Set the sigsegv handler to dump the ssa */
    manager->dump_sigaction();

    /* launch the enclave threads */
    threadpool->launch();
    swapperManager.waitWorkers();
    console->info("Program end.");
    return 0;
}
