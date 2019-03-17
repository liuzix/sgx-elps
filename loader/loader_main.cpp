#include "enclave_manager.h"

#include <iostream>
#include <memory>
#include <string>

#include <sys/mman.h>
#include <elfio/elfio.hpp>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include "swapper_interface.h"
#include "load_elf.h"
#include "signature.h"
#include "logging.h"

#define UNSAFE_HEAP_LEN 0x10000000

using namespace std;

void *makeUnsafeHeap(size_t length) {
    void *addr = mmap(NULL, length, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0); 
    if (addr == MAP_FAILED) {
        console->error("Cannot map unsafe heap, len = 0x{:x}", length);
        exit(-1);
    }
    return addr;
}

int main(int argc, char **argv) {
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

    void *unsafeHeap = makeUnsafeHeap(UNSAFE_HEAP_LEN);
    thread->setUnsafeHeap(unsafeHeap, UNSAFE_HEAP_LEN);
    swapperManager.launchWorkers();
    thread->run();
    swapperManager.waitWorkers();
    return 0;
}
