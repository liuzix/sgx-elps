#pragma once
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <unordered_map>
#include "enclave_thread.h"

using namespace std;

class EnclaveThreadPool {
private:
    atomic_int numTotalThread { 0 };
    condition_variable cv;
    mutex m;
    SwapperManager *swapper;
    int numActiveThread = 0;
    vector<shared_ptr<EnclaveThread>> threads;
    shared_ptr<EnclaveMainThread> mainThread;
public:
    unordered_map <uint64_t, shared_ptr<EnclaveThread>> thread_map;
    EnclaveThreadPool(SwapperManager *_swapper);
    void idleBlock();
    void newThreadNotify();
    void schedReady();
    void addMainThread(shared_ptr<EnclaveMainThread> thread);
    void addWorkerThread(shared_ptr<EnclaveThread> thread);
    void launch();

};
