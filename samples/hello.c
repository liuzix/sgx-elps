#include <unistd.h>
#include <sys/syscall.h>
__thread int test = 0;


int main () {
    int i = 0;
    int test  = 0;
    int* p = NULL;

    for (; i <= 10000; i++)
        test += i;
    asm volatile ("mov $0x1234, %%rax\n\t"
                  "mov $0x5678, %%rbx\n\t"
                  "mov $0, %%rcx\n\t"
                  "mov (%%rcx), %%rdx"
            :::"rax", "rbx", "rcx", "rdx");

    return test;
}

