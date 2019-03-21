#include <thread>
#include <iostream>
#include <functional>
#include <logging.h>
#include <swapper_interface.h>
#include "debug_request.h"
#include "syscall_request.h"


using namespace std;
#pragma GCC visibility push(hidden)
std::shared_ptr<spdlog::logger> console = spdlog::stdout_color_mt("swapper");
#pragma GCC visibility pop
SwapperManager swapperManager;

void SwapperManager::runWorker(int id) {
    console->info("Swapper thread started, id = {}", id); 

    RequestDispatcher dispatcher;
    dispatcher.addHandler<DebugRequest>(debugRequestHandler);
    dispatcher.addHandler<SyscallRequest>(syscallRequestHandler);
        
    while (true) {
        RequestBase *request = 0x0;
        if (!this->queue.take(request)) continue;
        dispatcher.dispatch(request);
    }
    console->info("Swapper thread exits, id = {}", id);
}

void SwapperManager::launchWorkers() {
    for (int i = 0; i < this->nThreads; i++) {
        auto job = bind(&SwapperManager::runWorker, this, i); 
        this->threads.emplace_back(job);
    }
}

void SwapperManager::waitWorkers() {
    for (int i = 0; i < this->nThreads; i++)
        this->threads[i].join();
}
