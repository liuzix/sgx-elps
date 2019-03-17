#include <ssa_dump.h>
#include <spdlog/spdlog.h>
#include "logging.h"
#include <iostream>

ssa_gpr_t ssa_gpr_dump;

#define __print_ssa_gpr(reg) \
        std::cout << "reg: " << std::hex <<ssa_gpr_dump.reg

void print_ssa_gpr(void) {
    std::cout << "-------- dump ssa_gpr info --------" << std::endl;
    __print_ssa_gpr(ax) << "\t";
    __print_ssa_gpr(cx) << "\n";
    __print_ssa_gpr(bx) << "\t";
    __print_ssa_gpr(dx) << "\n";
    __print_ssa_gpr(sp) << "\t";
    __print_ssa_gpr(bp) << "\n";
    __print_ssa_gpr(si) << "\t";
    __print_ssa_gpr(di) << "\n";
    __print_ssa_gpr(r8) << "\t";
    __print_ssa_gpr(r9) << "\n";
    __print_ssa_gpr(r10) << "\t";
    __print_ssa_gpr(r11) << "\n";
    __print_ssa_gpr(r12) << "\t";
    __print_ssa_gpr(r13) << "\n";
    __print_ssa_gpr(r14) << "\t";
    __print_ssa_gpr(r15) << "\n";
    __print_ssa_gpr(flags) << "\t";
    __print_ssa_gpr(ip) << "\n";
    __print_ssa_gpr(sp_u) << "\t";
    __print_ssa_gpr(bp_u) << "\n";
    __print_ssa_gpr(fs) << "\t";
    __print_ssa_gpr(gs) << "\n";
    std::cout << "exit_info.vector: " << ssa_gpr_dump.exit_info.vector << std::endl;
    std::cout << "exit_info.exit_type: " << ssa_gpr_dump.exit_info.exit_type << std::endl;
    std::cout << "exit_info.valid: " << ssa_gpr_dump.exit_info.valid << std::endl;
    std::cout << "-------- dump ssa_gpr info end --------" << std::endl;

    exit(-1);
}

void dump_ssa_gpr(ssa_gpr_t *ssa_gpr) {
    ssa_gpr_dump = *ssa_gpr;
    asm volatile ( "mov $0x4, %%rax\n\t"
                "mov %0, %%rbx\n\t"
                "mov %1, %%rcx\n\t"
                "enclu"
                :
                : "m" (print_ssa_gpr), "m"(__aex_handler)
                    );
}

void dump_ssa(uint64_t ptcs) {
    tcs_t *tcs = (tcs_t *)ptcs;
    ssa_gpr_t *ssa_gpr = (ssa_gpr_t *)(tcs->ossa + tcs->cssa * PAGE_SIZE + PAGE_SIZE - GPRSGX_SIZE);

    dump_ssa_gpr(ssa_gpr);
}
