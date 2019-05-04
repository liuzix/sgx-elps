#ifndef ALLOCATOR_H

#define ALLOCATOR_H
#include <sgx_arch.h>
#include <spin_lock.h>
#include <stdbool.h>
#include <cstddef>
#include <cassert>
#include <boost/intrusive/list.hpp>
#include <boost/intrusive/rbtree.hpp>
#include <unordered_map>
#define CHUNK_LIST_SIZE 40
using namespace boost::intrusive;


#define ALLOCATOR_DEBUG
//Define a list that will store MyClass using the public member hook
class MemoryArea {
private:
public:
    MemoryArea(size_t l, vaddr vm);
    size_t len;
    bool free;
    vaddr memBase;
   //This is a member hook
    list_member_hook<link_mode<normal_link>> list_hook_;
    set_member_hook<> rbtree_hook_;

    int checksum;
    uint64_t canary = 0xbeefbeef;
    void setChecksum()
    {
        checksum = 0;
        for (size_t i = 0; i < offsetof(MemoryArea, checksum); i++)
            checksum += ((char *)this)[i];

    }
    void checkIntegrity() {
        int actualChecksum = 0;
        for (size_t i = 0; i < offsetof(MemoryArea, checksum); i++)
            actualChecksum += ((char *)this)[i];
        if (this->checksum != actualChecksum) {
            __asm__("ud2");
        }
    }

};

template<class T>
class __key_comp {
public:
     constexpr bool operator()(const T &t1, const T &t2) const{
        return t1.memBase < t2.memBase;
    }
};


typedef member_hook<MemoryArea, list_member_hook<link_mode<normal_link>>, &MemoryArea::list_hook_> MemberHookList;
typedef list<MemoryArea, MemberHookList> MemberList;

typedef member_hook<MemoryArea, set_member_hook<>,
    &MemoryArea::rbtree_hook_> MemberHookRbtree;
typedef rbtree<MemoryArea, MemberHookRbtree, compare<__key_comp<MemoryArea>>> MemberRbtree;

class Allocator {
private:
    SpinLock rbtreeLock;
    SpinLock listLock;
    MemberRbtree root;
    MemberList chunkList[CHUNK_LIST_SIZE];

    void dump();
public:
    size_t len;
    Allocator(size_t len, vaddr heapBase);
    void *malloc(size_t len);
    void free(vaddr baseAddr);
    size_t getLen(void *ptr);
    void checkWholeTree();

    void expandHeap(void *base, size_t len);
};

//#ifdef IS_LIBOS
extern Allocator *unsafeAllocator;
extern Allocator *safeAllocator;
//#endif
void initUnsafeMalloc(void *base, size_t len);
void *unsafeMalloc(size_t len);
void unsafeFree(void *ptr);
void initSafeMalloc(size_t len);

template<class ObjType, typename ...Args>
ObjType* createUnsafeObj(Args&&... args) {
    char *mem = (char*)unsafeMalloc(sizeof(ObjType));
    return new ((void*)mem) ObjType(std::forward<Args>(args)...);
}

template <class T>
struct UnsafeAllocator {
    typedef T value_type;
    
    UnsafeAllocator() = default;
  
    T* allocate(std::size_t n) {
        return (T*)unsafeAllocator->malloc(n * sizeof(T));
    }
  
    void deallocate(T* p, std::size_t) {
        unsafeAllocator->free((vaddr)p);
    }
};
template <class T, class U>
bool operator==(const UnsafeAllocator<T>&, const UnsafeAllocator<U>&) { return true; }
template <class T, class U>
bool operator!=(const UnsafeAllocator<T>&, const UnsafeAllocator<U>&) { return false; }



#endif
