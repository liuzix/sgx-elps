#ifndef CONTROL_STRUCT_H
#define CONTROL_STRUCT_H

#include <chrono>
#include <queue.h>
#include <request.h>
#include <sgx_arch.h>
#include <spin_lock.h>
using namespace std;
using namespace std::chrono;

#define CONTROL_STRUCT_MAGIC 0xbeefbeef
#define AUX_CNT 38

struct main_args_t {
    int argc;     /* 0 */
    char **argv;  /* 8 */
    int envc;     /* 16 */
    char **envp;  /* 24 */
    size_t *auxv; /* 32 */

    vaddr heapBase;
    size_t heapLength;
    void *unsafeHeapBase;
    size_t unsafeHeapLength;
    void *tlsBase;
    size_t tlsSize;
};

struct panic_struct {
    SpinLockNoTimer lock;

    char panicBuf[1024];
    struct {
    } __attribute__((aligned(16)));
    char requestBuf[sizeof(DebugRequest)];
    char requestTest[sizeof(DebugRequest)];
};

struct libOS_control_struct {
    main_args_t mainArgs;

    /* for debugging */
    unsigned int magic = CONTROL_STRUCT_MAGIC;

    bool isMain;

    Queue<RequestBase *> *requestQueue;

    panic_struct *panic;

    std::atomic<steady_clock::duration> *timeStamp;
};

#endif
