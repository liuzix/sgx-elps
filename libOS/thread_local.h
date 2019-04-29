#pragma once
#include <stddef.h>
#include <stdint.h>
#include <spin_lock.h>
#include <unordered_map>
#include <libOS_tls.h>
#include "util.h"
using namespace std;

template <typename T>
class PerCPU {
private:
    SpinLockNoTimer lock;
    unordered_map<uint64_t, T> map;
public:
    T &operator *() {
        uint64_t cpuID = getSharedTLS()->threadID;
        lock.lock();
        T &ret = map[cpuID];
        lock.unlock();
        return ret;
    }

    T &get() {
        return **this;
    }
};

