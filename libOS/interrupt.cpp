#include "interrupt.h"
#include "sched.h"
#include "panic.h"
#include "util.h"
#include "user_thread.h"
#include "thread_local.h"
void injectedFunc();
#define SSAFRAME_SIZE 4
extern "C" void do_interrupt(void *tcs) {
    extern const char no_interrupt_begin[], no_interrupt_end[];
    //libos_print("do_interrupt!");
    ssa_gpr_t *ssa_gpr = (ssa_gpr_t *)((char *)tcs + 4096 + 4096 * SSAFRAME_SIZE - GPRSGX_SIZE);
    //libos_print("do_interrupt. rip in file: 0x%lx", 
    //       ssa_gpr->ip - getSharedTLS()->loadBias);
    //libos_print("no_interrupt_begin 0x%lx, end 0x%lx",
    //        (uint64_t)no_interrupt_begin, (uint64_t)no_interrupt_end);
    if ((uint64_t)no_interrupt_begin <= ssa_gpr->ip
            && ssa_gpr->ip < (uint64_t)no_interrupt_end) {

        //libos_print("no interrupt zone, not injecting");
        __interrupt_exit();
        __asm__("ud2");
    }
    writeTLSField(preempt_rip, ssa_gpr->ip);
    writeTLSField(stack, ssa_gpr->sp);
    ssa_gpr->ip = (uint64_t)&injectedFunc;
    ssa_gpr->sp = (uint64_t)getSharedTLS()->preempt_injection_stack;
    __interrupt_exit();
    __asm__("ud2");
}

__attribute__((naked)) void injectedFunc() {
   __asm__ volatile("no_interrupt_begin:"
           "push %rbp;"
           "mov %rsp, %rbp;"
           "push %rax;"
           "push %rbx;"
           "push %rcx;"
           "push %rdx;"
           "push %rsi;"
           "push %rdi;"
           "push %r8;"
           "push %r9;"
           "push %r10;"
           "push %r11;" 
           "push %r12;"
           "push %r13;"
           "push %r14;"
           "push %r15;"
           "pushfq;"
           "push %gs:48;"
           "push %gs:56;"
           "call do_preempt;"
           "pop %gs:56;"
           "pop %gs:48;"
           "popfq;"
           "pop %r15;"
           "pop %r14;"
           "pop %r13;"
           "pop %r12;"
           "pop %r11;"
           "pop %r10;"
           "pop %r9;"
           "pop %r8;"
           "pop %rdi;"
           "pop %rsi;"
           "pop %rdx;"
           "pop %rcx;"
           "pop %rbx;"
           "pop %rax;"
           "pop %rbp;"
           "mov %gs:48, %rsp;"
           "jmp *%gs:56;"
           "no_interrupt_end:");
}

extern "C" void do_preempt() {
    //libos_print("preempt function injected!");
    void *xsave = scheduler->getCurrent()->get()->thread->xsaveRegion;
    __builtin_ia32_xsaveopt64(xsave, ~0);
    scheduler->schedule();
    __builtin_ia32_xrstor64(xsave, ~0);
    //libos_print("returning to: 0x%lx", readTLSField(preempt_rip));
}
