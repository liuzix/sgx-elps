#include "mmap.h"
#include "panic.h"
#include <new>

PageManager *pageManager = nullptr;

void mmap_init(uint64_t heapBase, uint64_t heapSize) {
    pageManager = new ((void *)heapBase) PageManager(heapBase, heapSize);
}

void *libos_mmap(void *base, size_t len) {
    void *ret;

    len += 4095;
    if (!pageManager) {
        libos_panic("mmap uninitialized!");
        return (void *)-1;
    }

    pageManager->lock.lock();
    if (!base) {
        ret = pageManager->allocPages(len / 4096);
        pageManager->lock.unlock();
        return ret;
    }

    bool successful = pageManager->explicitPage(base, len);
    if (successful) {
        pageManager->lock.unlock();
        return base;
    }
    else {
        ret = pageManager->allocPages(len / 4096);
        pageManager->lock.unlock();
        return ret;
    }
}

void libos_munmap(void *base, size_t len) {
    libos_print("munmap: 0x%lx, len = %d", base, len);
    if (!pageManager) {
        libos_panic("mmap uninitialized!");
    }

    len = (len + 4095) & (~4095);

    pageManager->lock.lock();
    pageManager->freePages(base, len / 4096);
    pageManager->lock.unlock();
}


extern "C" void *mmap(void *addr, size_t length, int, int,
                  int fd, off_t) {

    if (fd >= 0) {
        libos_print("mmapping fd is not supported");
        __asm__("ud2");
    }
    return libos_mmap(addr, length);
}

extern "C" int munmap(void *addr, size_t length) {
    libos_munmap(addr, length);
    return 0;
}

PageManager::PageManager(uint64_t base, size_t length) {
    size_t headerLen = offsetof(PageManager, bitset);
    size_t bitsetByteLen = length / (4096 * 8);
    this->clock = 0;
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

    do {
        if (!this->getBit(this->clock))
            nConsec++;
        else
            nConsec = 0;

        if (nConsec == nPages) {
            ret =
                (char *)this->availableBase + (this->clock - nPages + 1) * 4096;

            if (!this->explicitPage(ret, nPages)) break;
            return ret;
        }

        this->clock++;
        if (this->clock > (availableLength / 4096)) {
            this->clock = 0;
            nConsec = 0;
        }
    } while (this->clock != start);

    libos_print("mmap failed, nPages = %d", nPages);
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
        (uint64_t)addr + nPages * 4096 > this->availableBase + this->availableLength) {
        libos_print("explicitPage out of bounds");
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

    libos_print("mmap: giving out page 0x%lx, len = 0x%lx", addr, nPages * 4096);
    return true;
}
