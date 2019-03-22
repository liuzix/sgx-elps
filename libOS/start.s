.global _start
.global __eexit
.extern main
.extern dump_ssa

__asm_dump_ssa:
mov %rsi, %rdi
call dump_ssa

_start:
# begin switching stack
mov %gs:32, %r14            # get libos_data 
mov %rsp, 8(%r14)           # save original rsp
mov 16(%r14), %rsp          # swtich to enclave stack 
# end switching stack 
cmp $1, %rdi
jz __asm_dump_ssa

mov %rsp, %rbp
mov 40(%r14), %rdi
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

