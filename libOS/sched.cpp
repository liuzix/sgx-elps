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
        SchedEntity *tse = &schedQueue.front();
        tse->timeSlot = 0;
        schedQueue.pop_front();
        schedQueue.push_back(*tse);
    }
    schedQueue.front().switchTo();
}
