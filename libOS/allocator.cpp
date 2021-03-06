#include "allocator.h"
#include "singleton.h"
#include <unordered_map>
#include "panic.h"
#include "mmap.h"
#include "user_thread.h"
#include "sched.h"
#include <iostream>
#include <new>
#include "request.h"
#include <cmath>
#include <iterator>
#define MA_SIZE sizeof(MemoryArea)
#define MIN_PRESERVE_SIZE 2 * MA_SIZE
#define ALLOCATOR_DEBUG
#undef ALLOCATOR_DEBUG

#ifdef IS_LIBOS
Allocator *unsafeAllocator;
Allocator *safeAllocator;

extern "C" void malloc_check() {
    safeAllocator->checkWholeTree();
}

void initSafeMalloc(size_t len) {
    size_t allocatorSize = sizeof(Allocator);
    void *base = libos_mmap(nullptr, len);
    if (base == (void *)-1) {
        libos_panic("safe malloc: init mmap failed.");
    }
    safeAllocator = new (base) Allocator (len - allocatorSize, (vaddr)base + allocatorSize);
}

void initUnsafeMalloc(void *base, size_t len) {
    size_t allocatorSize = sizeof(Allocator);
    unsafeAllocator = new (base) Allocator (len - allocatorSize, (vaddr)base + allocatorSize);
}

void *unsafeMalloc(size_t len) {
    return unsafeAllocator->malloc(len);
}

void unsafeFree(void *ptr) {
    unsafeAllocator->free((vaddr)ptr);
}


#endif

static inline int myLog2(size_t x) {
    return sizeof(uint32_t) * CHAR_BIT - __builtin_clz(x) - 1;
}

static inline size_t sizeToBucket(size_t len) {
    int n = myLog2(len);
    if (n > 0) return n - 1;
    else return n;
}

MemoryArea::MemoryArea(size_t l, vaddr vm) {
    len = l;
    memBase = vm;
    free = true;
}

Allocator::Allocator(size_t len, vaddr heapBase) {
    this->expandHeap((void *)heapBase, len);
}

void Allocator::expandHeap(void *base, size_t len) {
    MemoryArea *ma = (MemoryArea *) new (base) MemoryArea(len - MA_SIZE, (vaddr)base);
    listLock.lock();
    rbtreeLock.lock();
#ifdef ALLOCATOR_DEBUG
    ma->setChecksum();
#endif
    chunkList[CHUNK_LIST_SIZE - 1].push_back(*ma);
    root.insert_equal(*ma);
    rbtreeLock.unlock();
    listLock.unlock();
}
void Allocator::dump() {
    for (int i = 0; i < CHUNK_LIST_SIZE; i++) {
        std::cout << "bucket " << i << " has " << this->chunkList[i].size() << " free chunks" << std::endl;
    }
    std::cout << "--------------" << std::endl;
}

void *Allocator::malloc(size_t len) {
    if (len == 0)
        return (void *)0;
#ifdef ALLOCATOR_DEBUG
    rbtreeLock.lock();
    this->checkWholeTree();
    rbtreeLock.unlock();
#endif
    listLock.lock();
    for (int i = 0; i < CHUNK_LIST_SIZE; i++) {
        if (len >= pow(2, i + 2))
            continue;
        MemberList::iterator mit(chunkList[i].begin());
        for (; mit != chunkList[i].end(); mit++) {
            if (mit->len < len) continue;

            mit->free = false;
            vaddr retAddr = (vaddr)&(*mit) + MA_SIZE;
            size_t oldLen = mit->len;
            mit->len = len;
            rbtreeLock.lock();
            MemberRbtree::iterator mitRbt = root.iterator_to(*mit);
            root.erase(mitRbt);
            rbtreeLock.unlock();
            chunkList[i].erase(mit);
            if (oldLen - len >= MIN_PRESERVE_SIZE) {
                vaddr curBase = retAddr + len;
                MemoryArea *ma = (MemoryArea *) new ((void *)curBase) MemoryArea(oldLen - len - MA_SIZE, curBase);
                rbtreeLock.lock();
                root.insert_equal(*ma);
                rbtreeLock.unlock();
                if (ma->len == 1)
                    chunkList[0].push_back(*ma);
                else
                    chunkList[sizeToBucket(oldLen - len - MA_SIZE)].push_back(*ma);
            }
            listLock.unlock();
#ifdef ALLOCATOR_DEBUG
            mit->setChecksum();
#endif
            return (void *)retAddr;
        }
    }


#ifdef ALLOCATOR_DEBUG
#ifdef IS_LIBOS
    //libos_print("cannot find chunck of size %d", len);
#else
    std::cout << "cannot find chunk of size " << len << std::endl;
    this->dump();
#endif
#endif
    listLock.unlock();
    return nullptr;
}

