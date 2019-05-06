#include <cstdio>
#include <cstring>
#include "util.h"
#include "panic.h"
#include "slub.h"
#include "spin_lock.h"

//#define SLUB_DEBUG

#define OBJECTS_PER_SLAB 64
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

struct slab {
    /* first cache line */
    slab *next;
    slab *prev;
    void *mem; 
    int32_t free_nr;
    /* this is the size without object_header */
    int32_t size_each;
    slub_per_cpu *slub_cache;
    char padding[24];

    /* second cache line */
    uint64_t bitmap[OBJECTS_PER_SLAB >> 6];
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
    auto ret = (slab *)mapper(alloc_size);
    SLUB_ASSERT(ret);
    memset(ret, 0, sizeof(slab));
    
    ret->mem  = (void *)((((uint64_t)ret + sizeof(slab) - 1UL) / object_size_header)
        * object_size_header
        + object_size_header); 
    
    //ret->mem = (char *)ret + slab_header_len;

    ret->free_nr = OBJECTS_PER_SLAB;
    ret->size_each = object_size;
    return ret;
}
 
LIBOS_INLINE void *slab_allocate_object(slab *slb) {
    SLUB_ASSERT(slb->free_nr > 0);      

    for (int i = 0; i < OBJECTS_PER_SLAB >> 6; i++) {
        int j = __builtin_ctzl(~*(slb->bitmap + i));
        if (j == 64) continue;

        void *ret = (char *)slb->mem
                  + (slb->size_each + sizeof(object_header)) * ((i << 6) + j)
                  + sizeof(object_header);
        *(slb->bitmap + i) |= (1UL << (uint64_t)j);
        
        object_header *header =
            (object_header *)((char *)ret - sizeof(object_header));
        header->parent = slb;
#ifdef SLUB_DEBUG
        header->canary = OBJECT_CANARY;
#endif
        slb->free_nr -- ;
        return ret;
    }

    SLUB_ASSERT(false);
    __builtin_unreachable();
}

LIBOS_INLINE void slab_free_object(slab *slb, object_header *header) {
    SLUB_ASSERT(slb->free_nr < OBJECTS_PER_SLAB); 
    
    int slot = ((uint64_t)header - (uint64_t)slb->mem)
        / (slb->size_each + sizeof(object_header));
    SLUB_ASSERT(slot >= 0);
    SLUB_ASSERT(slot < OBJECTS_PER_SLAB);

    int i = slot / 64;
    int j = slot % 64;
   
    SLUB_ASSERT((*(slb->bitmap + i) & (1UL << (uint64_t)j)) != 0);
    *(slb->bitmap + i) &= ~(1UL << (uint64_t)j);

    slb->free_nr ++;
}

void *slub_allocate_per_cpu(slub_per_cpu *slub_per_cpu) {
    void *ret = nullptr;
    slub_per_cpu->lock.lock();
    
    if (slub_per_cpu->partial_list) {
#ifdef SLUB_DEBUG
        libos_print("goto partial list");
#endif
        slab *partial = slub_per_cpu->partial_list;
        ret = slab_allocate_object(partial);

        if (partial->free_nr == 0) {
#ifdef SLUB_DEBUG
            libos_print("moving slab %p to full list", partial);
#endif
            list_del(&slub_per_cpu->partial_list, partial);
            list_add_head(&slub_per_cpu->full_list, partial);
        }
        goto out;
    } else if (slub_per_cpu->empty_list) {
#ifdef SLUB_DEBUG
        libos_print("goto empty list");
#endif
        slab *empty = slub_per_cpu->empty_list;
        ret = slab_allocate_object(empty);

#ifdef SLUB_DEBUG
            libos_print("moving slab %p to partial list", empty);
#endif
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
#ifdef SLUG_DEBUG
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
