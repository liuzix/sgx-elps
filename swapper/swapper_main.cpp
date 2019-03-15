#include <thread>
#include <iostream>
#include <functional>
#include <logging.h>
#include <swapper_interface.h>

using namespace std;
#pragma GCC visibility push(hidden)
std::shared_ptr<spdlog::logger> console = spdlog::stdout_color_mt("swapper");
#pragma GCC visibility pop
SwapperManager swapperManager;

void SwapperManager::runWorker(int id) {
    console->info("Swapper thread started, id = {}", id); 

    while (true) {
        RequestBase *request = 0x0;
        if (!this->queue.take(request)) continue;
        
        console->info("Request ptr 0x{:x}", (uint64_t)request);
        console->info("Request tag {}", request->requestType);
        console->flush();
        if (DebugRequest::isInstanceOf(request)) {
            std::cout << this->panic.panicBuf << std::endl;
			request->setDone();
            std::cout.flush(); 
        } else {
            console->critical("Unknown request!");
        }
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
