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

void Scheduler::schedNotify() {

    getSharedTLS()->numTotalThread->fetch_add(1);
    
    if (getSharedTLS()->numTotalThread <= getSharedTLS()->numActiveThread)
        return;
    
    libos_print("[sched] sending schedNotify");
    SchedulerRequest *req = Singleton<SchedulerRequest>::getRequest(SchedulerRequest::SchedulerRequestType::NewThread);
    requestQueue->push(req); 
    req->blockOnDone(); 
}

void Scheduler::enqueueTask(SchedEntity &se) {
    lock.lock();
    if(!se.running && !se.onQueue)
        queue.push_back(se);
    if (!se.onQueue)
        schedNotify();
    se.onQueue = true;
    lock.unlock();
}

void Scheduler::dequeueTask(SchedEntity &se) {
    lock.lock();
    if (se.onQueue && !se.running) {
        queue.erase(queue.iterator_to(se));
    }

    if (se.onQueue)
        getSharedTLS()->numTotalThread->fetch_sub(1);
    se.onQueue = false;
    lock.unlock();
}

void Scheduler::schedule() {
    disableInterrupt();
    watchListCheck();

    if (*current && ++(*current)->timeSlot != MAXIMUM_SLOT) {
        return;
    }
    lock.lock();
    if (*current) (*current)->timeSlot = 0;

    if (*current && (*current)->onQueue) {
        queue.push_back(**current);
    }

    SchedEntity *prev = *current;
    if (prev)
        prev->running = false;
    if (!queue.empty()) {
        *current = &queue.front();
        current.get()->running = true;
        queue.pop_front();
        doubleLockThread(current.get()->thread, prev ? prev->thread : nullptr);
        lock.unlock();
        /* decide if we do need a context switch */
        if (*current != prev)
            (*current)->thread->jumpTo(prev ? prev->thread : nullptr);
        else
            enableInterrupt();
    } else {
        doubleLockThread(current.get()->thread, prev ? prev->thread : nullptr);
        lock.unlock();
        /* if it has already been idling */
        //if (!*current) return;
        *current = idle.get();
        if (*current != prev)
            (*idle)->thread->jumpTo(prev ? prev->thread : nullptr);
        else
            enableInterrupt();
    }

}

void Scheduler::setIdle(SchedEntity &se) {
     scheduler->dequeueTask(se);
     *idle = &se;
}

