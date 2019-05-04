#include <new>
#include "allocator.h"
#include "panic.h"
#include "mmap.h"
#include "string.h"

#define EXPAND_FACTOR 10
extern "C" void *malloc(size_t len) {
   bool check = safeAllocator->checkSumAll();
   if (!check) __asm__("ud2");
   void *ret = safeAllocator->malloc(len);
   libos_print("malloc: len = %d, first try 0x%lx", len, ret);
   if (ret) return ret; 
   
   size_t expandBy = ((len + 4095) & (~4095)) * 10;
   libos_print("malloc: expand heap by 0x%lx", expandBy);
   void *newPages = libos_mmap(nullptr, expandBy);
   if (newPages == (void *)-1) {
       libos_print("malloc failed! len = %d", len);
       return nullptr;
   }

   safeAllocator->expandHeap(newPages, expandBy);
   ret = safeAllocator->malloc(len);
   libos_print("malloc: len = %d, second try 0x%lx", len, ret);

   return ret;
}


extern "C" void free(void *ptr) {
    safeAllocator->free((vaddr)ptr); 
}

extern "C" void *realloc(void *ptr, size_t newSize) {
    libos_print("realloc!");
    void *newChunk = malloc(newSize);
    memcpy(newChunk, ptr, safeAllocator->getLen(ptr)); 
    return newChunk;
}

extern "C" void *calloc(size_t m, size_t n) {
    void *ret = malloc(m * n);
    if (!ret) return ret;

    memset(ret, 0, m * n);
    return ret;
}
