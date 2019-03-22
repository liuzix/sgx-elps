#ifndef SCHED_H
#define SCHED_H
#include <boost/intrusive/list.hpp>
#include "thread_local.h"
#include <spin_lock.h>

#define MAXIMUM_SLOT 20

using namespace boost::intrusive;

struct SchedEntity {
    list_member_hook<> member_hook_;
    bool onQueue;

    SchedEntity();
    int timeSlot;
    void switchTo();
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
    void enqueueTask(SchedEntity se);
    void dequeueTask(SchedEntity se);
};






#endif
