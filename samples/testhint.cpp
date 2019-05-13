#include <hint.h>
#include <time.h>
#include <cstddef>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
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

uint32_t futex_flag[10];
int accessWork(int tid) {
    libos_print("Worker Thread Launched.");
    void *addr = libos_mmap(NULL, MEM_SIZE);
    int j = 0;

    volatile uint64_t jif = *pjiffies;
    asm("":::"memory");
    for(size_t i = 0; i < MEM_SIZE; i += STEP)
    {
        //uint64_t jif = *pjiffies;
        hint((void *)((char *)addr + i));
        *((char *)addr + i) = 'e';
        j++;
        int tt = 5000;
        while (tt--);
        //jif = *pjiffies - jif;
        //libos_print("[%d] hint CPU cycles: %ld", tid, jif);
    }

    jif = *pjiffies - jif;
    asm("":::"memory");
    libos_print("[%d] main CPU cycles: %ld", tid, jif);
    libos_print("total access: %d", j);
    futex_flag[tid] = 1;
    libos_futex(futex_flag + tid, FUTEX_WAKE, 1, 0, 0, 0);
    return 0;
}

int testMalloc(int tid) {
	size_t i = 100;
	size_t jif = *pjiffies, total = 0;

	while (i--) {
		jif = *pjiffies;
		char *buf = (char *)malloc(i % 4096 * sizeof(unsigned int) + 1);

		if (!buf) {
			std::cout << "[" << tid << "]" <<"malloc failed." << std::endl;
			break;
		}
		*buf = 'e';
		jif = *pjiffies - jif;
		total += jif;
		//free(buf);
	}
	std::cout << "[" << tid << "]" << "jif: " << total << std::endl;
	futex_flag[tid] = 1;
	libos_futex(futex_flag + tid, FUTEX_WAKE, 1, 0, 0, 0);
	return 0;
}

int contextWorker(int) {
    for (;;) {}
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
    vector<UserThread *> vSleepThreads;
    int n = 4;
    for (int i = 0; i < n; i++) {
        UserThread *t= new UserThread(std::bind(testMalloc, i));
        vSleepThreads.push_back(t);
        scheduler->enqueueTask(t->se);
    }

    for (int i = 0; i < n; i++)
        libos_futex(futex_flag + i, FUTEX_WAIT, 0, 0, 0, 0);

    return 0;
}


