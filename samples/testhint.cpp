#include <hint.h>
#include <cstddef>
#include <stdio.h>
#include <stdlib.h>
#include "../libOS/mmap.h"
#include "../libOS/panic.h"
#include "../libOS/futex.h"

#define MEM_SIZE 0x1000000
#define STEP     0x10000

uint32_t j = 0;

int workerThread(void) {
    unsigned long i;
    libos_print("Worker Thread Launched.");
    getSharedTLS()->inInterrupt->store(false);
    while (1) {
        libos_print("[2]: Interrupt state: %d", (int)*getSharedTLS()->inInterrupt);
        libos_futex(&j, FUTEX_WAKE, 0, 0, 0, 0);
        libos_print("[2]: Wake up finished.");
        scheduler->schedule();
        while (i < 0xffffff)
            i++;
        i = 0;
    }
    return 0;
}

int main(int argc, char **argv)
{
    void *addr = libos_mmap(NULL, MEM_SIZE);
    unsigned long self;

    libos_print("[1]: main thread launched queue len: %d", scheduler->queueSize());
    auto worker = new UserThread(workerThread);
    scheduler->enqueueTask(worker->se);
    libos_print("[1]: Enqueue thread 2, queue len: %d", scheduler->queueSize());

    int i = 5;
    while (i--) {
        libos_print("We are gonna sleep!");
        libos_print("[1]: Interrupt state: %d", (int)*getSharedTLS()->inInterrupt);
        libos_futex(&j, FUTEX_WAIT, 0, 0, 0, 0);
    }

    while (j < 0xffffffffff)
        j++;
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


