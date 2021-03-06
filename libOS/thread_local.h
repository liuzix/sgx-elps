#pragma once
#include <stddef.h>
#include <stdint.h>
#include <spin_lock.h>
#include <unordered_map>
#include <libOS_tls.h>
#include <type_traits>
#include "util.h"
using namespace std;

#define MAX_NUM_CPU 8

template <typename T>
class PerCPU {
private:
    T map[MAX_NUM_CPU];
public:
    PerCPU() {
        for (int i = 0; i < MAX_NUM_CPU; i++) {
            if (is_pointer<T>::value)
                map[i] = nullptr;
        }
    }
    PerCPU(int a) {}

    __attribute__ ((always_inline))
    T &operator *() {
        bool interruptFlag = disableInterrupt();
        uint64_t cpuID = getSharedTLS()->threadID;
        T &ret = map[cpuID];
        if (!interruptFlag)
            enableInterrupt();
        return ret;
    }

    T &operator[] (int cpu) {
        return map[cpu];
    }

    T &get() {
        return **this;
    }
};

