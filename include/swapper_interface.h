#ifndef SWAPPER_INTERFACE_H
#define SWAPPER_INTERFACE_H

#include <vector>
#include <thread>
#include <queue.h>
#include <request.h>

class SwapperManager {
private:
    vector<thread> threads;
    Queue<RequestBase *> queue;    
    
    int nThreads = 4;
public:
    void runWorker(); 

    void launchWorkers();

    void setNumThreads(int n) {
        this->nThreads = n;
    }
};

#endif
