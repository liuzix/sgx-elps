#include "sched.h"
#include <cstring>

SchedQueue schedQueue;

SchedEntity::SchedEntity() {
    timeSlot = 0;
}
void SchedEntity::switchTo() {
    return;
}

void enqueueTask(SchedEntity se) {
    schedQueue.push_back(se);
}

void dequeueTask(SchedEntity se) {
    schedQueue.erase(schedQueue.iterator_to(se));
}

void schedule() {
    if (++schedQueue.front().timeSlot == MAXIMUM_SLOT) {
        schedQueue.push_back(schedQueue.front());
        schedQueue.pop_front();
        schedQueue.back().timeSlot = 0;
    }
    schedQueue.front().switchTo();
}
