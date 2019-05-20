#ifndef SCHED_H
#define SCHED_H
#include "thread_local.h"
#include "request.h"
#include <boost/intrusive/list.hpp>
#include <boost/intrusive/set.hpp>
#include <chrono>
#include <functional>
#include <spin_lock.h>
#include <libOS/tsx.h>

#define MAXIMUM_SLOT 1

using namespace boost::intrusive;
using namespace std::chrono;

class UserThread;

class SchedQueue;

struct SchedEntity {
private:
    atomic_int refCount;
public:
    int32_t ticket;
    UserThread *thread;
    SchedQueue *queue;
    SchedQueue *oldQueue;
    TransLock seLock;
    list_member_hook<> member_hook_;
    set_member_hook<> set_member_hook_;
    bool onQueue;
    bool running;
    int timeSlot;
    SchedEntity(UserThread *_t) {
        thread = _t;
        timeSlot = 0;
        queue = nullptr;
        refCount = 0;
    }

    void refInc() {
//        refCount ++;
    }

    void refDec() {
//        LIBOS_ASSERT(--refCount);
    }
};

class SchedQueue : public
list<SchedEntity, member_hook<SchedEntity, list_member_hook<>,
                                      &SchedEntity::member_hook_>, link_mode<normal_link>>
{
  public:
    TransLock qLock;
};

typedef list<RequestBase, member_hook<RequestBase, list_member_hook<>,
                                      &RequestBase::watchListHook>, link_mode<normal_link>>
    WatchList;

class Scheduler {
  private:
//    SpinLockNoTimer lock;
    PerCPU<SchedEntity *> idle;
    SchedQueue queue;
    void schedNotify();
  public:
    PerCPU<SchedQueue> eachQueue = PerCPU<SchedQueue>(1);
    PerCPU<WatchList> eachWatchList = PerCPU<WatchList>(1);
    PerCPU<SchedEntity *> current;
    PerCPU<SchedulerRequest *> schedReqCache;
    void schedule();
    void enqueueTask(SchedEntity &se);
    void dequeueTask(SchedEntity &se);
    void loadBalance(int cpu, int cpu_num);
    void loadBalance_sender(int cpu);
    void setIdle(function<int()>);
    size_t queueSize() { return queue.size(); };
    PerCPU<SchedEntity *>* getCurrent() { return &this->current; }
};

extern Scheduler *scheduler;
void scheduler_init();
void watchListCheck();
int get_cpu();

#endif
