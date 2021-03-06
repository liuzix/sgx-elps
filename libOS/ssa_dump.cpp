#include <ssa_dump.h>
#include <spdlog/spdlog.h>
#include "logging.h"
#include <iostream>
#include <cstring>
#include "panic.h"
#include "thread_local.h"
#include "allocator.h"
#include "sched.h"
#include "user_thread.h"

extern "C" int __eexit(int);
ssa_gpr_t ssa_gpr_dump;
extern atomic_int futex_counter;

//void dumpWatchList();
#ifdef HAS_COUT
#define __print_ssa_gpr(reg, suffix) \
        std::cout << "reg: " << std::hex << ssa_gpr_dump.reg << suffix
#else
#define __print_ssa_gpr(reg, suffix) \
    do {                            \
        char buf[50];               \
                                    \
        sprintf(buf, "%s:\t0x%016lx", #reg, ssa_gpr_dump.reg); \
        writeToConsole(buf);                           \
    } while(0)
#define __print_exit_info(name, suffix) \
    do {                                \
        char buf[50];                   \
                                        \
        sprintf(buf, "exit_info.%s: %d", #name, ssa_gpr_dump.exit_info.name); \
        writeToConsole(buf);                                               \
    } while(0)
#endif

void print_ssa_gpr(void) {
#ifdef HAS_COUT
    std::cout << "-------- dump ssa_gpr info --------" << std::endl;
#else
    libos_panic("-------- dump ssa_gpr info --------");
#endif
    libos_print("bias = 0x%lx", getSharedTLS()->loadBias);
    __print_ssa_gpr(ax, "\t");
    __print_ssa_gpr(cx, "\n");
    __print_ssa_gpr(bx, "\t");
    __print_ssa_gpr(dx, "\n");
    __print_ssa_gpr(sp, "\t");
    __print_ssa_gpr(bp, "\n");
    __print_ssa_gpr(si, "\t");
    __print_ssa_gpr(di, "\n");
    __print_ssa_gpr(r8, "\t");
    __print_ssa_gpr(r9, "\n");
    __print_ssa_gpr(r10, "\t");
    __print_ssa_gpr(r11, "\n");
    __print_ssa_gpr(r12, "\t");
    __print_ssa_gpr(r13, "\n");
    __print_ssa_gpr(r14, "\t");
    __print_ssa_gpr(r15, "\n");
    __print_ssa_gpr(flags,  "\t");
    __print_ssa_gpr(ip, "\n");
    __print_ssa_gpr(sp_u, "\t");
    __print_ssa_gpr(bp_u, "\n");
    __print_ssa_gpr(fs, "\t");
    __print_ssa_gpr(gs, "\n");
    uint64_t rip_in_file = ssa_gpr_dump.ip - getSharedTLS()->loadBias;
    libos_print("rip in file: 0x%lx", rip_in_file);
    libos_print("preempt rip in file: 0x%lx, interrupt flag: %d", readTLSField(preempt_rip) - getSharedTLS()->loadBias,
                                                                  (int)*((libOS_shared_tls *)readTLSField(libOS_data))->inInterrupt);
#ifdef HAS_COUT
    std::cout << "exit_info.vector: " << ssa_gpr_dump.exit_info.vector << std::endl;
    std::cout << "exit_info.exit_type: " << ssa_gpr_dump.exit_info.exit_type << std::endl;
    std::cout << "exit_info.valid: " << ssa_gpr_dump.exit_info.valid << std::endl;
    std::cout << "-------- dump ssa_gpr info end --------" << std::endl;
#else
    libos_panic("-------- dump ssa_gpr info end --------");
#endif
}

void dump_ssa_gpr(ssa_gpr_t *ssa_gpr) {
    ssa_gpr_dump = *ssa_gpr;
    print_ssa_gpr();
}

void do_backtrace(uint64_t *rbp, uint64_t rip) {
    libos_print("start: 0x%lx", rip - getSharedTLS()->loadBias);
    while (*rbp) {
        uint64_t func = *(rbp + 1);
        libos_print("rip: 0x%lx", func - getSharedTLS()->loadBias);
        rbp = (uint64_t *)*rbp;
    }
    libos_print("backtrace ends");
    libos_panic("--------check sum of all chunks... --------");
    safeAllocator->checkSumAll();
}

SpinLock dumpLock;
#define SSAFRAME_SIZE 4
extern "C" void dump_ssa(uint64_t ptcs) {
    dumpLock.lock();
    tcs_t *tcs = (tcs_t *)ptcs;
    ssa_gpr_t *ssa_gpr = (ssa_gpr_t *)((char *)tcs + 4096 + 4096 * SSAFRAME_SIZE - GPRSGX_SIZE);
    libos_panic("Ready to dump.");
    size_t queue_len = scheduler->eachQueue.get().size();
    libos_print("Local queue length = %d", queue_len);
    libos_print("Current thread id: %d", (*(scheduler->current))->thread->id);
    libos_print("-------- Local queue ---------");
    while (queue_len) {
        auto& t = scheduler->eachQueue.get().front();

        libos_print("thread %d", t.thread->id);
        scheduler->eachQueue.get().pop_front();
        queue_len--;
    }
    libos_print("-------- Local queue end ---------");

    libos_print("Threading sleeping on futex = %d", futex_counter.load());
    //dumpWatchList();
    dump_ssa_gpr(ssa_gpr);
    do_backtrace((uint64_t *)ssa_gpr->bp, ssa_gpr->ip);
    dumpLock.unlock();
    __eexit(-1);
    __asm__("ud2");
}
