#ifndef LIBOS_TLS_H
#define LIBOS_TLS_H

#include <atomic>
#include <stdint.h>

struct libOS_control_struct;
#define PRINT_BUFFER_SIZE (1 << 17)
#define SAFE_HEAP_LEN (256 * 1024 * 1024)
#define PAGE_SIZE 0x1000

struct libOS_shared_tls {
    uint64_t next_exit;           // set by the loader
    uint64_t loader_stack;        // saved by the loader
    uint64_t enclave_stack;       // intialized by loader, updated by enclave on exits
    uint64_t reentry;           // set by the enclave
    uint64_t enclave_return_val;  // set by the enclave
    libOS_control_struct *controlStruct;
    uint64_t interrupt_stack;
    uint64_t loadBias;
    uint64_t threadID;
    uint64_t interrupt_exit;
    uint64_t interrupt_outside_stack;
    uint64_t preempt_injection_stack;
    std::atomic_bool *inInterrupt;
    std::atomic_int *numTotalThread;
    uint64_t isMain;
    uint64_t *pjiffies;
    uint64_t request_obj;
    int buffer_index = 0;
    char buffer[PRINT_BUFFER_SIZE];
    int *dbg_pbuffer_index;
    char *dbg_buffer;
    std::atomic_int *numActiveThread;
    int numKernelThreads = 0;
}  __attribute__ ((packed));

struct enclave_tls {
    uint64_t enclave_base;
    uint64_t enclave_size;
    uint64_t tcs_offset;
    uint64_t initial_stack_offset;
    libOS_shared_tls *libOS_data;
    void *ssa;
    void *stack;
    uint64_t preempt_rip;
}  __attribute__ ((packed));

#endif
