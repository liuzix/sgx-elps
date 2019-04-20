#pragma once
#include <stddef.h>
#include <stdint.h>
#include <spin_lock.h>
#include <unordered_map>
#include <libOS_tls.h>

#define readTLSField(field) readQWordFromGS(offsetof(enclave_tls, field))
#define writeTLSField(field, value) writeQWordToGS(offsetof(enclave_tls, field), value)

uint64_t readQWordFromGS(size_t offset);
void writeQWordToGS(size_t offset, uint64_t value);

libOS_shared_tls *getSharedTLS();

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

<<<<<<< HEAD
bool disableInterrupt();

void enableInterrupt();
=======

>>>>>>> get cureent function; pthread_self function
