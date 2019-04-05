#include <hint.h>
#include <cstddef>
#include <stdio.h>
#include <stdlib.h>
#include "../libOS/mmap.h"
#include "../libOS/panic.h"

#define MEM_SIZE 0x8000000
#define STEP     0x800000

int main(int argc, char **argv)
{
    uint64_t jiffies = *pjiffies;
    void *addr = libos_mmap(NULL, MEM_SIZE);

    hint(addr);
    libos_print("hello: %ld, %ld\n", jiffies, *pjiffies);
    if (addr == (void *)-1) {
       return -1;
    }
/*
    for(size_t i = 0; i < MEM_SIZE; i += STEP)
    {
        hint((void *)((char *)addr + i));
        *((char *)addr + i) = 'e';

        return (int)(cc2 - cc1);
    }
    */
    return 0;
}

