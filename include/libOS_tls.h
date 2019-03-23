#ifndef LIBOS_TLS_H
#define LIBOS_TLS_H

#include <atomic>
#include <stdint.h>

struct libOS_control_struct;

struct libOS_shared_tls {
    uint64_t next_exit;           // set by the loader
    uint64_t loader_stack;        // saved by the loader
    uint64_t enclave_stack;       // intialized by loader, updated by enclave on exits
    uint64_t isReentry;           // set by the enclave
    uint64_t enclave_return_val;  // set by the enclave
    libOS_control_struct *controlStruct;
    uint64_t interrupt_stack;
    uint64_t loadBias;
    uint64_t threadID;
    uint64_t interrupt_exit;
    uint64_t interrupt_outside_stack;   
    std::atomic_bool *inInterrupt;
}  __attribute__ ((packed));

struct enclave_tls {
    uint64_t enclave_base;
    uint64_t enclave_size;
    uint64_t tcs_offset;
    uint64_t initial_stack_offset;
    libOS_shared_tls *libOS_data;
    void *ssa;
    void *stack;
}  __attribute__ ((packed));

#endif
