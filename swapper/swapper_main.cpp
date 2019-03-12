#include <thread>
#include <functional>
#include <logging.h>
#include <swapper_interface.h>

using namespace std;

std::shared_ptr<spdlog::logger> console = spdlog::stdout_color_mt("swapper");

SwapperManager swapperManager;

void SwapperManager::runWorker(int id) {
    console->info("Swapper thread started, id = {}", id); 
    sleep(10);
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
