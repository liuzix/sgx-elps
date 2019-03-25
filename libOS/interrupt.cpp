#include "interrupt.h"
#include "sched.h"
#include "panic.h"

void injectedFunc();
#define SSAFRAME_SIZE 4
extern "C" void do_interrupt(void *tcs) {
    libos_print("do_interrupt!");
    
    ssa_gpr_t *ssa_gpr = (ssa_gpr_t *)((char *)tcs + 4096 + 4096 * SSAFRAME_SIZE - GPRSGX_SIZE);
    writeTLSField(preempt_rip, ssa_gpr->ip); 
    writeTLSField(stack, ssa_gpr->sp);
    ssa_gpr->ip = (uint64_t)&injectedFunc;
    ssa_gpr->sp = (uint64_t)getSharedTLS()->preempt_injection_stack;
    __interrupt_exit();
    __asm__("ud2");
}

__attribute__((naked)) void injectedFunc() {
   __asm__("push %rbp;"
           "mov %rsp, %rbp;"
           "push %rax;"
           "push %rcx;"
           "push %rdx;"
           "push %rsi;"
           "push %rdi;"
           "push %r8;"
           "push %r9;"
           "push %r10;"
           "push %r11;" 
           "pushfq;"
           "push %gs:48;"
           "push %gs:56;"
           "call do_preempt;"
           "pop %gs:56;"
           "pop %gs:48;"
           "popfq;"
           "pop %r11;"
           "pop %r10;"
           "pop %r9;"
           "pop %r8;"
           "pop %rdi;"
           "pop %rsi;"
           "pop %rdx;"
           "pop %rcx;"
           "pop %rax;"
           "pop %rbp;"
           "mov %gs:48, %rsp;"
           "jmp *%gs:56;");
}

extern "C" void do_preempt() {
    libos_print("preempt function injected!");
    scheduler->schedule();
}
