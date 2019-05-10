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
    bool interruptFlag = disableInterrupt();
    int cpu = get_cpu();
    eachQueue[cpu].qLock.lock();
    se.seLock.lock();
    LIBOS_ASSERT(!se.onQueue);
    if(!se.running && !se.onQueue) {
        eachQueue[cpu].push_back(se);
        se.refInc();
    }
    if (!se.onQueue)
        schedNotify();
    se.queue = &eachQueue[cpu];
    se.onQueue = true;
    se.seLock.unlock();
    eachQueue[cpu].qLock.unlock();

    if (!interruptFlag)
        enableInterrupt();
}

void Scheduler::dequeueTask(SchedEntity &se) {
    auto qLock = &(se.queue->qLock);
    qLock->lock();
    se.seLock.lock();
    LIBOS_ASSERT(se.onQueue);
    if (se.onQueue && !se.running) {
        (se.queue)->erase((se.queue)->iterator_to(se));
        se.refDec();
    }

    if (se.onQueue)
        getSharedTLS()->numTotalThread->fetch_sub(1);
    se.queue = nullptr;
    se.onQueue = false;
    se.seLock.unlock();
    qLock->unlock();
}

/* the lock of eachQueue[cpu] is grabbed before this function*/
void Scheduler::loadBalance(int cpu, int cpu_num) {
    if (cpu_num == 1) return;
    for (int i = 0; i < cpu_num; i++) {
        if (i == cpu) continue;
        if (eachQueue[i].size() == 0) continue;
        auto& src_q = eachQueue[i];
        src_q.qLock.lock();

        auto it = src_q.begin();
        while (it != src_q.end()) {
            it->seLock.lock();
            if (!it->running) {
                /* pull task from source queue*/
                src_q.erase(it);
                src_q.qLock.unlock();
                eachQueue[cpu].push_back(*it);
                /* change home queue of pulled task*/
                it->queue = &eachQueue[cpu];
                it->seLock.unlock();
                return;
            } else {
                it->seLock.unlock();
                it++;
            }
        }
        src_q.qLock.unlock();
    }
}

void Scheduler::loadBalance_sender(int cpu) {
    if (eachQueue[cpu].size() < 3) return;

    for (int i = 0; i < 4; i++) {
        if (i == cpu) continue;
        if (eachQueue[i].size() > eachQueue[cpu].size()) continue;
        auto& dst_q = eachQueue[i];
        auto& src_q = eachQueue[cpu];

        auto it = src_q.begin();
        while (it != src_q.end()) {
            it->seLock.lock();
            if (!it->running) {
                /* pull task from source queue*/
                src_q.erase(it);
                dst_q.qLock.lock();
                dst_q.push_back(*it);
                dst_q.qLock.unlock();

                /* change home queue of pulled task*/
                it->queue = &dst_q;
                it->seLock.unlock();
                return;
            } else {
                it->seLock.unlock();
                it++;
            }
        }
        dst_q.qLock.unlock();
    }
}

void Scheduler::schedule() {
    disableInterrupt();
    watchListCheck();

//    libos_print("============LEN:[%d]===", eachQueue[cpu].size());
    if (*current && ++(*current)->timeSlot != MAXIMUM_SLOT) {
        return;
    }
    if (*current) (*current)->timeSlot = 0;
    int cpu = get_cpu();
    int cpu_num = getSharedTLS()->numKernelThreads;
    SchedQueue *queue = nullptr;
    bool prev_lock = 0;

    if (*current) {
        (*current)->seLock.lock();
        prev_lock = 1;
        if ((*current)->onQueue) {
            LIBOS_ASSERT((*current)->queue);
            queue = (*current)->queue;
            queue->qLock.lock();
            queue->push_back(**current);
            current.get()->refInc();
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
start_sched:
        if (*current)
            current.get()->refDec();

        *current = &(*eachQueue).front();

        current.get()->refInc();

        current.get()->seLock.lock();
        current.get()->running = true;
        (*eachQueue).pop_front();
        current.get()->seLock.unlock();
        //loadBalance_sender(cpu);
        //doubleLockThread(current.get()->thread, prev ? prev->thread : nullptr);
        (*eachQueue).qLock.unlock();

        /* decide if we do need a context switch */
        if (*current != prev)
            (*current)->thread->jumpTo(prev ? prev->thread : nullptr);
        else
            enableInterrupt();
    } else {
        /* if queue is empty, try to pull task from other queues*/
        (*eachQueue).qLock.unlock();
        loadBalance(cpu, cpu_num);
        (*eachQueue).qLock.lock();
        if ((*eachQueue).size())
            goto start_sched;
        (*eachQueue).qLock.unlock();

        /* decide if we do need a context switch */
        /* if it has already been idling */
        if (*current)
            current.get()->refDec();

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