void *Allocator::memAlign(size_t alignment, size_t size) {
    if (size == 0)
        return (void *)0;
#ifdef ALLOCATOR_DEBUG
    rbtreeLock.lock();
    this->checkWholeTree();
    rbtreeLock.unlock();
#endif
    size_t len = size + MIN_PRESERVE_SIZE + alignment;
    void *oldAddr = malloc(len);
    void *newAddr = oldAddr;
    size_t oldLen, newTmp;
    MemoryArea *maOld = (MemoryArea *)((size_t)oldAddr - MA_SIZE);
    maOld->free = true;
    newTmp = (size_t)newAddr;
    newTmp += MIN_PRESERVE_SIZE;
    while (1) {
        if ((unsigned long)newTmp % (unsigned long)alignment != 0)
            newTmp++;
        else
            break;
    }
    newAddr = (void *)newTmp;
    /*
    MemoryArea *ma = (MemoryArea *) new ((void *)((size_t)newAddr - MA_SIZE))
        MemoryArea(len + (size_t)oldAddr - (size_t)newAddr, (size_t)newAddr - MA_SIZE);
    */
    oldLen = (size_t)newAddr - MA_SIZE - (size_t)oldAddr;
    maOld->len = oldLen;
    listLock.lock();
    rbtreeLock.lock();
    root.insert_equal(*maOld);
    rbtreeLock.unlock();
    if (oldLen == 1)
        chunkList[0].push_back(*maOld);
    else
        chunkList[sizeToBucket(oldLen)].push_back(*maOld);
    listLock.unlock();
#ifdef ALLOCATOR_DEBUG
    ma->setChecksum();
#endif
    return newAddr;
}

void Allocator::free(vaddr baseAddr) {
    if (baseAddr == 0)
        return;
    MemoryArea *ma = (MemoryArea *)(baseAddr -  MA_SIZE);
    listLock.lock();
    rbtreeLock.lock();
#ifdef ALLOCATOR_DEBUG
    this->checkWholeTree();
    if (ma->canary != 0xbeefbeef) __asm__("ud2");
    ma->checkIntegrity();
    // add poison
    memset((void *)baseAddr, 0xCC, ma->len);
#endif
    root.insert_equal(*ma);
    MemberRbtree::iterator mit = root.iterator_to(*ma),
                           mitNext = std::next(mit), mitPrev = std::prev(mit);
    if (mitNext != root.end()) {
        assert(mitNext->free);
        if ((uint64_t)&(*mit) + MA_SIZE + mit->len == (uint64_t)&(*mitNext)) {
            MemberList::iterator mitLst = chunkList[sizeToBucket((*mitNext).len)].iterator_to(*mitNext);
            chunkList[sizeToBucket((*mitLst).len)].erase(mitLst);
            (*mit).len += MA_SIZE + (*mitNext).len;
            root.erase(mitNext);
        }
    }
    this->checkWholeTree();
    if (mit != root.begin()) {
        assert(mitPrev->free);
        if ((uint64_t)&(*mitPrev) + MA_SIZE + (*mitPrev).len == (uint64_t)&(*mit)) {
            MemberList::iterator mitLst = chunkList[sizeToBucket((*mitPrev).len)].iterator_to(*mitPrev);
            chunkList[sizeToBucket((*mitLst).len)].erase(mitLst);
            mitPrev->len += MA_SIZE + mit->len;
            root.erase(mit);
            chunkList[sizeToBucket((*mitPrev).len)].push_back(*mitPrev);
            rbtreeLock.unlock();
            listLock.unlock();
            return;
        }
    }
    chunkList[sizeToBucket((*mit).len)].push_back(*mit);
    ma->free = true;
    this->checkWholeTree();
    rbtreeLock.unlock();
    listLock.unlock();
    return;
}

size_t Allocator::getLen(void *ptr) {
    if (ptr == nullptr) return 0;
    MemoryArea *ma = (MemoryArea *)((char *)ptr -  MA_SIZE);
    return ma->len;
}

/* Warning: this function will hurt performance */
void Allocator::checkWholeTree() {
    for (auto &ma: this->root) {
        if (ma.canary != 0xbeefbeef) {
            __asm__("ud2");
        }
    }
}

bool Allocator::checkSumAll() {
    bool ret = true;
    for (int i = 0; i < CHUNK_LIST_SIZE; i++) {
        MemberList::iterator mit(chunkList[i].begin());
        for (; mit != chunkList[i].end(); mit++)
            if (mit->magic != 0xdeadbeef) {
                libos_print("chunck list[%d] corrupted.", i);
                ret = false;
            }
    }
    return ret;
}

template<typename ReqT>
std::unordered_map<int, ReqT*>* Singleton<ReqT>::umap;

template<typename ReqT>
SpinLock* Singleton<ReqT>::mapLock;

template class Singleton<SwapRequest>;
template class Singleton<DebugRequest>;
template class Singleton<SyscallRequest>;
template class Singleton<SchedulerRequest>;
template class Singleton<SleepRequest>;
