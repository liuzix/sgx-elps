#include "sched.h"
#include "user_thread.h"
#include <spin_lock.h>

std::atomic<steady_clock::duration> *timeStamp; 
Scheduler *scheduler;

void scheduler_init() {
    scheduler = new Scheduler;
}

void Scheduler::enqueueTask(SchedEntity &se) {
    lock.lock();
    queue.push_back(se);
    se.onQueue = true;
    lock.unlock();
}

void Scheduler::dequeueTask(SchedEntity &se) {
    lock.lock();
    if (se.onQueue)
        queue.erase(queue.iterator_to(se));
    se.onQueue = false;
    lock.unlock();
}

void Scheduler::schedule() {
    if (*current && ++(*current)->timeSlot != MAXIMUM_SLOT) {
        return;
    }

    lock.lock();
    if (*current) (*current)->timeSlot = 0;

    if (*current && (*current)->onQueue)
        queue.push_back(**current);

    SchedEntity *prev = *current;
    if (!queue.empty()) {
        *current = &queue.front(); 
        queue.pop_front();
        lock.unlock();
        /* decide if we do need a context switch */
        if (*current != prev)
            (*current)->thread->jumpTo(prev ? prev->thread : (*idle)->thread);
    } else {
        lock.unlock();
        /* if it has already been idling */
        //if (!*current) return;
        *current = nullptr;
        (*idle)->thread->jumpTo(prev ? prev->thread : (*idle)->thread);
    }
}

void Scheduler::setIdle(SchedEntity &se) {
     scheduler->dequeueTask(se);
     *idle = &se;
}


