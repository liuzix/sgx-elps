#ifndef SWAPPER_INTERFACE_H
#define SWAPPER_INTERFACE_H

#include <vector>
#include <thread>
#include <functional>
#include <chrono>
#include <queue.h>
#include <atomic>
#include <request.h>
#include <control_struct.h>

using namespace std::chrono;

class SwapperManager {
private:
    vector<thread> threads;
    Queue<RequestBase*> queue;
    panic_struct panic; 
    std::atomic<steady_clock::duration> timeStamp;
    function<void()> wakeUpThread;
    function<void()> schedReady;
    int nThreads = 3;

    void runWorker(int id); 
public:
    void launchWorkers();
    void waitWorkers();

    void setNumThreads(int n) {
        this->nThreads = n;
    }

    void setWakeUp(function<void()> func) {
        this->wakeUpThread = func;
    }

    void setSchedReady(function<void()> func) {
        this->schedReady = func;
    }

    friend class EnclaveThread;
    friend void schedulerRequestHandler(SwapperManager *, SchedulerRequest *);
};

extern SwapperManager swapperManager;
#endif
