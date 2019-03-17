#include <unistd.h>
#include <sys/syscall.h>
__thread int test = 0;


int main () {
    int i = 0;
    int test  = 0;

    for (; i <= 10000; i++)
        test += i;
    
    syscall(0);
    return test;
}

