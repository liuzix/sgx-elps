#ifndef SPIN_LOCK_H
#define SPIN_LOCK_H

#include <atomic>
#if IS_LIBOS
bool disableInterrupt();
void enableInterrupt();
#endif

class SpinLockNoTimer {
    std::atomic_flag locked = ATOMIC_FLAG_INIT ;
    bool interruptFlag;
public:
    void lock() {
#if IS_LIBOS
        interruptFlag = disableInterrupt();
#endif
        while (locked.test_and_set(std::memory_order_acquire)) { ; }
    }
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

/*
class SpinLock {
    std::atomic_flag locked = ATOMIC_FLAG_INIT ;
public:
    void lock() {
        while (locked.test_and_set(std::memory_order_acquire)) { ; }
    }
    void unlock() {
        locked.clear(std::memory_order_release);
    }
};

*/
using SpinLock = SpinLockNoTimer;
#endif
