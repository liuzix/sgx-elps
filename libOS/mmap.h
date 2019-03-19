#ifndef MMAP_H
#define MMAP_H

#include <stddef.h>
#include <stdint.h>

class PageManager {
  private:
    uint64_t availableBase;
    size_t availableLength;
    size_t clock = 0;
    uint8_t bitset[];

    bool getBit(size_t index);
    void setBit(size_t index, bool t);

  public:
    PageManager(uint64_t base, size_t length);

    void *allocPages(int nPages);
    void freePages(void *addr, int nPages);
    bool explicitPage(void *addr, int nPages);
};

void mmap_init(uint64_t heapBase, uint64_t heapSize);

void *libos_mmap(void *base, size_t len);

void libos_munmap(void *base, size_t len);

#endif
