#include <control_struct.h>
#include <functional>
#include <string.h>
#include "panic.h"
#include "libos.h"
#include "allocator.h"
#include "sched.h"
#include "mmap.h"
#include "thread_local.h"
#include "user_thread.h"
#include <request.h>
#include "singleton.h"
#include <syscall_format.h>
#include "elf.h"
#include "logging.h"
#include "futex.h"

#include <vector>
#include <list>

Queue<RequestBase*> *requestQueue = nullptr;

extern "C" void __temp_libc_start_init(void);
extern "C" void __eexit(int ret);
extern "C" int __async_swap(void *addr);
extern "C" int __libc_start_main(int (*main)(int,char **,char **), int argc, char **argv);
//extern Singleton<SwapRequest> sg;
void initWatchList();


uint64_t *pjiffies;

int idleThread() {
    for (;;) {
        libos_print("idling!");
        getSharedTLS()->numActiveThread --;
        __eexit(0x1000); // yield cpu
        scheduler->schedule();
    }
    return 0;
}

int newThread(int argc, char **argv) {
    libos_print("We are in a new thread!");
    libos_print("Enabling interrupt");
    getSharedTLS()->inInterrupt->store(false);

    INIT_FUTEX_QUEUE();
    //auto schedReady = createUnsafeObj<SchedulerRequest>(SchedulerRequest::SchedulerRequestType::SchedReady);
    auto schedReady = Singleton<SchedulerRequest>::getRequest(SchedulerRequest::SchedulerRequestType::SchedReady);
    requestQueue->push(schedReady);
    int ret = main(argc, argv);
    //int ret = __libc_start_main((int (*)(int,char **,char **))&main, argc, argv);
    __eexit(ret);
    return 0;
}

void test_auxv(uint64_t sp, int argc) {
    char **argv = (char **)sp;
    char **envp = argv + argc + 1;
    size_t aux[40], *auxv;
    char buf[128];
    sprintf(buf, "argv: %lx, envp: %lx", (uint64_t)argv, (uint64_t)envp);
    libos_print(buf);

    int i = 0;
    for (i = 0; argv[i]; i++)
        libos_print(argv[i]);

    for (i = 0; envp[i]; i++) {}
    //    libos_print(envp[i]);

    auxv = (size_t *)(envp+i+1);

    for (i = 0; auxv[i]; i += 2)
        aux[auxv[i]] = auxv[i + 1];

#define print_auxv(name, fmt, type)                   \
    do {                                        \
        char buf[512];                           \
                                                \
        sprintf(buf, fmt, #name, (type)aux[name]); \
        libos_print(buf);                       \
    } while(0)

    print_auxv(AT_HWCAP, "%s: 0x%lx", uint64_t);
    print_auxv(AT_PAGESZ, "%s: 0x%d", int);
    print_auxv(AT_CLKTCK, "%s: 0x%d", int);
    print_auxv(AT_PHDR, "%s: 0x%lx", uint64_t);
    print_auxv(AT_PHENT, "%s: 0x%d", int);
    print_auxv(AT_PHNUM, "%s: 0x%d", int);
    print_auxv(AT_BASE, "%s: 0x%lx", uint64_t);
    print_auxv(AT_FLAGS, "%s: 0x%lx", uint64_t);
    print_auxv(AT_ENTRY, "%s: 0x%lx", uint64_t);
    print_auxv(AT_UID, "%s: 0x%d", int);
    print_auxv(AT_EUID, "%s: 0x%d", int);
    print_auxv(AT_GID, "%s: 0x%d", int);
    print_auxv(AT_EGID, "%s: 0x%d", int);
    print_auxv(AT_SECURE, "%s: 0x%d", int);
    print_auxv(AT_RANDOM, "%s: 0x%lx", uint64_t);
    print_auxv(AT_HWCAP2, "%s: 0x%lx", uint64_t);
    print_auxv(AT_EXECFN, "%s: %s", char *);
    print_auxv(AT_PLATFORM, "%s: %s", char *);

#undef print_auxv
}

/* @sp: sp holds the pointer to the top of the initial stack i.e argv[0]
 *      ┌────────────────┐
 *      │   auxv[n - 1]  │ <---------- stack bottom (high address)
 *      ├────────────────┤
 *      │       .        │
 *      │       .        │
 *      │       .        │
 *      ├────────────────┤
 *      │     axuv[0]    │
 *      ├────────────────┤
 *      │ envp[n] = NULL │
 *      ├────────────────┤
 *      │       .        │
 *      │       .        │
 *      │       .        │
 *      ├────────────────┤
 *      │     envp[0]    │
 *      ├────────────────┤
 *      │ argv[n] = NULL │
 *      ├────────────────┤
 *      │       .        │
 *      │       .        │
 *      │       .        │
 *      ├────────────────┤
 *      │    argv[0]     │  <--------- sp (low address)
 *      ├────────────────┤
 *      │     stack      │
 *      │       |        │
 *      │       |        │
 *      │       |        │
 *      │       v        │
 * */
extern "C" int __libOS_start(libOS_control_struct *ctrl_struct, uint64_t sp) {
    if (!ctrl_struct)
        return -1;
    if (ctrl_struct->magic != CONTROL_STRUCT_MAGIC)
        return -1;
    if (!ctrl_struct->isMain) {
        getSharedTLS()->inInterrupt->store(false);
        libos_print("thread entering enclave. calling scheduler");
        scheduler->schedule();
        libos_print("scheduler returned!");
        __asm__("ud2");
    }

    libOS_shared_tls *shared_tls = getSharedTLS();
    pjiffies = shared_tls->pjiffies;
    timeStamp = ctrl_struct->timeStamp;
    ctrl_struct->isMain = false;
    requestQueue = ctrl_struct->requestQueue;
    initPanic(ctrl_struct->panic);
    libos_print("We are inside LibOS!");

    // set TLS initialization parameters
    tlsBase = ctrl_struct->mainArgs.tlsBase;
    tlsLength = ctrl_struct->mainArgs.tlsSize;

    initUnsafeMalloc(ctrl_struct->mainArgs.unsafeHeapBase, ctrl_struct->mainArgs.unsafeHeapLength);
    writeToConsole("UnsafeMalloc intialization successful.");
    libos_print("UnsafeHeap base = 0x%lx, length = 0x%lx", ctrl_struct->mainArgs.unsafeHeapBase,
                ctrl_struct->mainArgs.unsafeHeapLength);

    mmap_init(ctrl_struct->mainArgs.heapBase, ctrl_struct->mainArgs.heapLength);
    initSafeMalloc(10 * 4096);
    libos_print("Safe malloc initialization successful");
    //test_auxv(sp, ctrl_struct->mainArgs.argc);

    initSyscallTable();
    scheduler_init();
    initWatchList();
    scheduler->setIdle((new UserThread(idleThread))->se);
    auto mainThr = new UserThread(std::bind(newThread, ctrl_struct->mainArgs.argc, ctrl_struct->mainArgs.argv));
    scheduler->enqueueTask(mainThr->se);
    scheduler->schedule();
    libos_panic("Shouldn't have reached here!");
    __asm__("ud2");
    return 0;
}

