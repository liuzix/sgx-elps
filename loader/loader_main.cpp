#include "enclave_manager.h"

#include <iostream>
#include <memory>
#include <string>

#include <elfio/elfio.hpp>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include "swapper_interface.h"
#include "load_elf.h"
#include "signature.h"
#include "logging.h"
#include <ssa_dump.h>

using namespace std;

std::map<uint64_t, char> sig_flag_map;
char get_flag(uint64_t rbx) {
    char res = sig_flag_map[rbx];
    //console->log("rbx: 0x{:x}, flag: {} ", rbx, res);
    return res;
}

void set_flag(uint64_t rbx, char flag) {
    sig_flag_map[rbx] = flag;
}


void sig_exit() {
    exit(-1);
}

static void __sigaction(int sig, siginfo_t *info, void *ucontext) {
    ucontext_t *context = (ucontext_t *)ucontext;
    uint64_t rbx = context->uc_mcontext.gregs[REG_RBX];

    //Per-thread flag
    set_flag(rbx, 1);
    console->error("Segmentation Fault!");
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

}

int main(int argc, char **argv) {
    /* Set the sigsegv handler to dump the ssa */
    dump_sigaction();
    console->set_level(spdlog::level::trace);
    if (argc < 2) {
        console->error("Usage: loader [binary file name]");
        return -1;
    }

    console->info("Welcome to the Loader");
    console->info("Start loading binary file: {}", argv[1]);

    auto manager = make_shared<EnclaveManager>(0x400000, 0x400000);
    auto thread = load_one(argv[1], manager);
    vaddr heap = manager->makeHeap(0x100000);
    manager->prepareLaunch();

    char const *testArgv[] ={"hello", (char *)0};
    thread->setArgs(1, (char **)testArgv);
    thread->setSwapper(swapperManager);
    thread->setHeap(heap, 0x100000);

    console->info("tcs: 0x{:x}", thread->getTcs());
    swapperManager.launchWorkers();
    thread->run();
    swapperManager.waitWorkers();
    return 0;
}
