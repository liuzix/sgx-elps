#include "allocator.h"
#include <new>
#include <cmath>
#define MA_SIZE sizeof(MemoryArea)
#define MIN_PRESERVE_SIZE 2 * MA_SIZE

MemoryArea::MemoryArea(size_t l) {
    len = l;
}

Allocator::Allocator(size_t len, vaddr heapBase) {
    heapStart = heapBase;
    MemoryArea *ma = (MemoryArea *) new ((void *)heapBase) MemoryArea(len - MA_SIZE);
    //root.push_back(*ma);
    chunkList[CHUNK_LIST_SIZE - 1].push_back(*ma);
}

void *Allocator::malloc(size_t len) {
    for (int i = 0; i < CHUNK_LIST_SIZE; i++) {
        if (len >= pow(2, i + 2))
            continue;
        MemberList::iterator mit(chunkList[i].begin());
        for (; mit != chunkList[i].end(); mit++) {
            if((*mit).len > len) {
                vaddr retAddr = (vaddr)&(*mit) + MA_SIZE;
                size_t oldLen = (*mit).len;
                chunkList[i].erase(mit);
                if (oldLen - len >= MIN_PRESERVE_SIZE) {
                    vaddr curBase = retAddr + len;
                    MemoryArea *ma = (MemoryArea *) new ((void *)curBase) MemoryArea(oldLen - len - MA_SIZE);
                    chunkList[i].push_back(*ma);
                }
                return (void *)retAddr;
            }
        }
    }
    return nullptr;
}

void Allocator::free(vaddr baseAddr) {
    //TODO
    //free an area
    //then merge adjancent area
}
