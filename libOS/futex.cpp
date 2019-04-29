/*
 * This is a user-level futex implementeation for our libos,
 * we want to keep the interface the same as the kernel
 */
#include "futex.h"
#include "panic.h"
#include <climits>
#ifndef ENOSYS
#define ENOSYS  38
#endif

SpinLockNoTimer futexHashLock;
FutexHash *futexHash;
FutexHash::bucket_type *futex_queue_buckets;

static inline FutexBucket* libos_get_bucket(uint32_t *addr) {
    futexHashLock.lock();
    auto fbit = futexHash->find(addr);
    FutexBucket *fb;

    if (fbit == futexHash->end()) {
        fb = new FutexBucket(addr);
        libos_print("[get_bucket] Create new bucket for addr: 0x%lx.", (uint64_t)addr);
        futexHash->insert(*fb);
        futexHashLock.unlock();
        libos_print("Finished creating bucket");
    }
    else {
        libos_print("Bucket found.");
        fb = &(*fbit);
        futexHashLock.unlock();
    }
    return fb;
}

static inline FutexBucket* libos_futex_queue_lock(uint32_t *addr) {
    FutexBucket *fb;

    fb = libos_get_bucket(addr);

    fb->waiters++;
    fb->lock.lock();

    return fb;
}

static inline void libos_futex_queue_unlock(FutexBucket *fb) {
    fb->lock.unlock();
    fb->waiters--;
}

static inline void libos_futex_enqueue(FutexBucket *fb) {
    fb->enqueue(*scheduler->current.get());
    scheduler->dequeueTask(*scheduler->current.get());
}

uint64_t libos_futex_wait(uint32_t *addr, unsigned int flags, uint32_t val,
                          uint64_t dummy, uint32_t bitset) {
    FutexBucket *fb;
    uint32_t uval;
    int ret = 0;

    fb = libos_futex_queue_lock(addr);
    /* Should we disable interrupt in this macro? */
    get_user_val(uval, addr);

    /*
     * There's no reason to reach here if get_user_val failed.
     * No additional actions are needed to check the result.
     */

    if (uval != val) {
        /* We are fine. No need to sleep. */
        fb->lock.unlock();
        return ret;
    }

    /* Ready to sleep */
    disableInterrupt();
    libos_futex_enqueue(fb);

    fb->lock.unlock();
    enableInterrupt();
    libos_print("[futex_wait] We are sleeping on addr: 0x%lx.", (uint64_t)addr);
    scheduler->schedule();

    /* Wake up */
    disableInterrupt();
    fb->lock.lock();
    /* Dequeued at wake up function */
    libos_futex_queue_unlock(fb);
    libos_print("I am waken up.");
    enableInterrupt();
    return ret;
}

uint64_t futex_wake(uint32_t *addr, unsigned int flags, uint32_t nr_wake, uint32_t val3) {
    futexHashLock.lock();
    auto fbit = futexHash->find(addr);
    FutexBucket *fb;
    int ret = 0;
    
    /* 1 or ALL */
    if (nr_wake != 1)
        nr_wake = INT_MAX;
    libos_print("[futex_wake] Wake up on address: 0x%lx", (uint64_t)addr);
    if (fbit == futexHash->end()) {
        futexHashLock.unlock();
        libos_print("No hash bucket found.");
        return ret;
    }
    fb = &*fbit;
    futexHashLock.unlock();

    if (!fb->waiters) {   /* This is atmoic read */
        libos_print("No waiters in the bucket.");
        return ret;
    }

    fb->lock.lock();

    auto& q = fb->getQueue();
    auto it = q.begin();

    while (it != q.end() && ret < (int)nr_wake) {
        SchedEntity *t = &*it;

        /*
         * The wait queue and the run queue share the same hook,
         * so we have to remove it from the wait queue.
         */
        it = q.erase(it);
        scheduler->enqueueTask(*t);
        libos_print("Enqueued thread: %d, queue len: %d", (*t).thread->id, scheduler->queueSize());
        ret++;
    }

    fb->lock.unlock();

    /* We might want to relase the empty bucket here */
    return ret;
}


