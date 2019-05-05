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
    int nThreads = 2;

    void runWorker(int id);
public:
    void launchWorkers();
    void waitWorkers();

    function<void()> wakeUpThread;
    function<void()> schedReady;
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
};

#define DBBUF_SIZE 40960
extern SwapperManager swapperManager;
extern char debuggerThreadBuf[DBBUF_SIZE];
extern int buf_index;
#endif
