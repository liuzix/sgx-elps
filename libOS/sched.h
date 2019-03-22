#ifndef SCHED_H
#define SCHED_H
#include <boost/intrusive/list.hpp>
#include "thread_local.h"
#include <functional>
#include <spin_lock.h>

#define MAXIMUM_SLOT 20

using namespace boost::intrusive;

struct SchedEntity {
    list_member_hook<> member_hook_;
    bool onQueue;
    std::function<void()> switcher;
    int timeSlot; 
    SchedEntity(std::function<void()> _switcher) {
        switcher = _switcher;
        timeSlot = 0;
    }

    void switchTo() { switcher(); }
};

typedef list< SchedEntity
            , member_hook< SchedEntity, list_member_hook<>, &SchedEntity::member_hook_>
            > SchedQueue;

class Scheduler {
private:
    SpinLock lock;
    PerCPU<SchedEntity *> current;
    PerCPU<SchedEntity *> idle;
    SchedQueue queue;
public:
    void schedule();
    void enqueueTask(SchedEntity &se);
    void dequeueTask(SchedEntity &se);
    void setIdle(SchedEntity &se);
};

void scheduler_init();
void scheduler_set_idle(SchedEntity &se);
void schedule();



#endif
