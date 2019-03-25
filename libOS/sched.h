#ifndef SCHED_H
#define SCHED_H
#include <boost/intrusive/list.hpp>
#include "thread_local.h"
#include <functional>
#include <spin_lock.h>

#define MAXIMUM_SLOT 1

using namespace boost::intrusive;

class UserThread;

struct SchedEntity {
    UserThread *thread;
    list_member_hook<> member_hook_;
    bool onQueue;
    int timeSlot; 
    SchedEntity(UserThread *_t) {
        thread = _t;
        timeSlot = 0;
    }

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

extern Scheduler *scheduler;
void scheduler_init();



#endif
