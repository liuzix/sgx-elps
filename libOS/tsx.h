#pragma once
#include <spin_lock.h>
#include <immintrin.h>
#include "util.h"
class TransLock {
    int lockvar = 0;
    bool interruptFlag;
public:
    void lock() {
        bool localInterruptFlag = disableInterrupt();
        while (__atomic_exchange_n(&lockvar, 1, __ATOMIC_ACQUIRE |__ATOMIC_HLE_ACQUIRE))
            _mm_pause(); /* Abort failed transaction */
        interruptFlag = localInterruptFlag;
    }

    void unlock() {
        bool reenable = !interruptFlag;
        __atomic_store_n(&lockvar, 0, __ATOMIC_RELEASE | __ATOMIC_HLE_RELEASE);
        if (reenable) enableInterrupt();
    }
}__attribute__((packed));



