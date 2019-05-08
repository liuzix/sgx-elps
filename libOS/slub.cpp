#include <cstdio>
#include <cstring>
#include "util.h"
#include "panic.h"
#include "slub.h"
#include "spin_lock.h"
#include "bitmap.h"

//#define SLUB_DEBUG

#define SLAB_SIZE (16UL * 1024UL)  // 16K per slab
#define OBJECTS_PER_SLAB 64UL
#define OBJECT_CANARY 0xdeadbeef

#define SLUB_FATAL(...) do {  \
    libos_print(__VA_ARGS__); \
    __asm__ volatile ("ud2"); \
} while (0)

#ifdef SLUB_DEBUG
#define SLUB_ASSERT(x) do {   \
    if (x) break;             \
    SLUB_FATAL("SLUB fatal: %s, line %d", \
            __FILE__, __LINE__); \
} while (0)
#else
#define SLUB_ASSERT(x) do {} while (0)
#endif

struct slub_per_cpu;

constexpr size_t max_bitmap_size(size_t object_size) {
    return (SLAB_SIZE / object_size) >> 3;
}

constexpr size_t first_off(size_t slab_header_size, size_t object_size) {
    return ((slab_header_size - 1) / object_size + 1) * object_size;
}

constexpr int objects_per_slab(size_t slab_header_size, size_t object_size) {
    return (SLAB_SIZE - first_off(slab_header_size, object_size)) / object_size;
}


struct slab {
    /* first cache line */
    slab *next = nullptr;
    slab *prev = nullptr;
    void *mem = nullptr; 
    int32_t free_nr = OBJECTS_PER_SLAB;
    /* this is the size without object_header */
    int32_t size_each = 0;
    slub_per_cpu *slub_cache = nullptr;
    char padding[24];
    
    /* second cache line */
    Bitmap<OBJECTS_PER_SLAB> bitmap;
} __attribute__((packed));

struct slub_per_cpu {
    SpinLock lock;
    slab *empty_list;
    slab *partial_list;
    slab *full_list;
    MapperFuncT mapper;
    UnmapperT unmapper;
    int32_t size_each;
};

struct object_header {
    slab *parent;
#ifdef SLUB_DEBUG
    uint64_t canary;
#endif
};

static slab *new_slab(MapperFuncT, size_t object_size);

static void *slab_allocate_object(slab *);

LIBOS_INLINE void list_add_head(slab **head, slab *obj) {
   obj->next = *head;
   if (obj->next)
       obj->next->prev = obj;
   obj->prev = nullptr;
   *head = obj;
}

LIBOS_INLINE void list_del(slab **head, slab *obj) {
    if (obj->prev)
        obj->prev->next = obj->next;
    else
        *head = obj->next;

    if (obj->next)
        obj->next->prev = obj->prev;

    obj->prev = nullptr;
    obj->next = nullptr;
}

slub_per_cpu *new_slub_per_cpu(size_t objectSize, MapperFuncT mapper, UnmapperT unmapper) {
    auto ret = new slub_per_cpu; 
    
    ret->empty_list = nullptr;
    ret->partial_list = nullptr;
    ret->full_list = nullptr;
    ret->mapper = mapper;
    ret->unmapper = unmapper;
    ret->size_each = objectSize;
   
    auto first_slab = new_slab(mapper, objectSize);
    first_slab->slub_cache = ret;

    list_add_head(&ret->empty_list, first_slab);
    return ret;
}

static slab *new_slab(MapperFuncT mapper, size_t object_size) {
    size_t object_size_header = object_size + sizeof(object_header);

    size_t alloc_size = ((OBJECTS_PER_SLAB * object_size_header - 1) & ~4095UL) + 4096UL;
    auto ret = new (mapper(alloc_size)) slab;
    SLUB_ASSERT(ret);
    
    ret->mem  = (void *)((((uint64_t)ret + sizeof(slab) - 1UL) / object_size_header)
        * object_size_header
        + object_size_header); 
    
    ret->free_nr = OBJECTS_PER_SLAB;
    ret->size_each = object_size;
    return ret;
}
 
