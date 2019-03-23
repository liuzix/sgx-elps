.macro push_vec
    mov $0, %rax
    1:
        cmp %rax, n
        je 2
        push (%rbx)
        add $1, %rax
        add $8, %rbx
        jmp 1
    2:
    .endm

.global _start
.global __interrupt_exit
.global __eexit
.extern main
.extern dump_ssa

__asm_dump_ssa:
mov %rsi, %rdi
call dump_ssa

__asm_do_interrupt:
mov %rsp, 80(%r14)           # save original rsp
mov 48(%r14), %rsp
call do_interrupt

_start:
# begin switching stack
mov %gs:32, %r14            # get libos_data 

cmp $2, %rdi
jz __asm_do_interrupt

mov %rsp, 8(%r14)           # save original rsp
mov 16(%r14), %rsp          # swtich to enclave stack 
# end switching stack 

cmp $1, %rdi
jz __asm_dump_ssa

mov %rsp, %rbp
mov 40(%r14), %rdi

lea [rdi + 8], %rbx         # get argv address
mov (%rbx), %rbx            # load argv
xor %rcx, %rcx
mov (%rdi), %ecx            # get argc
push_vec

lea [rdi + 16], %rbx
xor %rcx, %rcx
mov (%rbx), %ecx            # get envc
lea [rdi + 24], %rbx
mov (%rbx), %rbx            # load env
push_vec

lea [rdi + 32], %rbx
mov (%rbx), %rbx
mov $38, %rcx               # temporarily hardcoded
push_vec

call __libOS_start
ud2
__eexit:
# begin switching stack
mov %gs:32, %r14            # get libos_data 
mov %rsp, 16(%r14)           # save original rsp
mov 8(%r14), %rsp          # swtich to normal stack 
# end switching stack

mov (%r14), %rbx
mov %rdi, 32(%r14) 
mov $4, %rax
enclu
ud2

__interrupt_exit:
mov %gs:32, %r14
mov 80(%r14), %rsp
mov 72(%r14), %rbx
mov $4, %rax
enclu
ud2
