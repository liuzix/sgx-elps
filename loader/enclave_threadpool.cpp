#include "enclave_threadpool.h"
#include <functional>
#include <thread>
#include <logging.h>

using namespace std;

DEFINE_LOGGER(EnclaveThreadPool, spdlog::level::trace);

EnclaveThreadPool::EnclaveThreadPool(SwapperManager *_swapper) : swapper(_swapper) {
    swapper->setWakeUp(std::bind(&EnclaveThreadPool::newThreadNotify, this));
    swapper->setSchedReady(std::bind(&EnclaveThreadPool::schedReady, this));
}

void EnclaveThreadPool::idleBlock() {
    unique_lock<std::mutex> lk(m);
    while (numActiveThread > numTotalThread) {
        numActiveThread --;
        cv.wait(lk);
        numActiveThread ++;
    }
}

void EnclaveThreadPool::newThreadNotify() {
    unique_lock<std::mutex> lk(m);
    if (numActiveThread < numTotalThread)
        cv.notify_all();
}

void EnclaveThreadPool::schedReady() {
    for (auto thread: threads) {
        if (thread == mainThread) continue;
        std::thread thr([=]{
            thread->run();
        });
        thr.detach();
    }
}

void EnclaveThreadPool::addMainThread(shared_ptr<EnclaveMainThread> thread) {
    thread->threadPool = this;
    thread->setSwapper(*this->swapper);
    threads.push_back(thread);
    thread_map[thread->getTcs()] = thread;
    mainThread = thread;
}

void EnclaveThreadPool::addWorkerThread(shared_ptr<EnclaveThread> thread) {
    thread->threadPool = this;
    thread->setSwapper(*this->swapper);
    threads.push_back(thread);
    thread_map[thread->getTcs()] = thread;
}

void EnclaveThreadPool::launch() {
    if (!mainThread) {
        console->error("mainThread is unset!");
        exit(-1);
    }

    std::thread thread(std::bind(&EnclaveMainThread::run, mainThread));
    thread.detach();
}
