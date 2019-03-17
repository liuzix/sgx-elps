#include <ssa_dump.h>
#include <spdlog/spdlog.h>
#include "logging.h"
#include <iostream>

std::map<uint64_t, char> sig_flag_map;
ssa_gpr_t ssa_gpr_dump;

char get_flag(uint64_t rbx) {
    char res = sig_flag_map[rbx];
    //console->log("rbx: 0x{:x}, flag: {} ", rbx, res);
    return res;
}

void set_flag(uint64_t rbx, char flag) {
    sig_flag_map[rbx] = flag;
}


void sig_exit() {
    exit(-1);
}

static void __sigaction(int sig, siginfo_t *info, void *ucontext) {
    ucontext_t *context = (ucontext_t *)ucontext;
    uint64_t rbx = context->uc_mcontext.gregs[REG_RBX];

    //Per-thread flag
    set_flag(rbx, 1);
    console->error("Segmentation Fault!");
}

void dump_sigaction(void) {
    sigset_t msk = {0};
    struct sigaction sa;
    {
        sa.sa_handler = NULL;
        sa.sa_sigaction = __sigaction;
        sa.sa_mask = msk;
        sa.sa_flags = SA_SIGINFO;
        sa.sa_restorer = NULL;
    }

    sigaction(SIGSEGV, &sa, NULL);

}


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

void dump_ssa(uint64_t ptcs, uint32_t ssaframesize) {
    tcs_t *tcs = (tcs_t *)ptcs;
    ssa_gpr_t *ssa_gpr = (ssa_gpr_t *)(tcs->ossa + tcs->cssa * PAGE_SIZE + PAGE_SIZE - GPRSGX_SIZE);

    dump_ssa_gpr(ssa_gpr);
}
