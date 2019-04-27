#include <hint.h>
#include <cstddef>
#include <stdio.h>
#include <stdlib.h>
#include "../libOS/mmap.h"
#include "../libOS/panic.h"
#include "../libOS/futex.h"

__thread int test = 0;
#define MEM_SIZE 0x1000000
#define STEP     0x10000

uint32_t j = 0, k = 0, wait_main = 0, wait_forever = 0;

struct pthread;

int workerThread(void) {
    unsigned long i;
    unsigned long ret;
    libos_print("Worker Thread Launched.");
    getSharedTLS()->inInterrupt->store(false);
    while (i < 0xffffff)
        i++;
    i = 0;
    while (1) {
         //ret = libos_futex(&j, FUTEX_WAKE, 0, 0, 0, 0);
         libos_futex(&j, FUTEX_REQUEUE, 1, 5, &k, 0);
         libos_print("[2]: Requeue finished.");
         while (i < 0xffffff)
             i++;
         break;
    }
    while (1) {
        int ret = libos_futex(&k, FUTEX_WAKE, 1, 0, 0, 0);
        libos_print("[13]: Wake up finished: %d.", ret);
        if (ret == 0) {
            libos_print("[13] All process waked up.");
            libos_futex(&wait_main, FUTEX_WAKE, 1, 0, 0, 0);
            libos_print("[13] main process waked up.");
            break;
        }
        while (i < 0xfffffff)
            i++;
        i = 0;

        libos_futex(&j, FUTEX_REQUEUE, 1, 5, &k, 0);
        libos_print("[13]: Requeue finished. %d", scheduler->queueSize());
        libos_print("Interrupt status: %d", (int)*getSharedTLS()->inInterrupt);
        while (i < 0xfffffff)
            i++;
        i = 0;
    }
    for (int i = 0; i < 0xfffffffffff; i++) ;
    libos_print("[13]: I am gonna sleep forever.");
    libos_futex(&wait_forever, FUTEX_WAIT, 0, 0, 0, 0);

    return 0;
}

int sleepThread(int i) {
    while (1) {
        libos_print("[%d] ready to sleep.", i);
        libos_futex(&j, FUTEX_WAIT, 0, 0, 0, 0);
        libos_print("[%d] wake up.", i);
        libos_futex(&wait_forever, FUTEX_WAIT, 0, 0, 0, 0);
        break;
    }
    return 0;
}

int main(int argc, char **argv)
{
/*
    test ++;
    void *addr = libos_mmap(NULL, MEM_SIZE);
    unsigned long self;

    libos_print("[1]: main thread launched.");

    vector<UserThread *> vSleepThreads;
    for (int i = 2; i < 13; i++) {
        UserThread *t= new UserThread(std::bind(sleepThread, i));
        vSleepThreads.push_back(t);
        scheduler->enqueueTask(t->se);
    }
    for (int i = 0; i < 0xfffffff; i++) ;

    auto worker = new UserThread(workerThread);
    scheduler->enqueueTask(worker->se);

    libos_print("[1] We are gonna sleep!");
    libos_futex(&wait_main, FUTEX_WAIT, 0, 0, 0, 0);
    libos_print("[1] I am wake up.");

    return 0;

    int i = 30;
    while (i--) {
        libos_print("[1] We are gonna sleep!");
        libos_futex(&j, FUTEX_WAIT, 0, 0, 0, 0);
        libos_print("[1] I am wake up.");
        return 0;
    }


    asm ("mov %%fs:0,%0" : "=r" (self) );
    libos_print("self: 0x%lx", ((pthread *)self)->tid);
    return 0;
    while (j < 0xfffffffffffffff)
        j++;
    libos_print("application addr: 0x%lx", (uint64_t)addr);
    if (addr == (void *)-1) {
       return -1;
    }
*/

    void *addr = libos_mmap(NULL, MEM_SIZE);
    int j = 0;
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


