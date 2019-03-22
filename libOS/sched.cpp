#include "sched.h"
#include <cstring>

SchedQueue schedQueue;

SchedEntity::SchedEntity() {
    timeSlot = 0;
}
void SchedEntity::switchTo() {
    return;
}

void enqueueTask() {
    SchedEntity se;
    schedQueue.push_back(se);
}

void dequeueTask() {
    schedQueue.push_back(schedQueue.front());
    schedQueue.pop_front();
}

void schedule() {
    if (++schedQueue.front().timeSlot == MAXIMUM_SLOT) {
        dequeueTask();
    }
}
