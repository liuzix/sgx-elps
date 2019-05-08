#ifndef MMAP_H
#define MMAP_H

#include <spin_lock.h>
#include <stddef.h>
#include <stdint.h>
#include <bitmap.h>
#include <libOS_tls.h>
#include <immintrin.h>

#define LOG_1(n) (((n) >= 2) ? 1 : 0)
#define LOG_2(n) (((n) >= 1<<2) ? (2 + LOG_1((n)>>2)) : LOG_1(n))
#define LOG_4(n) (((n) >= 1<<4) ? (4 + LOG_2((n)>>4)) : LOG_2(n))
#define LOG_8(n) (((n) >= 1<<8) ? (8 + LOG_4((n)>>8)) : LOG_4(n))
#define LOG(n)   (((n) >= 1<<16) ? (16 + LOG_8((n)>>16)) : LOG_8(n))

#define BUDDY_THRESH        0x200000
#define BUDDY_THRESH_PAGE   (BUDDY_THRESH / PAGE_SIZE)
#define NR_BITMAP LOG((BUDDY_THRESH / PAGE_SIZE))

class PageManager {
public:
    SpinLock lock;
private:
    uint64_t availableBase;
    size_t availableLength;
    size_t clock = 0;
    Bitmap<SAFE_HEAP_LEN / PAGE_SIZE, 1> bitset[NR_BITMAP];

    bool getBit(int order, size_t index);
    void setBit(size_t index, bool t);
    void *buddyAllocPages(int nPages);
    int *splitAndGetBits(int from, int to);
    bool buddyExplicitPage(void *addr, int nPages);

public:
    PageManager(uint64_t base, size_t length);

    void *allocPages(int nPages);
    void freePages(void *addr, int nPages);
    bool explicitPage(void *addr, int nPages);
    const uint64_t nr_bitmap = NR_BITMAP;
};

void mmap_init(uint64_t heapBase, uint64_t heapSize);

void *libos_mmap(void *base, size_t len);

void libos_munmap(void *base, size_t len);

#endif
