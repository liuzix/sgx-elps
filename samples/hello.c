#include <unistd.h>
#include <sys/syscall.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
__thread int test = 0;


int main () {
    int i = 0;
    int test  = 0;
    int* p = NULL;
    test++;
    for (; i <= 10000; i++)
        test += i;
/*    asm volatile ("mov $0x1234, %%rax\n\t"
                  "mov $0x5678, %%rbx\n\t"
                  "mov $0, %%rcx\n\t"
                  "mov (%%rcx), %%rdx"
            :::"rax", "rbx", "rcx", "rdx");
*/
    int fd;
//    fd = syscall(2,"/home/kaige/sgx-elps/samples/read_dst.txt", O_RDWR);
//    char buf[10] = "----------";
    char buf[256];
//    syscall(0, fd, buf, 256);
//    syscall(1, fd, buf, 10);
//    syscall(3, fd);

    fd = open("../samples/read_dst.txt", O_RDWR);
    read(fd, buf, 256);
    close(fd);


//    printf("try this\n");
    return test;
}