LIBOS_INLINE void *slab_allocate_object(slab *slb) {
    SLUB_ASSERT(slb->free_nr > 0);      

    ssize_t slot = slb->bitmap.scan_and_set();

    SLUB_ASSERT(slot >= 0); 
    char *ret = (char *)slb->mem +
        slot * (slb->size_each + sizeof(object_header)) + sizeof(object_header);
    object_header *header =
        (object_header *)((char *)ret - sizeof(object_header));
    header->parent = slb;
#ifdef SLUB_DEBUG
    header->canary = OBJECT_CANARY;
#endif
    slb->free_nr -- ;
    return ret;
}

LIBOS_INLINE void slab_free_object(slab *slb, object_header *header) {
    SLUB_ASSERT((size_t)slb->free_nr < OBJECTS_PER_SLAB); 
    
    int slot = ((uint64_t)header - (uint64_t)slb->mem)
        / (slb->size_each + sizeof(object_header));
    SLUB_ASSERT(slot >= 0);
    SLUB_ASSERT((size_t)slot < OBJECTS_PER_SLAB);

    slb->bitmap.unset(slot);
    slb->free_nr ++;
}

void *slub_allocate_per_cpu(slub_per_cpu *slub_per_cpu) {
    void *ret = nullptr;
    slub_per_cpu->lock.lock();
    
    if (slub_per_cpu->partial_list) {
        slab *partial = slub_per_cpu->partial_list;
        ret = slab_allocate_object(partial);

        if (partial->free_nr == 0) {
            list_del(&slub_per_cpu->partial_list, partial);
            list_add_head(&slub_per_cpu->full_list, partial);
        }
        goto out;
    } else if (slub_per_cpu->empty_list) {
        slab *empty = slub_per_cpu->empty_list;
        ret = slab_allocate_object(empty);

        list_del(&slub_per_cpu->empty_list, empty);
        list_add_head(&slub_per_cpu->partial_list, empty);
        goto out;
    }

    /* slow path */
    slub_per_cpu->partial_list = new_slab(slub_per_cpu->mapper,
                                          slub_per_cpu->size_each);    
    slub_per_cpu->partial_list->slub_cache = slub_per_cpu;
    ret = slab_allocate_object(slub_per_cpu->partial_list);
out:
    slub_per_cpu->lock.unlock();
    return ret;
}

void slub_free(void *obj) {
    object_header *header =
        (object_header *)((char *)obj - sizeof(object_header));
#ifdef SLUB_DEBUG
    SLUB_ASSERT(header->canary == OBJECT_CANARY);
#endif
    
    header->parent->slub_cache->lock.lock();
    slab_free_object(header->parent, header);

    slub_per_cpu *slub = header->parent->slub_cache;
    
    if (header->parent->free_nr == 1) {
        list_del(&slub->full_list, header->parent);
        list_add_head(&slub->partial_list, header->parent);
    } else if (header->parent->free_nr == OBJECTS_PER_SLAB) {
        list_del(&slub->partial_list, header->parent);
        list_add_head(&slub->empty_list, header->parent);
    }

    header->parent->slub_cache->lock.unlock();
}

#define NUM_SLUB_BUCKETS 10
SlubCache *unsafeSlubBuckets[NUM_SLUB_BUCKETS];

void init_unsafe_slub_buckets() {
    for (int i = 0; i < NUM_SLUB_BUCKETS; i++) {
        unsafeSlubBuckets[i] = createUnsafeSlub(8 << (i + 1));
    }
}

void *unsafe_slub_malloc(size_t len) {
    if (len <= 8) return unsafeSlubBuckets[0]->allocate();

    int index = 64 - __builtin_clzl(len - 1) - 3;

    if (index >= NUM_SLUB_BUCKETS) return unsafeMalloc(len);
    //libos_print("unsafe_slub_malloc: len = %d, bucket = %d", len, index);
    return unsafeSlubBuckets[index]->allocate();
}

void unsafe_slub_free(void *ptr) {
    slub_free(ptr);
}
