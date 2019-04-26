#ifndef _LIBOS_FUTEX_H_
#define _LIBOS_FUTEX_H_
#include "user_thread.h"
#include "sched.h"
#include <spin_lock.h>
#include "panic.h"
#include <boost/intrusive/unordered_set.hpp>
#include <boost/functional/hash.hpp>
#include <boost/intrusive/list.hpp>

#ifndef MAX_FUTEX_BUCKETS
#define MAX_FUTEX_BUCKETS 100
#endif

/* Following macros are from Linux kernel */
#define FUTEX_WAIT		0
#define FUTEX_WAKE		1
#define FUTEX_FD		2
#define FUTEX_REQUEUE		3
#define FUTEX_CMP_REQUEUE	4
#define FUTEX_WAKE_OP		5
#define FUTEX_LOCK_PI		6
#define FUTEX_UNLOCK_PI		7
#define FUTEX_TRYLOCK_PI	8
#define FUTEX_WAIT_BITSET	9
#define FUTEX_WAKE_BITSET	10
#define FUTEX_WAIT_REQUEUE_PI	11
#define FUTEX_CMP_REQUEUE_PI	12

#define FUTEX_PRIVATE_FLAG	128
#define FUTEX_CLOCK_REALTIME	256
#define FUTEX_CMD_MASK		~(FUTEX_PRIVATE_FLAG | FUTEX_CLOCK_REALTIME)

#define FUTEX_WAIT_PRIVATE	(FUTEX_WAIT | FUTEX_PRIVATE_FLAG)
#define FUTEX_WAKE_PRIVATE	(FUTEX_WAKE | FUTEX_PRIVATE_FLAG)
#define FUTEX_REQUEUE_PRIVATE	(FUTEX_REQUEUE | FUTEX_PRIVATE_FLAG)
#define FUTEX_CMP_REQUEUE_PRIVATE (FUTEX_CMP_REQUEUE | FUTEX_PRIVATE_FLAG)
#define FUTEX_WAKE_OP_PRIVATE	(FUTEX_WAKE_OP | FUTEX_PRIVATE_FLAG)
#define FUTEX_LOCK_PI_PRIVATE	(FUTEX_LOCK_PI | FUTEX_PRIVATE_FLAG)
#define FUTEX_UNLOCK_PI_PRIVATE	(FUTEX_UNLOCK_PI | FUTEX_PRIVATE_FLAG)
#define FUTEX_TRYLOCK_PI_PRIVATE (FUTEX_TRYLOCK_PI | FUTEX_PRIVATE_FLAG)
#define FUTEX_WAIT_BITSET_PRIVATE	(FUTEX_WAIT_BITSET | FUTEX_PRIVATE_FLAG)
#define FUTEX_WAKE_BITSET_PRIVATE	(FUTEX_WAKE_BITSET | FUTEX_PRIVATE_FLAG)
#define FUTEX_WAIT_REQUEUE_PI_PRIVATE	(FUTEX_WAIT_REQUEUE_PI | \
					 FUTEX_PRIVATE_FLAG)
#define FUTEX_CMP_REQUEUE_PI_PRIVATE	(FUTEX_CMP_REQUEUE_PI | \
					 FUTEX_PRIVATE_FLAG)

#define fb_waiters_inc(waiters) \
    {                           \
        waiters++;              \
    }

#define fb_waiters_dec(waiters) \
    {                           \
        waiters--;              \
    }

#define fb_waiter_read(waiters) \
    {                           \
        waiters;                \
    }

#define get_user_val(uval, addr) \
    {                           \
        disableInterrupt();     \
        uval = *addr;           \
        enableInterrupt();      \
    }

using namespace boost;

typedef intrusive::list<SchedEntity, member_hook<SchedEntity, list_member_hook<>,
                                     &SchedEntity::member_hook_> > FutexQueue;

/* This is a bad name */
class FutexBucket : public unordered_set_base_hook<>{
private:
    FutexQueue q;
public:
    SpinLockNoTimer lock;
    std::atomic<int> waiters;
    uint32_t *waitAddr;
    unordered_set_member_hook<> member_hook_;
    FutexBucket(uint32_t *a) { waitAddr = a; }
    FutexQueue& getQueue() { return q; }
    uint32_t *getWaitAddr() { return waitAddr; };
    void enqueue(SchedEntity &t) { q.push_back(t); }
    SchedEntity& pop_front() {
        /* CAUTION: pop an empty queue would cause error */
        SchedEntity &r = q.front();
        q.pop_front();
        return r;
    }
    void remove(SchedEntity &se) {
        auto it = q.iterator_to(se);
        libos_print("We are goona leave the sleep queue.");
        if (it == q.end()) {
            libos_print("No such a thread: %d", se.thread->id);
            return ;
        }
        libos_print("remove thread: %d", (*it).thread->id);
        q.erase(q.iterator_to(se));
    }

    friend bool operator== (const FutexBucket &a, const FutexBucket &b)
    {  return a.waitAddr == b.waitAddr;  }

    friend std::size_t hash_value(const FutexBucket &value)
    {  return std::size_t(value.waitAddr); }
};

struct BucketEqual {
    bool operator()(const FutexBucket &lhs, const FutexBucket &rhs) const {
        return lhs.waitAddr == rhs.waitAddr;
    }
};

struct BucketHash {
    std::size_t operator()(FutexBucket const &t) const {
        boost::hash<uint32_t *> hasher;
        size_t res = hasher(t.waitAddr);
        libos_print("[Hasher] 0x%lx -> 0x%lx", (uint64_t)t.waitAddr, res);
        return res;
    }
};

typedef intrusive::unordered_set<FutexBucket /*,
                                 intrusive::equal<BucketEqual>,
                                 intrusive::hash<BucketHash>*/> FutexHash;

template class intrusive::equal<BucketEqual>;
template class intrusive::hash<BucketHash>;
template class intrusive::unordered_set<FutexBucket/*, intrusive::equal<BucketEqual>, intrusive::hash<BucketHash>*/>;

extern SpinLockNoTimer futexHashLock;
extern FutexHash *futexHash;
extern FutexHash::bucket_type *futex_queue_buckets;


inline void INIT_FUTEX_QUEUE(void) {
    futex_queue_buckets = new FutexHash::bucket_type[MAX_FUTEX_BUCKETS];
    futexHash = new FutexHash(FutexHash::bucket_traits(futex_queue_buckets, MAX_FUTEX_BUCKETS));
/*
    UserThread u(123);
    UserThread u2(1234);
    UserThread u3(1235);
    UserThread u4(12);
    UserThread u5(13);

    libos_print("testing thread created.");
    FutexBucket fb((uint32_t *)0x7f7280393ec0);
    fb.enqueue(u.se);
    FutexBucket fb2((uint32_t *)0x7f7280393ec4);
    libos_print("testing bucket created.");
    futexHash->insert(fb);
    futexHash->insert(fb2);

    fb.pop_front();
    futexHash->erase((uint32_t*)0x7f7280393ec0);
    futexHash->erase((uint32_t*)0x7f7280393ec4);
    libos_print("Test passed.");
*/
}

extern "C" uint64_t libos_futex(uint32_t *addr, int op, uint32_t val,
                                uint64_t utime, uint32_t *addr2, uint32_t val3);
#endif

