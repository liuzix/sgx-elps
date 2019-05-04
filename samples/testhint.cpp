#include <hint.h>
#include <time.h>
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
int randarr[] = {38978, 36044, 71999, 29451, 38358, 62752, 62947, 63576, 19430, 64894, 76316, 30149, 34561, 55391, 42922, 75683, 23965, 13387, 70725, 63887, 70251, 36409, 39296, 39003, 64312, 40984, 29656, 68797, 25598, 17437, 77413, 74716, 25141, 49146, 13523, 12424, 63197, 50230, 31043, 17391, 26355, 27500, 37061, 10097, 40570, 60738, 67314, 21806, 27115, 20575, 11103, 47902, 78297, 64034, 70755, 32605, 37901, 52298, 76601, 69815, 43693, 34293, 79897, 39941, 70226, 9905, 30166, 73864, 74103, 69169, 34821, 51048, 65918, 22885, 61992, 40291, 31031, 50177, 75944, 20208, 47458, 15151, 26138, 51697, 27980, 58991, 32248, 27517, 26537, 21627, 67506, 69659, 62562, 35548, 51254, 17660, 26564, 67202, 79120, 41036, 10819, 69198, 49669, 44229, 64791, 70142, 77726, 41335, 19609, 18924, 44745, 50899, 52097, 50433, 78253, 58199, 61019, 29245, 79871, 20178, 42069, 46116, 50561, 10864, 51628, 78182, 75445, 74179, 48307, 24034, 69592, 64112, 48129, 65638, 54306, 27039, 57559, 63643, 15447, 15268, 63363, 68867, 62986, 52568, 59458, 25214, 17250, 78148, 27376, 62114, 14693, 24983, 21765, 60223, 79617, 67326, 19096, 16865, 70768, 75683, 20986, 77898, 78074, 71242, 24582, 65846, 21568, 10840, 32586, 54885, 74285, 67580, 24257, 8899, 56469, 29078, 47929, 45611, 74844, 59883, 36639, 21961, 67662, 31216, 53218, 30906, 79304, 72448, 74188, 60848, 12376, 70950, 40135, 50416, 37757, 44583, 10154, 75222, 15979, 10374, 77824, 34909, 25789, 77338, 26833, 17420, 18388, 60387, 43772, 71892, 61123, 13571, 45999, 53257, 56061, 44304, 22034, 61081, 35856, 73260, 43232, 64124, 56617, 71692, 27531, 57965, 14426, 46075, 18199, 33672, 76392, 37067, 79662, 79529, 60015, 37002, 25365, 73275, 50216, 18419, 64123, 74539, 39682, 73306, 65406, 26893, 55992, 23369, 74020, 51572, 64248, 27238, 45101, 33547, 42073, 37795};
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
    int n = 3;
    for (int i = 0; i < n; i++) {
        UserThread *t= new UserThread(std::bind(accessWork, i));
        vSleepThreads.push_back(t);
        scheduler->enqueueTask(t->se);
    }

    for (int i = 0; i < n; i++)
        libos_futex(futex_flag + i, FUTEX_WAIT, 0, 0, 0, 0);

    return 0;
}


