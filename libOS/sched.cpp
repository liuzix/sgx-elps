#include "sched.h"
#include <cstring>
SchedEntity::SchedEntity(EnclaveThread *eth) {
    memcpy(&ethread, eth, sizeof(EnclaveThread));
    timeSlot = 0;
}
void enqueueTask(EnclaveThread *eth) {
    SchedEntity se(eth);
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
