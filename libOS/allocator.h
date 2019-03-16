#ifndef ALLOCATOR_H
#define ALLOCATOR_H
#include <sgx_arch.h>
#include <spin_lock.h>
#include <stdbool.h>
#include <cstddef>
#include <boost/intrusive/list.hpp>
#include <boost/intrusive/rbtree.hpp>
#define CHUNK_LIST_SIZE 20
using namespace boost::intrusive;

//Define a list that will store MyClass using the public member hook
class MemoryArea {
private:
public:
    MemoryArea(size_t l, vaddr vm);
    size_t len;
    vaddr memBase;
   //This is a member hook
    list_member_hook<> list_hook_;
    set_member_hook<> rbtree_hook_;
};

template<class T>
class __key_comp {
public:
     constexpr bool operator()(const T &t1, const T &t2) const{
        return t1.memBase < t2.memBase;
    }
};


typedef member_hook<MemoryArea, list_member_hook<>, &MemoryArea::list_hook_> MemberHookList;
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
public:
    size_t len;
    Allocator(size_t len, vaddr heapBase);
    void *malloc(size_t len);
    void free(vaddr baseAddr);
};


#endif
