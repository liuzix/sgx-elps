#include <hint.h>
#include <cstddef>
#include <stdio.h>
#include <stdlib.h>
#include "../libOS/mmap.h"

#define MEM_SIZE 0x8000000
#define STEP     0x800000

int main(int argc, char **argv)
{
    void *addr = libos_mmap(NULL, MEM_SIZE);
    int cc1, cc2;
    // void *addr = malloc(0x8000000);
    if (addr == (void *)-1) {
       return -1;
    }
    
    for(size_t i = 0; i < MEM_SIZE; i += STEP)
    {
        //cc1 = get_cycle();
        
        hint((void *)((char *)addr + i));
       
        //cc2 = get_cycle();
        *((char *)addr + i) = 'e';

        return (int)(cc2 - cc1);
    }
    return 0;
}

//  .78   .74   .77
//80