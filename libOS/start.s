.macro push_vec
    mov $0, %rax
    1:
        cmp %rax, %rcx
        je 2f
        add $1, %rax
        add $8, %rbx
        jmp 1b
    2:
        sub $8, %rbx
        mov $0, %rax
    3:
        cmp %rax, %rcx
        je 4f
        push (%rbx)
        add $1, %rax
        sub $8, %rbx
        jmp 3b
    4:
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
mov %rsi, %rdi
call do_interrupt

_start:
# begin switching stack
mov %gs:32, %r14            # get libos_data 

# temporarily comment out the preempt
cmp $2, %rdi
jz __asm_do_interrupt

mov %rsp, 8(%r14)           # save original rsp
mov 16(%r14), %rsp          # swtich to enclave stack 
# end switching stack 

cmp $1, %rdi
jz __asm_dump_ssa

mov %rsp, %rbp
mov 40(%r14), %rdi

cmp $0, 112(%r14)
je __slave_thread

mov $80, %rcx               # temporarily hardcoded
mov 32(%rdi), %rbx
__push_aux:
push_vec

xor %rcx, %rcx
mov 16(%rdi), %ecx          # get envc
add $1, %ecx
mov 24(%rdi), %rbx
__push_env:
push_vec

xor %rcx, %rcx
mov (%rdi), %ecx            # get argc
add $1, %ecx
mov 8(%rdi), %rbx           # argv
__push_argv:
push_vec
mov %rsp, %rsi
__slave_thread:
call __libOS_start
ud2
__eexit:
# begin switching stack
mov %gs:32, %r14            # get libos_data 
mov %rsp, 16(%r14)          # save original rsp
mov 8(%r14), %rsp           # swtich to normal stack 
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
