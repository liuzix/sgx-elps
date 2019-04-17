#include <hint.h>
#include <cstddef>
#include <stdio.h>
#include <stdlib.h>
#include "../libOS/mmap.h"
#include "../libOS/panic.h"

#define MEM_SIZE 0x1000000
#define STEP     0x10000

int main(int argc, char **argv)
{
    void *addr = libos_mmap(NULL, MEM_SIZE);
    int j = 0;

    libos_print("application addr: 0x%lx", (uint64_t)addr);
    if (addr == (void *)-1) {
       return -1;
    }
    //j = *(int *)(0x0);
    for(size_t i = 0; i < MEM_SIZE; i += STEP)
    {
        //uint64_t jiffies = *pjiffies;
        hint((void *)((char *)addr + i));
        *((char *)addr + i) = 'e';
        //libos_print("application addr: %ld", (uint64_t)((char *)addr + i));
        //libos_print("jiffies: %ld", *pjiffies - jiffies);
        j++;
    }

    libos_print("total access: %d", j);
    return 0;
}

