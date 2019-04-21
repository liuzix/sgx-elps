#ifndef _LIBOS_FUTEX_H_
#define _LIBOS_FUTEX_H_
#include "user_thread.h"
#include "panic.cpp"

#include <boost/intrusive/unordered_set.hpp>
#include <boost/functional/hash.hpp>

#ifndef MAX_FUTEX_VAR
#define MAX_FUTEX_VAR 100
#endif

using namespace boost;

struct UserThreadEqual {
    bool operator()(const UserThread &lhs, const UserThread &rhs) const {
        return lhs.pt_local.tid == rhs.pt_local.tid ;
    }
    typedef UserThread first_argument_type;
    typedef UserThread second_argument_type;
    typedef bool result_type;
};

struct UserThreadHash {
    std::size_t operator()(UserThread const&t) const {
        boost::hash<int> hasher;
        return hasher(t.pt_local.tid);
    }
};

typedef intrusive::unordered_set<UserThread,
                                 intrusive::equal<UserThreadEqual>,
                                 intrusive::hash<UserThreadHash>> FutexQueue;

//FutexQueue *futexQueue;

inline void INIT_FUTEX_QUEUE(void) {
    FutexQueue::bucket_type futex_queue_buckets[MAX_FUTEX_VAR];
    FutexQueue futexQueue= FutexQueue(FutexQueue::bucket_traits(futex_queue_buckets, MAX_FUTEX_VAR));

    UserThread u(123);
    UserThread u2(1234);
    UserThread u3(1235);
    UserThread u4(12);
    UserThread u5(13);
    libos_print("testing thread created.");
    futexQueue.insert(u);
    //futexQueue.insert(u2);
    //futexQueue.insert(u3);
    //futexQueue.insert(u4);
    //futexQueue.insert(u5);

    //libos_print("find tid: %d.\n", (futexQueue.find(123)));
    //libos_print("find tid: %d.\n", *(futexQueue->find(1234))->pt_local.tid);
    //libos_print("find tid: %d.\n", *(futexQueue->find(13))->pt_local.tid);
}

#endif

