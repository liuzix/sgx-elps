#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include "allocator.h"
#include "mmap.h"
#include "util.h"
#include "thread_local.h"

using MapperFuncT = void *(*)(size_t);
using UnmapperT = void (*)(void *, size_t);
/* opaque in API */
struct slub_per_cpu;

slub_per_cpu *new_slub_per_cpu(size_t objectSize, MapperFuncT, UnmapperT);
void *slub_allocate_per_cpu(slub_per_cpu *slub_per_cpu);
void slub_free(void* obj);

struct SlubCache {
public:
    size_t size;
    MapperFuncT mapperFunc;
    UnmapperT unmapperFunc;

    SlubCache(size_t objectSize,
            MapperFuncT mapper,
            UnmapperT unmapper) {
        size = objectSize;
        mapperFunc = mapper;
        unmapperFunc = unmapper;
    } 

    __attribute__((always_inline)) 
    void *allocate() {
        slub_per_cpu *cache = perCPUCache.get();
        if (!cache) {
           cache = new_slub_per_cpu(size, mapperFunc, unmapperFunc);
           *perCPUCache = cache;
        }

        return slub_allocate_per_cpu(cache);
    }

    __attribute__((always_inline))
    void free(void *obj) {
        slub_free(obj);
    }
private:
    PerCPU<slub_per_cpu *> perCPUCache;
};

static void unsafeFreeWrapper(void *ptr, size_t) {
    unsafeFree(ptr);
}

static void *mmapWrapper(size_t size) {
    return libos_mmap(nullptr, size);
}

static inline SlubCache *createUnsafeSlub(size_t objectSize) {
    return new SlubCache(objectSize, &unsafeMalloc, &unsafeFreeWrapper);
}

static inline SlubCache *createSafeSlub(size_t objectSize) {
    return new SlubCache(objectSize, &mmapWrapper, &libos_munmap);
}
