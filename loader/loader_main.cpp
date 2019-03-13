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

using namespace std;
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

    swapperManager.launchWorkers();
    thread->run();
    swapperManager.waitWorkers();
    return 0;
}
