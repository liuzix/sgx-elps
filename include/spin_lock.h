#ifndef SPIN_LOCK_H
#define SPIN_LOCK_H

#include <atomic>
#if IS_LIBOS
#include "../libOS/util.h"
#endif

class SpinLockNoTimer {
    std::atomic_flag locked = ATOMIC_FLAG_INIT ;
    bool interruptFlag;
public:
    __attribute__((always_inline))
    void lock() {
#if IS_LIBOS
        bool localInterruptFlag = disableInterrupt();
#endif
        while (locked.test_and_set(std::memory_order_acquire)) {
            __asm__ __volatile__("pause;");
        }
#if IS_LIBOS
        interruptFlag = localInterruptFlag;
#endif
    }
    __attribute__((always_inline))
    void unlock() {
#if IS_LIBOS
        bool reenable = !interruptFlag;
#endif
        locked.clear(std::memory_order_release);
#if IS_LIBOS
        if (reenable) enableInterrupt();
#endif
    }
};

using SpinLock = SpinLockNoTimer;

template <typename LockT>
class LockGuard {
private:
    LockT *lock;
public:
    LockGuard(LockT &l) {
        lock = &l;
        l.lock();
    }

    ~LockGuard() {
        lock->unlock();
    }
};

#endif
