#ifndef SWAPPER_INTERFACE_H
#define SWAPPER_INTERFACE_H

#include <vector>
#include <thread>
#include <queue.h>
#include <request.h>
#include <control_struct.h>

class SwapperManager {
private:
    vector<thread> threads;
    Queue<RequestBase*> queue;    
    panic_struct panic; 
    int nThreads = 4;
    
    void runWorker(int id); 
public:
    void launchWorkers();
    void waitWorkers();

    void setNumThreads(int n) {
        this->nThreads = n;
    }

    friend class EnclaveThread;
};

extern SwapperManager swapperManager;
#endif
