.global _start
.extern main

_start:
mov %rsp, %r8
mov %rbp, %r9
mov %rsi, %rsp
mov %rsp, %rbp
push %r8   # this is the stack
push %r10  # this is the return address
push %r9   # this is rbp
call __libOS_start
pop %rbp
pop %rbx   # restore the return address
pop %rsp   # restore stack
mov %rax, %rsi # store the return value of __libos_start to rsi
mov $4, %rax
enclu
ud2
