#include "panic.h"
#include "mmap.h"
#include <string.h>
#include "allocator.h"
#include <vector>
#include <list>

void testUnsafeMalloc() {
    libos_print("starting unsafe malloc test");
    
    std::vector<int, UnsafeAllocator<int>> vec;
    for (int i = 0; i < 10000000; i++)
        vec.push_back(i);

    for (int i = 0; i < 10000000; i++) {
        if (vec[i] != i) __asm__("ud2");
        vec.pop_back();
    }

    std::list<int, UnsafeAllocator<int>> li;
    for (int i = 0; i < 1000000; i++)
        li.push_back(i);

    auto it = li.begin();
    for (int i = 0; i < 1000000; i++)
        if (*(it++) != i) __asm__("ud2");
    
    libos_print("unsafe malloc test passed");
}

void testSafeMalloc() {
    libos_print("starting safe malloc test");
    
    std::vector<int> vec;
    for (int i = 0; i < 10000000; i++)
        vec.push_back(i);

    for (int i = 0; i < 10000000; i++) {
        if (vec[i] != i) __asm__("ud2");
        vec.pop_back();
    }

    std::list<int> li;
    for (int i = 0; i < 1000000; i++)
        li.push_back(i);

    auto it = li.begin();
    for (int i = 0; i < 1000000; i++)
        if (*(it++) != i) __asm__("ud2");
    
    libos_print("safe malloc test passed");
}

void testMmap() {
    libos_print("mmap tests start");
    std::vector<unsigned char *, UnsafeAllocator<unsigned char *>> vec;
    for (int i = 0; i < 20; i++) {
        void *pages = libos_mmap(nullptr, 4096 * (i % 20)); 
        if (pages == (void *)-1) {
            for (;;) {}
        }
        memset(pages, i % 256, 4096 * (i % 20));
        vec.push_back((unsigned char *)pages);
    }

    for (int i = 0; i < 20; i++) {
        for (int j = 0; j < 4096 * (i % 20); j++) {
            if (vec[i][j] != i % 256) {
                libos_print("assertion failed, i = %d", i);
                for (;;) {}
            }
        }
        libos_munmap(vec[i], 4096 * (i % 20));
    }
    libos_print("mmap tests passed");
}

