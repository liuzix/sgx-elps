#pragma once
#include <cstdint>
#include <cstddef>
#include <libOS_tls.h>

#define readTLSField(field) readQWordFromGS(offsetof(enclave_tls, field))
#define writeTLSField(field, value) writeQWordToGS(offsetof(enclave_tls, field), value)

#define LIBOS_INLINE __attribute__((always_inline)) static inline

#define LIBOS_UNUSED __attribute__((unused))

LIBOS_INLINE
uint64_t readQWordFromGS(size_t offset) {
    uint64_t ret;
    asm volatile ("movq %%gs:(%1), %0"
            : "=r" (ret): "r"(offset));
    return ret;
}

LIBOS_INLINE
void writeQWordToGS(size_t offset, uint64_t value) {
    asm volatile ("movq %0, %%gs:(%1)"
            ::"r" (value), "r"(offset));
}


LIBOS_INLINE
libOS_shared_tls *getSharedTLS() {
    return (libOS_shared_tls *)readTLSField(libOS_data);
}

LIBOS_INLINE
bool disableInterrupt() {
    return getSharedTLS()->inInterrupt->exchange(true);
}

LIBOS_INLINE
void enableInterrupt() {
    return getSharedTLS()->inInterrupt->store(false);
}
/*
uint64_t readQWordFromGS(size_t offset);
void writeQWordToGS(size_t offset, uint64_t value);
libOS_shared_tls *getSharedTLS();
bool disableInterrupt();
void enableInterrupt();
*/

#define LIBOS_FATAL(...) do {  \
    libos_print(__VA_ARGS__); \
    __asm__ volatile ("ud2"); \
} while (0)

#ifdef LIBOS_DEBUG
#define LIBOS_ASSERT(x) do {   \
    if (x) break;             \
    LIBOS_FATAL("LibOS fatal: %s, line %d", \
            __FILE__, __LINE__); \
} while (0)
#else
#define LIBOS_ASSERT(x) do {} while (0)
#endif
