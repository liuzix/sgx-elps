#include "enclave_threadpool.h"
#include <iostream>
#include <functional>
#include <logging.h>
#include <thread>

using namespace std;

DEFINE_LOGGER(EnclaveThreadPool, spdlog::level::trace);

EnclaveThreadPool::EnclaveThreadPool(SwapperManager *_swapper)
    : swapper(_swapper) {
    swapper->setWakeUp(std::bind(&EnclaveThreadPool::newThreadNotify, this));
    swapper->setSchedReady(std::bind(&EnclaveThreadPool::schedReady, this));
}

void EnclaveThreadPool::idleBlock() {
    unique_lock<std::mutex> lk(m);
    if (pendingWakeUp) {
        pendingWakeUp = false;
        return;
    }
    while (numActiveThread > numTotalThread) {
        numActiveThread--;
        console->info("start cv wait");
        cv.wait(lk);
        console->info("end cv wait");
        numActiveThread++;
        if (numActiveThread == 1)
            break;
    }
    pendingWakeUp = false;
}

void EnclaveThreadPool::newThreadNotify() {
    unique_lock<std::mutex> lk(m);
    //console->info("numActiveThread = {}, numTotalThread = {}", numActiveThread,
    //              numTotalThread);
    
    pendingWakeUp = true;
    if (numActiveThread < (int)threads.size()) {
        console->info("cv broadcast");
        cv.notify_one();
    }
}

void EnclaveThreadPool::schedReady() {
    for (auto thread : threads) {
        if (thread == mainThread)
            continue;
        std::thread thr([=] { thread->run(); });
        numActiveThread++;
        thr.detach();
    }
}

void EnclaveThreadPool::addThreadCommon(shared_ptr<EnclaveThread> thread) {
    thread->threadPool = this;
    thread->getSharedTLS()->numTotalThread = &numTotalThread;
    thread->getSharedTLS()->numActiveThread = &numActiveThread;
    thread->setSwapper(*this->swapper);
    threads.push_back(thread);
    thread_map[thread->getTcs()] = thread;
}

void EnclaveThreadPool::addMainThread(shared_ptr<EnclaveMainThread> thread) {
    addThreadCommon(thread);
    mainThread = thread;
}

void EnclaveThreadPool::addWorkerThread(shared_ptr<EnclaveThread> thread) {
    addThreadCommon(thread);
}

void EnclaveThreadPool::launch() {
    for (auto &t: thread_map)
        t.second->getSharedTLS()->numKernelThreads = thread_map.size(); 

    if (!mainThread) {
        console->error("mainThread is unset!");
        exit(-1);
    }

    std::thread thread(std::bind(&EnclaveMainThread::run, mainThread));
    numActiveThread++;
    thread.detach();
}
