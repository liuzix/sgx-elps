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

xor %rcx, %rcx
mov (%rdi), %ecx            # get argc
mov 8(%rdi), %rbx        # get argv value
push_vec

xor %rcx, %rcx
mov 16(%rdi), %rcx       # get envc
mov 24(%rdi), %rbx        # get envv value
push_vec

mov $38, %rcx               # temporarily hardcoded
mov 32(%rdi), %rbx
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
