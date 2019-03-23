#include "sched.h"
#include <spin_lock.h>

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
    
    if (!queue.empty()) {
        *current = &queue.front(); 
        queue.pop_front();
        lock.unlock();
        (*current)->switchTo();
    } else {
        lock.unlock();
        *current = nullptr;
        (*idle)->switchTo(); 
    }
}

void Scheduler::setIdle(SchedEntity &se) {
     *idle = &se; 
}
