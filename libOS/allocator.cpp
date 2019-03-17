#include "allocator.h"
#include <new>
#include <cmath>
#include <iterator>
#define MA_SIZE sizeof(MemoryArea)
#define MIN_PRESERVE_SIZE 2 * MA_SIZE

MemoryArea::MemoryArea(size_t l, vaddr vm) {
    len = l;
    memBase = vm;
}

//static bool MemoryArea::operator<(MemoryArea &ma1, MemoryArea &ma2) {
//    return ma1.memBase < ma2.memBase;
//}

Allocator::Allocator(size_t len, vaddr heapBase) {
    MemoryArea *ma = (MemoryArea *) new ((void *)heapBase) MemoryArea(len - MA_SIZE, heapBase);
    rbtreeLock.lock();
    root.insert_equal(*ma);
    rbtreeLock.unlock();
    listLock.lock();
    chunkList[CHUNK_LIST_SIZE - 1].push_back(*ma);
    listLock.unlock();
}

void *Allocator::malloc(size_t len) {
    if (len == 0)
        return (void *)0;
    for (int i = 0; i < CHUNK_LIST_SIZE; i++) {
        if (len >= pow(2, i + 2))
            continue;
        listLock.lock();
        MemberList::iterator mit(chunkList[i].begin());
        for (; mit != chunkList[i].end(); mit++) {
            if((*mit).len > len) {
                vaddr retAddr = (vaddr)&(*mit) + MA_SIZE;
                size_t oldLen = (*mit).len;
                MemberRbtree::iterator mitRbt = root.iterator_to(*mit);
                rbtreeLock.lock();
                root.erase(mitRbt);
                rbtreeLock.unlock();
                chunkList[i].erase(mit);
                if (oldLen - len >= MIN_PRESERVE_SIZE) {
                    vaddr curBase = retAddr + len;
                    MemoryArea *ma = (MemoryArea *) new ((void *)curBase) MemoryArea(oldLen - len - MA_SIZE, curBase);
                    rbtreeLock.lock();
                    root.insert_equal(*ma);
                    rbtreeLock.unlock();
                    if (len == 1)
                        chunkList[0].push_back(*ma);
                    else
                        chunkList[(int)log2(oldLen - len - MA_SIZE) - 1].push_back(*ma);
                }
                listLock.unlock();
                return (void *)retAddr;
            }
        }
        listLock.unlock();
    }
    return nullptr;
}

void Allocator::free(vaddr baseAddr) {
    MemoryArea *ma = (MemoryArea *)(baseAddr + MA_SIZE);
    rbtreeLock.lock();
    root.insert_equal(*ma);
    rbtreeLock.unlock();
    listLock.lock();
    rbtreeLock.lock();
    MemberRbtree::iterator mit = root.iterator_to(*ma),
                           mitNext = std::next(mit), mitPrev = std::prev(mit);
    if (mitNext != root.end()) {
        if (&(*mit) + MA_SIZE + (*mit).len == &(*mitNext)) {
            MemberList::iterator mitLst = chunkList[(int)log2((*mitNext).len) - 1].iterator_to(*mitNext);
            chunkList[(int)log2((*mitLst).len) - 1].erase(mitLst);
            (*mit).len += MA_SIZE + (*mitNext).len;
            root.erase(mitNext);
        }
    }
    if (mit != root.begin()) {
        if (&(*mitPrev) + MA_SIZE + (*mitPrev).len == &(*mit)) {
            MemberList::iterator mitLst = chunkList[(int)log2((*mitPrev).len) - 1].iterator_to(*mitPrev);
            chunkList[(int)log2((*mitLst).len) - 1].erase(mitLst);
            (*mitPrev).len += MA_SIZE + (*mit).len;
            root.erase(mit);
            chunkList[(int)log2((*mitPrev).len) - 1].push_back(*mitPrev);
            rbtreeLock.unlock();
            listLock.unlock();
            return;
        }
    }
    chunkList[(int)log2((*mit).len) - 1].push_back(*mit);
    rbtreeLock.unlock();
    listLock.unlock();
    return;
}

