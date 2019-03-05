
int main () {
    int i = 0;
    int sum = 0;

    for (; i <= 100; i++)
        sum += i;

    return sum;
}

asm(".global _start\n"
    "_start:\n"
    "mov %rsp, %r8;"
    "mov %rbp, %r9;"
    "mov %rsi, %rsp;"
    "mov %rsp, %rbp;"
    "push %r8;"   // this is the stack
    "push %rdx;"  // this is the return address
    "push %r9;"    // this is rbp
    "call main;"
    "pop %rbp;"
    "pop %rbx;"   // restore the return address
    "pop %rsp;"   // restore stack
    "mov %rax, %rsi;"
    "mov $4, %rax;"
    "enclu;");

