#include "sched.h"
#include "panic.h"
#include "user_thread.h"
#include <spin_lock.h>

std::atomic<steady_clock::duration> *timeStamp; 
Scheduler *scheduler;

void watchListCheck();

void scheduler_init() {
    scheduler = new Scheduler;
}

void Scheduler::enqueueTask(SchedEntity &se) {
    lock.lock();
    if(!se.running && !se.onQueue)
        queue.push_back(se);
    se.onQueue = true;
    lock.unlock();
}

void Scheduler::dequeueTask(SchedEntity &se) {
    lock.lock();
    if (se.onQueue && !se.running)
        queue.erase(queue.iterator_to(se));
    se.onQueue = false;
    lock.unlock();
}

void Scheduler::schedule() {
    watchListCheck();
    if (*current && ++(*current)->timeSlot != MAXIMUM_SLOT) {
        return;
    }

    lock.lock();
    if (*current) (*current)->timeSlot = 0;

    if (*current && (*current)->onQueue) {
        libos_print("pushing current task on queue");
        queue.push_back(**current);
    } else {
        libos_print("no current or current is not on queue");
    }

    SchedEntity *prev = *current;
    if (prev)
        prev->running = false;
    if (!queue.empty()) {
        *current = &queue.front(); 
        current.get()->running = true;
        queue.pop_front();
        lock.unlock();
        /* decide if we do need a context switch */
        if (*current != prev)
            (*current)->thread->jumpTo(prev ? prev->thread : nullptr);
    } else {
        lock.unlock();
        /* if it has already been idling */
        //if (!*current) return;
        *current = idle.get();
        if (*current != prev)
            (*idle)->thread->jumpTo(prev ? prev->thread : nullptr);
    }
}

void Scheduler::setIdle(SchedEntity &se) {
     scheduler->dequeueTask(se);
     *idle = &se;
}


