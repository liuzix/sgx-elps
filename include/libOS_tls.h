#ifndef LIBOS_TLS_H
#define LIBOS_TLS_H

#include <stdint.h>

struct libOS_shared_tls {
    uint64_t next_exit;
    uint64_t next_entry;
    uint64_t loader_stack;
    uint64_t enclave_stack;
    uint64_t isReentry;
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
