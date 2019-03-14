#include "allocator.h"
#include <cmath>

/* comment out stuff that doesn't compile */
/*
Allocator::Allocator(size_t len, vaddr heapBase) {
    int ind = (int)log2(len);
    MemoryArea ima;
    ima.len = len;
    ima.heapBase = heapBase;
    chunkList[ind].push_back(ima);
    root.push_back(ima);
}

void *Allocator::la_alloc(size_t len) {
    //TODO
    //find an appropriate size
    //then split
}

bool *la_free(vaddr baseAddr) {
    //TODO
    //free an area
    //then merge adjancent area
}
*/
