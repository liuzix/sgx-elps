#include <ssa_dump.h>
#include <spdlog/spdlog.h>
#include "logging.h"
#include <iostream>
#include <cstring>
#include "panic.h"

ssa_gpr_t ssa_gpr_dump;

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
#ifdef HAS_COUT
    std::cout << "exit_info.vector: " << ssa_gpr_dump.exit_info.vector << std::endl;
    std::cout << "exit_info.exit_type: " << ssa_gpr_dump.exit_info.exit_type << std::endl;
    std::cout << "exit_info.valid: " << ssa_gpr_dump.exit_info.valid << std::endl;
    std::cout << "-------- dump ssa_gpr info end --------" << std::endl;
#else
    __print_exit_info(vector, "\n");
    __print_exit_info(exit_type, "\n");
    __print_exit_info(valid, "\n");
    libos_panic("-------- dump ssa_gpr info end--------");
    
#endif
    asm volatile("ud2"::);
}

void dump_ssa_gpr(ssa_gpr_t *ssa_gpr) {
    ssa_gpr_dump = *ssa_gpr;
    print_ssa_gpr();
}

#define SSAFRAME_SIZE 4
extern "C" void dump_ssa(uint64_t ptcs) {
    tcs_t *tcs = (tcs_t *)ptcs;
    char buf[50];
    sprintf(buf, "0x%lx", ptcs);
    libos_panic(buf);
    //ssa_gpr_t *ssa_gpr = (ssa_gpr_t *)(tcs->ossa + tcs->cssa * PAGE_SIZE + PAGE_SIZE - GPRSGX_SIZE);
    ssa_gpr_t *ssa_gpr = (ssa_gpr_t *)((char *)tcs + 4096 + 4096 * SSAFRAME_SIZE - GPRSGX_SIZE);
    libos_panic("Ready to dump.");
    dump_ssa_gpr(ssa_gpr);
    for (;;) {}
}
