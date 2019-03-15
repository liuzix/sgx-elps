.macro EENTER_PROLOG
push    %rbp
mov     %rsp, %rbp
push    %rbx
push    %r12
push    %r13
push    %r14
push    %r15
push    %r8
push    %rcx
push    %rdx
push    %rsi
push    %rdi
.endm


.macro EENTER_EPILOG
mov     -8*1(%rbp),  %rbx
mov     -8*2(%rbp),  %r12
mov     -8*3(%rbp),  %r13
mov     -8*4(%rbp),  %r14
mov     -8*5(%rbp),  %r15
mov     %rbp, %rsp
pop     %rbp
ret
.endm

.global __eenter
.global __back
.hidden __back
__eenter:
EENTER_PROLOG
mov $0x2, %rax
mov %rdi, %rbx
lea __aex_handler(%rip), %rcx
enclu
/* we have exited the enclave by now */
/* the enclave should store its return value in rsi,
    because rax needs to hold the leaf function number */
__back:
EENTER_EPILOG
ret


__aex_handler:
/* just ERESUME for now */
nop
enclu
int3
