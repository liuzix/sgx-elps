#ifndef SCHED_H
#define SCHED_H
#include "thread_local.h"
#include "request.h"
#include <boost/intrusive/list.hpp>
#include <boost/intrusive/set.hpp>
#include <chrono>
#include <functional>
#include <spin_lock.h>

#define MAXIMUM_SLOT 1

using namespace boost::intrusive;
using namespace std::chrono;

class UserThread;

class SchedQueue;

struct SchedEntity {
    int32_t ticket;
    UserThread *thread;
    SchedQueue *queue;
    SpinLockNoTimer seLock;
    list_member_hook<> member_hook_;
    set_member_hook<> set_member_hook_;
    bool onQueue;
    bool running;
    int timeSlot;
    SchedEntity(UserThread *_t) {
        thread = _t;
        timeSlot = 0;
        queue = nullptr;
    }
};

class SchedQueue : public
list<SchedEntity, member_hook<SchedEntity, list_member_hook<>,
                                      &SchedEntity::member_hook_>, link_mode<normal_link>>
{
  public:
    SpinLockNoTimer qLock;
};

class Scheduler {
  private:
//    SpinLockNoTimer lock;
    PerCPU<SchedEntity *> idle;
    SchedQueue queue;
    PerCPU<SchedQueue> eachQueue = PerCPU<SchedQueue>(1);
    void schedNotify();
  public:
    PerCPU<SchedEntity *> current;
    PerCPU<SchedulerRequest *> schedReqCache;
    void schedule();
    void enqueueTask(SchedEntity &se);
    void dequeueTask(SchedEntity &se);
    void setIdle(function<int()>);
    size_t queueSize() { return queue.size(); };
    PerCPU<SchedEntity *>* getCurrent() { return &this->current; }
};

extern Scheduler *scheduler;
void scheduler_init();
void watchListCheck();

#endif
