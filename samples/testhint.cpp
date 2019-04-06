#include <hint.h>
#include <cstddef>
#include <stdio.h>
#include <stdlib.h>
#include "../libOS/mmap.h"
#include "../libOS/panic.h"

#define MEM_SIZE 0x1000000
#define STEP     0x1000

int main(int argc, char **argv)
{
    void *addr = libos_mmap(NULL, MEM_SIZE);
    int j = 0;

    if (addr == (void *)-1) {
       return -1;
    }

    for(size_t i = 0; i < MEM_SIZE; i += STEP)
    {
        //hint((void *)((char *)addr + i));
        uint64_t jiffies = *pjiffies;
        *((char *)addr + i) = 'e';
        j++;
        //libos_print("jiffies: %ld", *pjiffies - jiffies);
    }

    libos_print("total access: %d", j);
    return 0;
}

