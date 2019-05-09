#include "sched.h"
#include "singleton.h"
#include "libos.h"
#include "panic.h"
#include "user_thread.h"
#include <spin_lock.h>

Scheduler *scheduler;


void scheduler_init() {
    scheduler = new Scheduler;
}

int get_cpu() {
    bool interruptFlag = disableInterrupt();
    uint64_t cpuID = getSharedTLS()->threadID;
    if (!interruptFlag)
        enableInterrupt();
    return (int)cpuID;
}

void Scheduler::schedNotify() {

    getSharedTLS()->numTotalThread->fetch_add(1);

    //libos_print("numActiveThread: %d, numKernelThreads: %d",
    //        getSharedTLS()->numActiveThread->load(),
    //        getSharedTLS()->numKernelThreads);
    /*
    if (getSharedTLS()->numKernelThreads == getSharedTLS()->numActiveThread->load())
        return;

    libos_print("[sched] sending schedNotify");
    SchedulerRequest *req = Singleton<SchedulerRequest>::getRequest(SchedulerRequest::SchedulerRequestType::NewThread);
    requestQueue->push(req); 
    req->blockOnDone(); 
    */
}

void Scheduler::enqueueTask(SchedEntity &se) {
    int cpu = get_cpu();
    eachQueue[cpu].qLock.lock();
    se.seLock.lock();
    if(!se.running && !se.onQueue) {
        eachQueue[cpu].push_back(se);
    }
    if (!se.onQueue)
        schedNotify();
    se.queue = &eachQueue[cpu];
    se.onQueue = true;
    se.seLock.unlock();
    eachQueue[cpu].qLock.unlock();
}

void Scheduler::dequeueTask(SchedEntity &se) {
    auto qLock = &(se.queue->qLock);
    qLock->lock();
    se.seLock.lock();
    if (se.onQueue && !se.running) {
        (se.queue)->erase((se.queue)->iterator_to(se));
    }

    if (se.onQueue)
        getSharedTLS()->numTotalThread->fetch_sub(1);
    se.queue = nullptr;
    se.onQueue = false;
    se.seLock.unlock();
    qLock->unlock();
}

void Scheduler::schedule() {
    disableInterrupt();
    watchListCheck();

//    int len;
//    if ((len = (*eachQueue).size()) != 0)
//    for (int i = 0; i < 2; i++)
//        libos_print("=====[%d]=======LEN:[%d]===", i, eachQueue[i].size());
    if (*current && ++(*current)->timeSlot != MAXIMUM_SLOT) {
        return;
    }
    if (*current) (*current)->timeSlot = 0;
    SchedQueue *queue = nullptr;
    bool prev_lock = 0;

    if (*current) {
        (*current)->seLock.lock();
        prev_lock = 1;
        //libos_print("===[%x]===", queue);
        if ((*current)->onQueue) {
            LIBOS_ASSERT((*current)->queue);
            queue = (*current)->queue;
            queue->qLock.lock();
            queue->push_back(**current);
            queue->qLock.unlock();
        }
    }

    SchedEntity *prev = *current;
    if (prev) {
        prev->running = false;
    }
    if (prev_lock) {
        (*current)->seLock.unlock();
    }

    (*eachQueue).qLock.lock();

    if (!(*eachQueue).empty()) {
        *current = &(*eachQueue).front();
        current.get()->seLock.lock();
        current.get()->running = true;
        (*eachQueue).pop_front();
        current.get()->seLock.unlock();
        //doubleLockThread(current.get()->thread, prev ? prev->thread : nullptr);
        (*eachQueue).qLock.unlock();

        /* decide if we do need a context switch */
        if (*current != prev)
            (*current)->thread->jumpTo(prev ? prev->thread : nullptr);
        else
            enableInterrupt();
    } else {
        //doubleLockThread(*current ? current.get()->thread : nullptr,
        //        prev ? prev->thread : nullptr);
        (*eachQueue).qLock.unlock();

        /* decide if we do need a context switch */
        /* if it has already been idling */
        //if (!*current) return;
        //libos_print("sched: running idle");
        *current = idle.get();
        if (*current != prev)
            (*idle)->thread->jumpTo(prev ? prev->thread : nullptr);
        else
            enableInterrupt();
    }

}

void Scheduler::setIdle(function<int()> fn) {
    if (*idle) return;
    auto idleThr = new UserThread(fn);
    idleThr->se.onQueue = false;
    *idle = &idleThr->se;
}

