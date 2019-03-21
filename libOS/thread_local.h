#pragma once
#include <stddef.h>
#include <stdint.h>
#include <unordered_map>
#include <libOS_tls.h>

#define readTLSField(field) readQWordFromGS(offsetof(enclave_tls, field))

uint64_t readQWordFromGS(size_t offset);

libOS_shared_tls *getSharedTLS();

using namespace std;

template <typename T>
class PerCPU {
private:
    unordered_map<uint64_t, T> map;    
public:
    T &operator *() {
        uint64_t cpuID = getSharedTLS()->threadID;
        return &map[cpuID];
    }
};
