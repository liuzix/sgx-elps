#pragma once
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <unordered_map>
#include "enclave_thread.h"

using namespace std;

extern uint64_t __jiffies;
class EnclaveThreadPool {
private:
    atomic_int numTotalThread { 0 };
    condition_variable cv;
    bool pendingWakeUp = false;
    mutex m;
    SwapperManager *swapper;
    atomic_int numActiveThread { 0 };
    vector<shared_ptr<EnclaveThread>> threads;
    shared_ptr<EnclaveMainThread> mainThread;
    void addThreadCommon(shared_ptr<EnclaveThread> thread);
public:
    unordered_map <uint64_t, shared_ptr<EnclaveThread>> thread_map;
    map<uint64_t, atomic<char>> sig_flag_map;
    EnclaveThreadPool(SwapperManager *_swapper);
    void idleBlock();
    void newThreadNotify();
    void schedReady();
    void addMainThread(shared_ptr<EnclaveMainThread> thread);
    void addWorkerThread(shared_ptr<EnclaveThread> thread);
    void timer();
    void launch();

};
