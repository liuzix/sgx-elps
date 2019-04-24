/*
 * This is a user-level futex implementeation for our libos,
 * we want to keep the interface the same as the kernel
 */
#include "futex.h"
#include "panic.h"
#ifndef ENOSYS
#define ENOSYS  38
#endif

FutexHash *futexHash;
FutexHash::bucket_type *futex_queue_buckets;

static inline FutexBucket* libos_get_bucket(uint32_t *addr) {
    libos_print("Try to get a bucket.");
    auto fbit = futexHash->find(addr);
    FutexBucket *fb;

    if (fbit == futexHash->end()) {
        libos_print("Create new bucket.");
        fb = new FutexBucket(addr);
        futexHash->insert(*fb);
    }
    else {
        libos_print("Bucket found.");
        fb = &(*fbit);
    }
    return fb;
}

static inline FutexBucket* libos_futex_queue_lock(uint32_t *addr) {
    FutexBucket *fb;

    fb = libos_get_bucket(addr);

    fb_waiters_inc(fb->waiters);

    fb->lock.lock();

    return fb;
}

static inline void libos_futex_queue_unlock(FutexBucket *fb) {
    fb->lock.unlock();
    fb_waiters_dec(fb->waiters);
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
    libos_print("We are sleeping on addr: 0x%lx.", (uint64_t)addr);
    libos_print("[futex_wait[1]] Interrupt state before sleep : %d", (int)*getSharedTLS()->inInterrupt);
    scheduler->schedule();

    /* Wake up */
    disableInterrupt();
    fb->lock.lock();
    //fb->remove(*scheduler->current.get());

    libos_futex_queue_unlock(fb);
    libos_print("I am waken up.");
    enableInterrupt();
    libos_print("[futex_wait[1]] Interrupt state after unlock: %d", (int)*getSharedTLS()->inInterrupt);
    return ret;
}

uint64_t futex_wake(uint32_t *addr, unsigned int flags, uint32_t nr_wake, uint32_t val3) {
    auto fbit = futexHash->find(addr);
    FutexBucket *fb;
    int ret = 0;

    libos_print("Wake up on address: 0x%lx", (uint64_t)addr);
    if (fbit == futexHash->end()) {
        libos_print("No hash bucket found.");
        return ret;
    }
    fb = &*fbit;

    if (!fb->waiters) {   /* This is atmoic read */
        /* We might want to release the bucket in the future. But just leave it here for now. */
        libos_print("No waiters in the bucket.");
        return ret;
    }

    fb->lock.lock();

    auto& q = fb->getQueue();
    auto it = q.begin();

    while (it != q.end()) {
        SchedEntity *t = &*it;

        /* The sleep queue and the run queue share the same hook */
        it = q.erase(it);
        scheduler->enqueueTask(*t);
        libos_print("Enqueued thread: %d, queue len: %d", (*t).thread->id, scheduler->queueSize());
        ret++;
        if (ret > (int)nr_wake)
            break;
    }

    fb->lock.unlock();

    libos_print("[futex_wake[2]] Interrupt state after unlock: %d", (int)*getSharedTLS()->inInterrupt);
    return ret;
}

/*
uint64_t futex_requeue(uint32_t *addr1, unsigned int flags, uint32_t *addr2,
                       int nr_wake, int nr_requeue, uint32_t cmpval, int requeue_pi) {
    FutexBucket *fb1, *fb2;
    uint32_t uval;
    int ret = 0;

    fb1 = libos_futex_queue_lock(addr1);
    fb2 = NULL;
    uval = 0;

    return ret;
}
*/

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
        //return futex_requeue();
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
