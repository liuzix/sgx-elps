#include <hint.h>
#include <cstddef>
#include <stdio.h>
#include <stdlib.h>
#include "../libOS/mmap.h"
#include "../libOS/panic.h"
#include <pthread.h>

#define MEM_SIZE 0x1000000
#define STEP     0x10000

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

int main(int argc, char **argv)
{
    void *addr = libos_mmap(NULL, MEM_SIZE);
    int j = 0;
    unsigned long self;
    pthread_t p;

    p = pthread_self();
    libos_print("tid: 0x%lx", ((pthread *)p)->tid);
    return 0;
    libos_print("application addr: 0x%lx", (uint64_t)addr);
    if (addr == (void *)-1) {
       return -1;
    }
    //j = *(int *)(0x0);
    for(size_t i = 0; i < MEM_SIZE; i += STEP)
    {
        //uint64_t jiffies = *pjiffies;
        hint((void *)((char *)addr + i));
        *((char *)addr + i) = 'e';
        //libos_print("application addr: %ld", (uint64_t)((char *)addr + i));
        //libos_print("jiffies: %ld", *pjiffies - jiffies);
        j++;
    }

    libos_print("total access: %d", j);
    return 0;
}

