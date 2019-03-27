#include "enclave_manager.h"

#include <iostream>
#include <memory>
#include <string>

#include <sys/mman.h>
#include <signal.h>
#include <elfio/elfio.hpp>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <atomic>
#include "swapper_interface.h"
#include "load_elf.h"
#include "signature.h"
#include "logging.h"
#include "enclave_threadpool.h"
#include <ssa_dump.h>
#include <sys/auxv.h>

#define UNSAFE_HEAP_LEN 0x10000000
#define SAFE_HEAP_LEN 0x10000000

#define AUX_CNT 38

using namespace std;

DEFINE_LOGGER(main, spdlog::level::trace);
uint64_t enclave_base, enclave_end;
shared_ptr<EnclaveThreadPool> threadpool;
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

static void __sigaction(int n, siginfo_t *, void *ucontext) {
    static int sigintCounter = 0;
    ucontext_t *context = (ucontext_t *)ucontext;
    uint64_t rip = context->uc_mcontext.gregs[REG_RIP];
    console->error("rip: 0x{:x}, __aex_handler: 0x{:x}", (uint64_t)rip, (uint64_t)&__aex_handler);
    if (rip != (uint64_t)__aex_handler) {
        console->error("Segmentation Fault !");
        console->flush();
        exit(-1);
    }
    uint64_t rbx = context->uc_mcontext.gregs[REG_RBX];
    console->info("rbx in signal handler = 0x{:x}", rbx);
    //Per-thread flag
    set_flag(rbx, 1);

    uint64_t debugStack = (uint64_t)malloc(4096) + 4096 - 16;
    threadpool->thread_map[rbx]->getSharedTLS()->enclave_stack = debugStack;
    if (n == SIGSEGV)
        console->error("Segmentation Fault!");
    else if (n == SIGINT) {
        console->error("SIGINT received");
        if (sigintCounter++ == 1)
            std::abort();
    } else
        console->error("Signal num = {}", n);
    console->flush();
}

void dump_sigaction(void) {
    sigset_t msk = {0};
    struct sigaction sa;
    {
        sa.sa_handler = NULL;
        sa.sa_sigaction = __sigaction;
        sa.sa_mask = msk;
        sa.sa_flags = SA_SIGINFO;
        sa.sa_restorer = NULL;
    }

    sigaction(SIGSEGV, &sa, NULL);
    sigaction(7, &sa, NULL);
    sigaction(SIGILL, &sa, NULL);

}

size_t *get_curr_auxv(void) {
    /* AUX_CNT is defined in this file */
    size_t *auxv = new size_t[AUX_CNT * 2 + 2];
    int i, j;

    for (i = 1, j = 1; i < AUX_CNT * 2; i += 2, j++) {
        auxv[i] = getauxval(j);
        auxv[i - 1] = j;
    }
    auxv[i] = 0;
    auxv[i - 1] = 0;

    return auxv;
}

int main(int argc, char **argv, char **envp) {
    console->set_level(spdlog::level::trace);
    if (argc < 2) {
        console->error("Usage: loader [binary file name]");
        return -1;
    }

    console->info("Welcome to the Loader");
    console->info("Start loading binary file: {}", argv[1]);

    auto manager = make_shared<EnclaveManager>(0x0, SAFE_HEAP_LEN * 2);
    
    ELFLoader loader(manager);
    loader.open(argv[1]);
    loader.relocate();
    
    enclave_base = manager->getBase();
    enclave_end = enclave_base + manager->getLen();
    
    auto thread = loader.load();
    vaddr heap = manager->makeHeap(SAFE_HEAP_LEN);
    
    threadpool = std::make_shared<EnclaveThreadPool>(&swapperManager);
    threadpool->addMainThread(thread);

    for (int i = 0; i < 1; i++) {
        threadpool->addWorkerThread(loader.makeWorkerThread());
    }
    
    manager->prepareLaunch();

    char const *testArgv[] ={"hello", "world", (char *)0};
    thread->setArgs(2, (char **)testArgv);
    thread->setAux(get_curr_auxv());
    thread->setEnvs(envp);
    thread->setSwapper(swapperManager);
    thread->setHeap(heap, SAFE_HEAP_LEN);

    void *unsafeHeap = makeUnsafeHeap(UNSAFE_HEAP_LEN);
    thread->setUnsafeHeap(unsafeHeap, UNSAFE_HEAP_LEN);
    
    console->info("tcs: 0x{:x}", thread->getTcs());
    
    swapperManager.launchWorkers();
    /* Set the sigsegv handler to dump the ssa */
    dump_sigaction();

    /* launch the enclave threads */
    threadpool->launch();
    swapperManager.waitWorkers();
    console->info("Program end.");
    return 0;
}
