#include "mmap.h"
#include <new>

PageManager *pageManager = nullptr;

void mmap_init(uint64_t heapBase, uint64_t heapSize) {
    pageManager = new ((void *)heapBase) PageManager(heapBase, heapSize);
}

PageManager::PageManager(uint64_t base, size_t length) {
    size_t headerLen = offsetof(PageManager, bitset);
    size_t bitsetByteLen = length / (4096 * 8);
    this->availableBase = base + headerLen + bitsetByteLen;
    this->availableBase = (this->availableBase + 4095) & (~4095);
    this->availableLength = base + length - this->availableBase;
    this->availableLength &= ~4095;
    
    for (size_t i = 0; i < bitsetByteLen; i++)
        bitset[i] = 0;
}

bool PageManager::getBit(size_t index) {
    return (this->bitset[index / 8] >> (index % 8)) & 1;
}

void PageManager::setBit(size_t index, bool t) {
    uint8_t temp = bitset[index / 8];
    if (t)
        bitset[index / 8] = temp | (1 << (index % 8));
    else
        bitset[index / 8] = temp & ~(1 << (index % 8));
}

void *PageManager::allocPages(int nPages) {
    int nConsec = 0;
    size_t start = this->clock;
    char *ret = nullptr;

    if (nPages <= 0)
        return nullptr;

    while (this->clock != start) {
        if (this->getBit(this->clock))
            nConsec++;
        else
            nConsec = 0;

        if (nConsec == nPages) {
            ret =
                (char *)this->availableBase + (this->clock - nPages + 1) * 4096;

            this->explicitPage(ret, nPages);
            return ret;
        }

        this->clock++;
        if (this->clock > (availableLength / 4096)) {
            this->clock = 0;
            nConsec = 0;
        }
    }

    return (void *)-1;
}

void PageManager::freePages(void *addr, int nPages) {
    size_t start = ((uint64_t)addr - this->availableBase) / 4096;
    for (int i = 0; i < nPages; i++) {
        this->setBit(start + i, false);
    }
}

bool PageManager::explicitPage(void *addr, int nPages) {
    if ((uint64_t)addr < this->availableBase ||
        (uint64_t)addr > this->availableBase + this->availableLength) {

        return false;
    }

    size_t start = ((uint64_t)addr - this->availableBase) / 4096;
    for (int i = 0; i < nPages; i++) {
        if (this->getBit(start + i))
            return false;
    }

    for (int i = 0; i < nPages; i++) {
        this->setBit(start + i, true);
    }

    return true;
}
