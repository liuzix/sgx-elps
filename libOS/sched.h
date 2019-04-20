#ifndef SCHED_H
#define SCHED_H
#include "thread_local.h"
#include <boost/intrusive/list.hpp>
#include <boost/intrusive/set.hpp>
#include <chrono>
#include <functional>
#include <spin_lock.h>

#define MAXIMUM_SLOT 1

using namespace boost::intrusive;
using namespace std::chrono;

extern std::atomic<steady_clock::duration> *timeStamp;

class UserThread;

struct SchedEntity {
    int32_t ticket; 
    UserThread *thread;
    list_member_hook<> member_hook_;
    set_member_hook<> set_member_hook_;
    bool onQueue;
    bool running;
    int timeSlot;
    SchedEntity(UserThread *_t) {
        thread = _t;
        timeSlot = 0;
    }
};

typedef list<SchedEntity, member_hook<SchedEntity, list_member_hook<>,
                                      &SchedEntity::member_hook_>>
    SchedQueue;

class Scheduler {
  private:
    SpinLockNoTimer lock;
    PerCPU<SchedEntity *> idle;
    SchedQueue queue;
  public:
    PerCPU<SchedEntity *> current;
    void schedule();
    void enqueueTask(SchedEntity &se);
    void dequeueTask(SchedEntity &se);
    void setIdle(SchedEntity &se);
};

extern Scheduler *scheduler;
void scheduler_init();

#endif
