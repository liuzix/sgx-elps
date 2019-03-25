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
.endm

.extern get_flag
.extern sig_exit
.global __eenter
.global __back
.global __aex_handler
.global __interrupt_back
.hidden __back
.hidden __aex_handler
__eenter:
EENTER_PROLOG
mov $0x2, %rax
mov %rdi, %rbx
mov $0, %rdi /* clean-up rdi*/
lea __aex_handler(%rip), %rcx
push %rbp
enclu
/* we have exited the enclave by now */
/* the enclave should store its return value in rsi,
    because rax needs to hold the leaf function number */
__back:
pop %rbp
EENTER_EPILOG
ret

__exit:
call sig_exit@plt

__go_dump:
mov $1, %rdi
mov %rbx, %rsi
mov $0x2, %rax
/* rbx should be ok */
lea __aex_handler(%rip), %rcx
/* nested eenter */
enclu
ud2

__go_interrupt:
push %rbx
mov $2, %rdi
mov %rbx, %rsi
mov $0x2, %rax
lea __aex_handler(%rip), %rcx
enclu
ud2

__aex_handler:
nop
push %rax
push %rbx
push %rcx

mov %rbx, %rdi
call do_aex
mov %rax, %r13
pop %rcx
pop %rbx
pop %rax

cmp $1, %r13
je __go_dump
cmp $2, %r13
je __go_interrupt

enclu
ud2
__interrupt_back:
pop %rbx 
push %rbx
mov %rbx, %rdi
call clear_interrupt@plt
mov $3, %rax
pop %rbx
lea __aex_handler(%rip), %rcx
enclu
ud2
