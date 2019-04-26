#pragma once
#include "sched.h"
#include <boost/context/detail/fcontext.hpp>
#include <functional>
#include <boost/intrusive/unordered_set.hpp>
#include <boost/functional/hash.hpp>

extern void *tlsBase;
extern size_t tlsLength;

#define STACK_SIZE 8192
using namespace std;
using namespace boost::context::detail;
using namespace boost;

struct pthread {
    /* Part 1 -- these fields may be external or
     * internal (accessed via asm) ABI. Do not change. */
    struct pthread *self;
    uintptr_t *dtv;
    struct pthread *prev, *next; /* non-ABI */
    uintptr_t sysinfo;
    uintptr_t canary, canary2;

    /* Part 2 -- implementation details, non-ABI. */
    int tid = 0xBEEFBEEF;
    int errno_val;
    volatile int detach_state;
    volatile int cancel;
    volatile unsigned char canceldisable, cancelasync;
    unsigned char tsd_used:1;
    unsigned char dlerror_flag:1;
    unsigned char *map_base;
    size_t map_size;
    void *stack;
    size_t stack_size;
    size_t guard_size;
    void *result;
    struct __ptcb *cancelbuf;
    void **tsd;
    struct {
        volatile void *volatile head;
        long off;
        volatile void *volatile pending;
    } robust_list;
    volatile int timer_id;
    locale_t locale;
    volatile int killlock[1];
    char *dlerror_buf;
    void *stdio_locks;

    /* Part 3 -- the positions of these fields relative to
     * the end of the structure is external and internal ABI. */
    uintptr_t canary_at_end;
    uintptr_t *dtv_copy;
};

class UserThread : public boost::intrusive::list_base_hook<> {
public:
    fcontext_t fcxt;
    pthread *pt_local;
    uint64_t preempt_stack;
    function<int(void)> entry;
    SchedEntity se;
    int id;
    void jumpTo(UserThread *from);
    void *request_obj;
    /* for creating new thread */
    UserThread(function<int(void)> _entry);
    UserThread(int tid);
    intrusive::list_member_hook<> member_hook_;
    void terminate(int val);
};

pthread *allocateTCB();

