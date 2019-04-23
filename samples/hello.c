#include <unistd.h>
#include <sys/syscall.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
//__thread int test = 0;


int main () {
    int i = 0;
    int test  = 0;
//    int* p = NULL;

    for (; i <= 10000; i++)
        test += i;
/*    asm volatile ("mov $0x1234, %%rax\n\t"
                  "mov $0x5678, %%rbx\n\t"
                  "mov $0, %%rcx\n\t"
                  "mov (%%rcx), %%rdx"
            :::"rax", "rbx", "rcx", "rdx");
*/
    int fd;
    char buf[256] = "testtesttest";
    buf[255] = 0;

//    fd = open("../samples/read_dst.txt", O_RDWR);
//    write(fd, buf, 256);
//    close(fd);

    printf("--------------------try this\n---\n");
    printf("should print this\n");
    return test;
}