uint64_t futex_requeue(uint32_t *addr1, unsigned int flags, uint32_t *addr2,
                       int nr_wake, int nr_requeue, uint32_t *cmpval, int requeue_pi) {
    FutexBucket *fb1, *fb2;
    int ret = 0;

    futexHashLock.lock();
    auto fbit = futexHash->find(addr1);

    libos_print("[futex_requeue] Wake up on address: 0x%lx", (uint64_t)addr1);
    libos_print("[futex_requeue] Requeue to address: 0x%lx", (uint64_t)addr2);
    if (fbit == futexHash->end()) {
        futexHashLock.unlock();
        libos_print("[futex_requeue] No hash bucket found.");
        return ret;
    }
    fb1 = &*fbit;
    futexHashLock.unlock();

    fb2 = libos_get_bucket(addr2);

    /* Double lock */
    if (addr1 > addr2) {
        fb1->lock.lock();
        fb2->lock.lock();
    } else {
        fb2->lock.lock();
        fb1->lock.lock();
    }

    if (!fb1->waiters) {   /* This is atmoic read */
        /* We cannot relase the bucket since the waiters still hold the reference */
        libos_print("[futex_requeue] No waiters in the bucket.");
        if (addr1 > addr2) {
           fb2->lock.unlock();
           fb1->lock.unlock();
        } else {
           fb1->lock.unlock();
           fb2->lock.unlock();
        }
        return ret;
    }

    auto& q1 = fb1->getQueue();
    auto it = q1.begin();
    int req_count = 0;

    /* Wake up */
    while (it != q1.end() && ret < (int)nr_wake) {
        SchedEntity *t = &*it;

        it = q1.erase(it);
        fb1->waiters--;
        scheduler->enqueueTask(*t);
        libos_print("[futex_requeue] Enqueued thread: %d, queue len: %d", (*t).thread->id, scheduler->queueSize());
        ret++;
    }

    it = q1.begin();
    /* Requeue */
    while (it != q1.end() && req_count < (int)nr_requeue) {
        SchedEntity *t = &*it;

        it = q1.erase(it);
        fb2->enqueue(*t);
        fb2->waiters++;
        fb1->waiters--;
        req_count++;
    }

    /* Double unlock */
    if (addr1 > addr2) {
        fb2->lock.unlock();
        fb1->lock.unlock();
    } else {
        fb1->lock.unlock();
        fb2->lock.unlock();
    }

    return ret;
}


uint64_t libos_do_futex(uint32_t *addr, int op, uint64_t dummy, uint32_t val,
                        uint32_t *addr2, uint32_t val2, uint32_t val3) {
    int cmd = op & FUTEX_CMD_MASK;
    unsigned int flags = 0;
    uint64_t timeout = 0;

    switch(cmd) {
    case FUTEX_WAIT:
        return libos_futex_wait(addr, flags, val, timeout, val3);
    case FUTEX_WAKE:
        return futex_wake(addr, flags, val, val3);
    case FUTEX_REQUEUE:
        return futex_requeue(addr, flags, addr2, val, val2, NULL, 0);
    default:
        break;
    }

    return -ENOSYS;
}

extern "C" uint64_t libos_futex(uint32_t *addr, int op, uint32_t val,
                                uint64_t utime, uint32_t *addr2, uint32_t val3) {
    uint32_t val2 = 0;
    int cmd = op & FUTEX_CMD_MASK;

    if (cmd == FUTEX_REQUEUE || cmd == FUTEX_CMP_REQUEUE ||
        cmd == FUTEX_CMP_REQUEUE_PI || cmd == FUTEX_WAKE_OP)
        val2 = (uint32_t) (unsigned long) utime;

    return libos_do_futex(addr, op, utime, val, addr2, val2, val3);
}
