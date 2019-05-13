#include "mmap.h"
#include "panic.h"
#include "allocator.h"
#include <new>

PageManager *pageManager = nullptr;

void mmap_init(uint64_t heapBase, uint64_t heapSize) {
    pageManager = new ((void *)heapBase) PageManager(heapBase, heapSize);
    libos_print("mmap_init successful.");
}

void *libos_mmap(void *base, size_t len) {
    void *ret;

    len += 4095;
    if (!pageManager) {
        libos_panic("mmap uninitialized!");
        return (void *)-1;
    }

    //pageManager->lock.lock();
    if (!base) {
        ret = pageManager->allocPages(len / PAGE_SIZE);
        //pageManager->lock.unlock();
        libos_print("no base mmap addr: 0x%lx to 0x%lx", (uint64_t)ret, (uint64_t)ret + len * (len / PAGE_SIZE));
        return ret;
    }

    bool successful = pageManager->explicitPage(base, len);
    if (successful) {
        //pageManager->lock.unlock();
        libos_print("mmap addr: 0x%lx to 0x%lx", (uint64_t)base, (uint64_t)base + len * (len / PAGE_SIZE));
        return base;
    }
    else {
        ret = pageManager->allocPages(len / PAGE_SIZE);
        //pageManager->lock.unlock();
        libos_print("mmap addr: 0x%lx to 0x%lx", (uint64_t)ret, (uint64_t)ret + len * (len / PAGE_SIZE));
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


extern "C" bool chunkCheckSum(void) {
    if (safeAllocator)
        return safeAllocator->checkSumAll();
    return true;
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
    this->clock = 0;
    this->availableBase = base + headerLen + sizeof(bitset);
    this->availableBase = (this->availableBase + 4095) & (~4095);
    this->availableLength = base + length - this->availableBase;
    this->availableLength &= ~4095;

    bitset[NR_BITMAP - 1].unset(0, (SAFE_HEAP_LEN / BUDDY_THRESH));
    for (int i = 0; i < SAFE_HEAP_LEN / PAGE_SIZE; i++) {
        if ((bitset[NR_BITMAP - 1].getWord(i)) != 0) {
            libos_print("0x%lx, i = %d", (bitset[NR_BITMAP - 1].getWord(i)), i);
            break;
        }
    }
    libos_print("PageManger initialized. availableBase: 0x%lx", availableBase);
}

/*
 * Split one chunk from the `from` and
 * set the bitmaps all the way down to the `to`.
 *
 * No need to use lock since the only bit we need is always 1
 */
int PageManager::splitAndGetBits(int from, int to) {
    int res = bitset[from].scan_and_set();
    if (res == -1) {
        //libos_print("%s: not found from: %d", __func__, from);
        return res;
    }

    //libos_print("%s: set bit[%d]: %d", __func__, from, res);
    for (int i = from - 1; i >= to; i--) {
        //libos_print("%s: unset i[%d]: %d", __func__, i, (res << (from - i)) + 1);
        bitset[i].unset((res << (from - i)) + 1);
    }
    return (res << (from - to));
}

/* Must be called within lock */
void PageManager::mergeBits() {
    for (int i = 0; i < (int)(nr_bitmap - 1); i++)
        for (int j = 0; j < ((SAFE_HEAP_LEN / PAGE_SIZE) >> i); j += 2) {
            if (bitset[i].set(j, 2, BIT_SET_FORCE) == -1)
                continue;
            bitset[i + 1].unset(j / 2);
        }
}

/* nPages must be 2^n aligned */
void *PageManager::buddyAllocPages(int nPages) {
    LIBOS_ASSERT(nPages <= BUDDY_THRESH_PAGE);
    int order = LOG(nPages);
    libos_print("request %d pages.", nPages);

    if ((1UL << order) < (uint64_t)nPages)
        order++;

    int res = bitset[order].scan_and_set();

    if (res != -1) {
        //libos_print("first: nPages:%d, bitset: %d, index: %d", nPages, order, res);
        return (char *)this->availableBase + res * PAGE_SIZE * (1UL << order);
    }
    int from = order + 1;

    while (from < NR_BITMAP) {
        res = splitAndGetBits(from, order);
        if (res == -1) {
            from++;
            continue;
        }

        //libos_print("second: nPages:%d, bitset: %d, index: %d", nPages, order, res);
        return (char *)this->availableBase + res * PAGE_SIZE * (1UL << order);
    }
    libos_print("mmap failed, nPages = %d", nPages);
    return (void *)-1;
}

/*
 * When size of allocation is less than 2MB, use buddy to allocate,
 * or will directly give out the lowest pages.
 */
void *PageManager::allocPages(int nPages) {
    if (nPages <= BUDDY_THRESH_PAGE)
        return buddyAllocPages(nPages);

    int res = bitset[nr_bitmap - 1].scan_and_set(((nPages * PAGE_SIZE - 1) / BUDDY_THRESH) + 1);

    if (res == -1) {
        libos_print("mmap failed, nPages = %d", nPages);
        return (void *)-1;
    }
    return (char *)this->availableBase + res * PAGE_SIZE * (1UL << (nr_bitmap - 1));
}

/* free un-allocated pages are undefined */
void PageManager::freePages(void *addr, int nPages) {
    this->tlock.lock();
    libos_print("free pages.");
    for (int order = nr_bitmap - 1; order >= 0 && nPages; order--) {
        uint64_t offset = (uint64_t)addr - availableBase;
        if (offset % (PAGE_SIZE * (1UL << order)) != 0)
            continue ;

        int n = nPages / (1UL << order);
        if (!n) continue;

        int i = offset / (PAGE_SIZE * (1UL << order));

        bitset[order].unset(i, n);
        nPages -= n * (1UL << order);
        addr = (void *)((char *)addr + (PAGE_SIZE * (n * (1UL << order))));
    }
    mergeBits();
    this->tlock.unlock();
}

/*
 * Explicitly give out nPages start from addr.
 * addr must be 2^n pages aligned
 */
bool PageManager::buddyExplicitPage(void *addr, int nPages) {
    LIBOS_ASSERT(nPages <= BUDDY_THRESH_PAGE);
    int order = LOG(nPages);

    if ((1UL << order) < (uint64_t)nPages)
        order++;

    uint64_t offset = (uint64_t)addr - availableBase;
    if (offset % (PAGE_SIZE * (1UL << order)) != 0)
        return false;

    int i = offset / (PAGE_SIZE * (1UL << order));
    int res = bitset[order].set(i);

    if (res != -1)
        return true;

    int from = order + 1, to = order;

    while (from < NR_BITMAP) {
        int j = ((uint64_t)addr - availableBase) / (PAGE_SIZE * (1UL << from));
        int res = bitset[from].set(j);

        if (res == -1) {
            from++;
            continue;
        }

        for (int i = from - 1; i >= to; i--)
            bitset[i].unset((res << (from - i)) + 1, 1);
        //bitset[to].unset(i + 1, (1UL << (from - to)) - 1);

        return true;
    }

    return false;
}

bool PageManager::explicitPage(void *addr, int nPages) {
    if ((uint64_t)addr < this->availableBase ||
        (uint64_t)addr + nPages * 4096 > this->availableBase + this->availableLength) {
        libos_print("explicitPage out of bounds");
        return false;
    }

    if (nPages <= BUDDY_THRESH_PAGE)
        return buddyExplicitPage(addr, nPages);

    uint64_t offset = (uint64_t)addr - availableBase;
    if (offset % (PAGE_SIZE * (1UL << (nr_bitmap - 1))) != 0)
        return false;

    int i = offset / (PAGE_SIZE * (1UL << (nr_bitmap - 1)));

    return bitset[nr_bitmap - 1].set(i, ((nPages * PAGE_SIZE - 1) / BUDDY_THRESH) + 1, BIT_SET_FORCE) != -1;
}

/*
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
*/

/*
void PageManager::setBit(size_t index, bool t) {
    uint8_t temp = bitset[index / 8];
    if (t)
        bitset[index / 8] = temp | (1 << (index % 8));
    else
        bitset[index / 8] = temp & ~(1 << (index % 8));
}

*/

/*
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
*/

/*
void PageManager::freePages(void *addr, int nPages) {
    size_t start = ((uint64_t)addr - this->availableBase) / 4096;
    for (int i = 0; i < nPages; i++) {
        this->setBit(start + i, false);
    }
}
*/

/*
bool PageManager::getBit(int order, size_t index) {
    return bitset[order].get_and_set(index);
}
*/
